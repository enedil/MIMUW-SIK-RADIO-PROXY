#include <poll.h>
#include <iostream>
#include <algorithm>
#include <exception>
#include <ext/stdio_filebuf.h>
#include <cstring>
#include <istream>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include "../common/error.h"
#include "message.h"
#include "telnetserver.h"
#include "cyclicbuffer.h"

namespace {
    volatile sig_atomic_t interrupt_occured;
    void sigint_handler([[maybe_unused]] int signo) {
        interrupt_occured = 1;
    }

    class Menu {
    public:
        Menu(std::mutex& mtx, std::vector<TelnetServer::Proxy>& proxies);
        void resetPosition();
        void arrowUp();
        void arrowDown();
        void draw(std::ostream& output, std::string const& statusLine);
        size_t getPosition();
    private:
        std::mutex& mtx;
        std::vector<TelnetServer::Proxy>& proxies;
        size_t position;
    };

    Menu::Menu(std::mutex& mtx, std::vector<TelnetServer::Proxy>& proxies) : 
        position(0), 
        mtx(mtx), 
        proxies(proxies) 
    {}

    void Menu::resetPosition() {
        position = 0;
    }

    size_t Menu::getPosition() {
        return position;
    }

    void Menu::arrowUp() {
        if (position > 0)
            position -= 1;
    }

    void Menu::arrowDown() {
        std::lock_guard<std::mutex> lock(mtx);
        if (position <= proxies.size())
            position += 1;
    }

    void Menu::draw(std::ostream& output, std::string const& statusLine) {
        static const std::string star = " *";
        static const std::string CLRF = "\r\n";
        static const std::string CSI = "\x1b[";
        output << CSI << "2J"; // clear screen
        output << CSI << "H";  // go the beginning
        output << "Szukaj pośrednika";
        if (position == 0)
            output << star;
        output << CLRF;
        {
            std::scoped_lock<std::mutex> lock(mtx);
            for (size_t i = 0; i < proxies.size(); ++i) {
                output << "Przekaźnik:\t" << proxies[i].name;
                if (position == i+1)
                    output << star;
                output << CLRF;
            }
            output << "Zakończ połączenie";
            if (position == 1+proxies.size())
                output << star;
            output << CLRF;
        }
        if (!statusLine.empty())
            output << statusLine << CLRF;
        output.flush();
    }

    bool waitUntilReady(int filedes, short events, int timeout = 50) {
        pollfd fd { .fd = filedes, .events = events, .revents = 0 };
        int rv = poll(&fd, 1, timeout);
        if (rv < 0 && errno == EINTR && interrupt_occured)
            throw Interrupt();
        if (rv < 0 || (fd.revents & POLLERR))
            syserr("poll");
        return rv == 1;
    }

    int timedGetChar(std::istream& input, int filedes, char& c, int timeout = 50) {
        if (input.rdbuf()->in_avail() <= 0) {
            bool rv = waitUntilReady(filedes, POLLIN, timeout);
            if (!rv)
                return -1;
        }
        return static_cast<int>(static_cast<bool>(input.get(c)));
    }
}


TelnetServer::TelnetServer(unsigned port, int timeout, sockaddr_in proxyAddr) : proxyAddr(proxyAddr) {
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        syserr("signal(SIGINT)");
    sockaddr_in localAddress;
    std::memset(&localAddress, 0, sizeof(localAddress));
    localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(port);
    listenerSock.bind(localAddress);
    if (listen(listenerSock, 1) < 0)
        syserr("listen");

    udpSock.setSockOpt(SOL_SOCKET, SO_BROADCAST, 1);

    keepAlive = std::make_unique<KeepAlive>(udpSock);
    proxyReceiver = std::make_unique<ProxyReceiver>(udpSock, this, timeout);
}

void TelnetServer::discover(std::optional<sockaddr_in>&& recepient) {
    uint8_t dummy[1] = {0};
    if (recepient)
        Message::sendMessage(udpSock, MessageType::DISCOVER, dummy, dummy, recepient.value()); 
    else
        Message::sendMessage(udpSock, MessageType::DISCOVER, dummy, dummy, proxyAddr); 
}

