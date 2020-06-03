#include <thread>
#include <optional>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include "proxyreceiver.h"
#include "telnetserver.h"
#include "message.h"
#include "error.h"

constexpr bool operator==(sockaddr_in const& lhs, sockaddr_in const& rhs) {
    return lhs.sin_addr.s_addr == rhs.sin_addr.s_addr && lhs.sin_port == rhs.sin_port;
}

ProxyReceiver::ProxyReceiver(int sockfd, TelnetServer* telnetServer, int timeout) : 
    telnetServer(telnetServer), sockfd(sockfd), timeout(timeout) {
    if (pipe(pipefd) != 0)
        syserr("pipe");
    std::thread t { &ProxyReceiver::loop, this };
    t.detach();
}


void ProxyReceiver::sendPipeMessage(PipeMessage const&& msg) {
    if (write(pipefd[1], &msg, sizeof(msg)) != sizeof(msg))
        syserr("write on pipe");
}

void ProxyReceiver::receivePipeMessage(PipeMessage& msg) {
    if (read(pipefd[0], &msg, sizeof(msg)) != sizeof(msg))
        syserr("read on pipe");
}

void ProxyReceiver::loop() {
    PipeMessage msg = Command::NoAddress;
    std::vector<uint8_t> data;
    pollfd fds[2];
    pollfd& pipeEv = fds[0];
    pollfd& sockEv = fds[1];
    pipeEv.fd = pipefd[0];
    sockEv.fd = sockfd;
    while (true) {
        std::optional<sockaddr_in> currentAddress = std::nullopt;
        if (std::holds_alternative<Command>(msg)) {
            if (std::get<Command>(msg) == Command::Stop)
                return;
        } else {
            currentAddress = std::get<sockaddr_in>(msg);
        }
        pipeEv.events = POLLIN;
        pipeEv.revents = 0;
        sockEv.events = POLLIN;
        sockEv.revents = 0;
        int ret = poll(fds, std::size(fds), timeout);
        if (ret < 0 && errno == EINTR)
            continue;
        else if (ret < 0)
            syserr("poll");
        if (ret == 0) {
            telnetServer->dropProxy();
            msg = Command::NoAddress;
        }
        if ((pipeEv.revents & POLLERR) || (sockEv.revents & POLLERR))
            syserr("POLLERR");
        if (pipeEv.revents & POLLIN) {
            receivePipeMessage(msg);
            continue;
        }
        if (sockEv.revents & POLLIN) {
            sockaddr_in address;
            MessageType type;
            Message::recvMessage(sockEv.fd, type, data, address);
            switch (type) {
            case AUDIO:
            case METADATA:
                if (!currentAddress || !(address == currentAddress.value()))
                    continue;
                break;
            default:
                continue;
            }
            dispatchMessage(type, data, address);
        }
    }
}

void ProxyReceiver::dispatchMessage(MessageType type, std::vector<uint8_t> const& data, sockaddr_in& address) {
    switch (type) {
    case AUDIO:
        playMusic(data);
        break;
    case METADATA:
        telnetServer->METADATA(data);
        break;
    case IAM:
        telnetServer->IAM(data, address);
        break;
    default:
        break; // just ignore
    }
}

ProxyReceiver::~ProxyReceiver() {
    sendPipeMessage(Command::Stop);
    close(pipefd[0]);
    close(pipefd[1]);
}
