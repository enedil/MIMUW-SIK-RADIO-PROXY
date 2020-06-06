#pragma once
#include "error.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

// IPv4 socket abstraction. Cannot be instantiated directly. Socket operation managed
// by this class throw std::system_error in case of failed operation.
class Socket {
  public:
    // Gets filedes from managed socket object, so that it can be used transparently
    // in BSD socket API.
    operator int();

    // Closes socket.
    ~Socket();

    // Bind socket on specified address.
    void bind(const sockaddr_in &address);

    // Set socket option.
    template <typename T> void setSockOpt(int level, int optname, T optval) {
        if (setsockopt(filedes, level, optname, &optval, sizeof(optval)) < 0)
            syserr("setsockopt");
    }

  protected:
    // Disallow direct instantiation.
    Socket() = default;
    // File descriptor of the socket in question.
    int filedes;
};

// UdpSocket over IPv4.
class UdpSocket : public Socket {
  public:
    // Create socket.
    UdpSocket();
};

// TcpSocket over IPv4.
class TcpSocket : public Socket {
  public:
    // Create socket.
    TcpSocket();
    // Send all data (size bytes). Throws std::system_error in case of error.
    void sendAll(const char *data, size_t size);
    // Read all data (size bytes). Throws std::system_error in case of error.
    void readAll(char *data, size_t size);
};