void TelnetServer::loop() {
    while (true) {
        if (interrupt_occured)
            return;
        if (!waitUntilReady(listenerSock, POLLIN))
            continue;
        int telnetSock = accept(listenerSock, NULL, NULL);
        if (telnetSock == -1)
            syserr("accept");
        if (!handleConnection(telnetSock))
            break;
    }
}

bool TelnetServer::handleConnection(int sockin) {
    static const std::string telnetCharacterMode = "\377\375\042\377\373\001";
    static const std::string CSI = "\x1b[";
    static const std::string RETURN("\r\0", 2);
    int sockout = dup(sockin);
    if (sockout < 0) {
        close(sockin);
        return false;
    }
    __gnu_cxx::stdio_filebuf<char> filebufin(sockin, std::ios::in | std::ios::binary), 
                                   filebufout(sockout, std::ios::out | std::ios::binary);
    std::istream telnetin(&filebufin);
    std::ostream telnetout(&filebufout);
    telnetout << telnetCharacterMode;
    telnetout.flush();

    CyclicBuffer cyclicBuffer(3);
    char c;
    Menu menu(availableProxiesMutex, availableProxies);
    int v;
    bool changed = true;
    while ((v = timedGetChar(telnetin, sockin, c)) != 0) {
        if (changed) {
            std::string s;
            {
                std::lock_guard<std::mutex> lock(metadataMutex);
                s = metadata;
            }
            menu.draw(telnetout, s);
        }
        if (interrupt_occured)
            return false;
        if (v == -1)
            continue;
        cyclicBuffer.append(c);
        changed = false;
        if (cyclicBuffer.endsWith(CSI + "A")) {
            menu.arrowUp();
            changed = true;
        }
        else if (cyclicBuffer.endsWith(CSI + "B")) {
            menu.arrowDown();
            changed = true;
        }
        else if (cyclicBuffer.endsWith(RETURN)) {
            changed = true;
            std::scoped_lock<std::mutex, std::mutex, std::mutex> lock(availableProxiesMutex, currentProxyMutex, metadataMutex);
            size_t position = menu.getPosition(); 
            menu.resetPosition(); 
            if (position == 0) { // wyszukiwanie pośredników
                try {
                    availableProxies.clear();
                    discover();
                } catch (...) {
                    return true;
                }
            } else if (position == 1+availableProxies.size()) { // zakończ
                return false;
            } else if (position <= availableProxies.size()) { // zmiana pośrednika
                Proxy* proxy = &availableProxies[position-1];
                currentProxy = proxy;
                metadata = "";
                discover(proxy->address);
                keepAlive->sendPipeMessage(proxy->address);
                proxyReceiver->sendPipeMessage(proxy->address);
            } else {
                throw std::runtime_error("got index bigger than maximum");
            }
        }

    }
    return true;
}

void TelnetServer::dropProxy() {
    std::scoped_lock<std::mutex, std::mutex, std::mutex> lock(availableProxiesMutex, currentProxyMutex, metadataMutex);
    metadata = "";
    if (currentProxy) {
        availableProxies.erase(
                std::remove(std::begin(availableProxies), 
                    std::end(availableProxies),
                    *currentProxy.value()));
    }
    currentProxy = std::nullopt;
    keepAlive->sendPipeMessage(Command::NoAddress);
}

void TelnetServer::IAM(std::vector<uint8_t> const& name, sockaddr_in& address) {
    std::string sname = TelnetServer::toString(name);
    std::lock_guard<std::mutex> lock(availableProxiesMutex);
    if (std::find_if(std::begin(availableProxies), std::end(availableProxies), 
                [&](auto x){ return x.address == address; }) == std::end(availableProxies))
        availableProxies.push_back(Proxy {sname, address});
}

void TelnetServer::METADATA(std::vector<uint8_t> const& metadata) {
    std::lock_guard<std::mutex> lock(metadataMutex);
    this->metadata = TelnetServer::toString(metadata);
}

std::string TelnetServer::toString(std::vector<uint8_t> const& data) {
    return std::string {reinterpret_cast<const char*>(data.data()), data.size()};
}
