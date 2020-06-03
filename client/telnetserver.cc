#include <memory>
#include <mutex>
#include <vector>
#define _GNU_SOURCE
#include <cstring>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "error.h"
#include "message.h"
#include "telnetserver.h"

TelnetServer::TelnetServer(unsigned port) {
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
    keepAlive = std::make_unique<KeepAlive>(listenerSock);
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
        if (shutdown(telnetSock, SHUT_RDWR) != 0)
            syserr("shutdown");
        if (close(telnetSock) != 0)
            syserr("close");
    }
}

bool TelnetServer::handleConnection(int sock) {
    return true;
}

void TelnetServer::dropProxy() {
    std::lock_guard<std::mutex> lock(currentProxyMutex);
    currentProxy = std::nullopt;
    keepAlive->sendPipeMessage(Command::NoAddress);
}

void TelnetServer::IAM(std::vector<uint8_t> const& name, sockaddr_in& address) {
    std::string sname = TelnetServer::toString(name);
    std::lock_guard<std::mutex> lock(availableProxiesMutex);
    availableProxies.push_back(Proxy {sname, address});
}

void TelnetServer::METADATA(std::vector<uint8_t> const& metadata) {
    std::lock_guard<std::mutex> lock(metadataMutex);
    this->metadata = TelnetServer::toString(metadata);
}

std::string TelnetServer::toString(std::vector<uint8_t> const& data) {
    return std::string {reinterpret_cast<const char*>(data.data()), data.size()};
}
