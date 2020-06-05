#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "error.h"

class Socket {
public:
    // gets filedes from managed socket object
    operator int();
    ~Socket();
    void bind(const sockaddr_in& address);
    template<typename T>
    void setSockOpt(int level, int optname, T optval) {
        if (setsockopt(filedes, level, optname, &optval, sizeof(optval)) < 0)
            syserr("setsockopt");
    }

protected:
    Socket() = default;
    int filedes;
};

class UdpSocket : public Socket {
public:
    UdpSocket();
};

class TcpSocket : public Socket {
public:
    TcpSocket();
    void sendAll(const char* data, size_t size);
    void readAll(char* data, size_t size);
};
