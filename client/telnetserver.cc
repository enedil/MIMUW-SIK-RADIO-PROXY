#define _GNU_SOURCE
#include <cstring>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <system_error>
#include "message.h"
#include "telnet-server.h"

static void syserr(char const* reason) {
    throw std::system_error { std::error_code(errno, std::system_category()), reason };
}

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
