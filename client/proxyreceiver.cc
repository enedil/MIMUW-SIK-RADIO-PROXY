#include <iostream>
#include <iomanip>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <poll.h>
#include "proxyreceiver.h"
#include "telnetserver.h"
#include "message.h"
#include "../common/message.h"
#include "../common/error.h"
#include "../common/address.h"

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
    std::vector<uint8_t> data(1);
    data.resize(0);
    Message message(sockfd, data);
    pollfd fds[2];
    pollfd& pipeEv = fds[0];
    pollfd& sockEv = fds[1];
    pipeEv.fd = pipefd[0];
    sockEv.fd = sockfd;
    timespec ts{timeout, 0};
    while (true) {
        std::optional<sockaddr_in> currentAddress = std::nullopt;
        if (std::holds_alternative<Command>(msg)) {
            if (std::get<Command>(msg) == Command::Stop)
                return;
        } else {
            currentAddress = std::get<sockaddr_in>(msg);
        }
        pipeEv.events = POLLIN;
        sockEv.events = POLLIN;
        int ret = ppoll(fds, std::size(fds), &ts, NULL);
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
            address.sin_addr.s_addr = htonl(INADDR_ANY);
            address.sin_port = htons(0);
            address.sin_family = AF_INET;
            MessageType type;
            try {
                message.recvMessage(type, address);
            } catch (std::system_error& exc) {
                std::cerr << exc;
                continue; // drop packet
            }
            if (currentAddress && (address == currentAddress.value()))
                ts = {timeout, 0};
            switch (type) {
            case AUDIO:
            case METADATA:
                // Drop packet
                if (!currentAddress || !(address == currentAddress.value()))
                    continue;
                break;
            default:
                break;
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

void ProxyReceiver::playMusic(std::vector<uint8_t> const& data) {
    ssize_t written = write(STDOUT_FILENO, reinterpret_cast<const char*>(data.data()), data.size());
    if (written < 0 || static_cast<size_t>(written) != data.size())
        syserr("write to stdout");
}
