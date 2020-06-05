#include <string>
#include <unistd.h>
#include "error.h"
#include "socket.h"

Socket::operator int() {
    return filedes;
}

void Socket::bind(const sockaddr_in& address) {
    if (::bind(filedes, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0)
        syserr("bind()");
}

Socket::~Socket() {
    if (close(filedes) != 0)
        syserr("closing filedes");
}

UdpSocket::UdpSocket() {
    filedes = socket(AF_INET, SOCK_DGRAM, 0);
    if (filedes < 0)
        syserr("socket(UDP)");
}

TcpSocket::TcpSocket() {
    filedes = socket(AF_INET, SOCK_STREAM, 0);
    if (filedes < 0)
        syserr("socket(UDP)");
}

void TcpSocket::sendAll(const char* data, size_t size) {
    const char* ptr = data;
    while (size > 0) {
        ssize_t sent = write(filedes, ptr, size);
        if (sent <= 0)
            syserr("write(TCP)");
        size -= static_cast<size_t>(sent);
        ptr += sent;
    }
}

void TcpSocket::readAll(char* data, size_t size) {
    char* end = data + size;
    while (data < end) {
        ssize_t rcv = read(filedes, data, end-data);
        if (rcv <= 0)
            syserr("read");
        data += rcv;
    }
}