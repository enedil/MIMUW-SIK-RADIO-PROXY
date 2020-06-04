#include <algorithm>
#include <exception>
#include <ext/stdio_filebuf.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "error.h"
#include "message.h"
#include "telnetserver.h"

namespace {
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
        static const std::string CSI = "\x1B[";
        output << CSI << "2J"; // clear screen
        output << CSI << "H";  // go the beginning
        output << "Szukaj pośrednika";
        if (position == 0)
            output << star;
        output << CLRF;
        {
            std::scoped_lock<std::mutex> lock(mtx);
            for (size_t i = 0; i < proxies.size(); ++i) {
                output << proxies[i].name;
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

    bool endswith(std::string const& first, std::string const& second) {
        if (first.length() < second.length())
            return false;
        for (size_t i = 0; i < second.length(); ++i) {
            if (first[first.size()-i-1] != second[second.size()-i-1])
                return false;
        }
        return true;
    }
}


TelnetServer::TelnetServer(unsigned port, int timeout, sockaddr_in proxyAddr) : proxyAddr(proxyAddr) {
    sockaddr_in localAddress;
    std::memset(&localAddress, 0, sizeof(localAddress));
    localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(port);
    listenerSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenerSock == -1)
        syserr("socket() of listener");
    if (bind(listenerSock, reinterpret_cast<const sockaddr*>(&localAddress), sizeof(localAddress)) < 0)
        syserr("bind()");
    if (listen(listenerSock, (1<<16)) < 0)
        syserr("listen");

    udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock == -1)
        syserr("socket() udp");
    int optval = 1;
    if (setsockopt(udpSock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
        syserr("setsockopt broadcast");

    keepAlive = std::make_unique<KeepAlive>(udpSock);
    proxyReceiver = std::make_unique<ProxyReceiver>(udpSock, this, timeout);
}

void TelnetServer::discover(std::optional<sockaddr_in>&& recepient) {
    uint8_t dummy[1];
    if (recepient)
        Message::sendMessage(udpSock, DISCOVER, dummy, dummy, recepient.value()); 
    else
        Message::sendMessage(udpSock, DISCOVER, dummy, dummy, proxyAddr); 
}

TelnetServer::~TelnetServer() {
    if (shutdown(listenerSock, SHUT_RDWR) != 0)
        syserr("shutdown");
    if (close(listenerSock) != 0)
        syserr("close");
}

void TelnetServer::loop() {
    while (true) {
        int telnetSock = accept(listenerSock, NULL, NULL);
        if (telnetSock == -1)
            syserr("accept");
        if (!handleConnection(telnetSock))
            break;
    }
}

bool TelnetServer::handleConnection(int sockin) {
    static const std::string telnetCharacterMode = "\377\375\042\377\373\001";
    static const std::string CSI = "\x1B[";
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

    std::string cyclicBuffer("\xff", 3);
    char c;
    Menu menu(availableProxiesMutex, availableProxies);
    menu.draw(telnetout, "");
    while (telnetin.get(c)) {
        cyclicBuffer = cyclicBuffer.substr(1, cyclicBuffer.length()-1) + c;
        bool changed = false;
        if (endswith(cyclicBuffer, CSI + "A")) {
            menu.arrowUp();
            changed = true;
        }
        else if (endswith(cyclicBuffer, CSI + "B")) {
            menu.arrowDown();
            changed = true;
        }
        else if (endswith(cyclicBuffer, RETURN)) {
            changed = true;
            std::scoped_lock<std::mutex, std::mutex, std::mutex> lock(availableProxiesMutex, currentProxyMutex, metadataMutex);
            size_t position = menu.getPosition(); 
            menu.resetPosition(); 
            if (position == 0) { // wyszukiwanie pośredników
                discover();
            } else if (position == 1+availableProxies.size()) { // zakończ
                return false;
            } else if (position <= availableProxies.size()) { // zmiana pośrednika
                Proxy* proxy = &availableProxies[position-1];
                currentProxy = proxy;
                metadata = "";
                keepAlive->sendPipeMessage(proxy->address);
                proxyReceiver->sendPipeMessage(proxy->address);
            } else {
                throw std::runtime_error("got index bigger than maximum");
            }
        }

        if (changed) {
            std::string s;
            {
                std::lock_guard<std::mutex> lock(metadataMutex);
                s = metadata;
            }
            menu.draw(telnetout, s);
        }
    }
    return true;
}

void TelnetServer::dropProxy() {
    std::scoped_lock<std::mutex, std::mutex, std::mutex> lock(availableProxiesMutex, currentProxyMutex, metadataMutex);
    metadata = "";
    if (currentProxy) {
        std::remove(std::begin(availableProxies), std::end(availableProxies), *currentProxy.value());
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
