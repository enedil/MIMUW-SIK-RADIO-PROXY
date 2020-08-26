#include <iostream>
#include "message.h"
#include "error.h"
#include <cstring>
#include <limits>

Message::Message(int filedes, std::vector<uint8_t> &buffer)
    : buffer(buffer), filedes(filedes) {
    std::memset(&hdr, 0, sizeof(hdr));
    scattergather[0].iov_base = header.data();
    scattergather[0].iov_len = header.size() * sizeof(header[0]);
    hdr.msg_iov = scattergather.data();
    hdr.msg_iovlen = scattergather.size();
    hdr.msg_namelen = sizeof(sockaddr_in);
}

void Message::sendMessage(MessageType type, sockaddr_in &destination) {
    if (buffer.size() > std::numeric_limits<uint16_t>::max())
        throw std::invalid_argument("data too long");
    hdr.msg_name = &destination;
    // Encode data.
    scattergather[1].iov_base = buffer.data();
    scattergather[1].iov_len = buffer.size();
    // Encode type.
    header[0] = htons(static_cast<uint16_t>(type));
    // Encode size.
    header[1] = htons(buffer.size());
    ssize_t rec = sendmsg(filedes, &hdr, 0);
    if (rec < 0)
        syserr("sendmsg");
}

void Message::recvMessage(MessageType &type, sockaddr_in &destination) {
    hdr.msg_name = &destination;
    buffer.resize(std::numeric_limits<uint16_t>::max());
    scattergather[1].iov_base = buffer.data();
    scattergather[1].iov_len = buffer.size();

    ssize_t received;
    if ((received = recvmsg(filedes, &hdr, 0)) < 0)
        syserr("recvmsg");
    std::cerr << "received:" << received << "\n";
    // If data is too short, accept that.
    if (static_cast<size_t>(received) < scattergather[0].iov_len) {
        throw std::length_error{"MESSAGE too short"};
    }
    header[0] = ntohs(header[0]);
    header[1] = ntohs(header[1]);
    // If we receive too little data (less than declared), ignore the fact.
    buffer.resize(std::min<size_t>(header[1], received - scattergather[0].iov_len));
    switch (header[0]) {
    case DISCOVER:
        type = DISCOVER;
        break;
    case IAM:
        type = IAM;
        break;
    case KEEPALIVE:
        type = KEEPALIVE;
        break;
    case AUDIO:
        type = AUDIO;
        break;
    case METADATA:
        type = METADATA;
        break;
    default: // Wrong message type.
        throw std::domain_error{"Invalid operation"};
    }
}
