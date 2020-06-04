#include "message.h"
#include <errno.h>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <iostream>

#include <unistd.h>

#include <cstring>
#include <array>
#include <limits>
#include <stdexcept>

namespace Message {
    void sendMessage(int sockfd, MessageType type, uint8_t* begin, uint8_t* end, sockaddr_in& destination) {
        std::array<uint16_t, 2> header;
        std::array<iovec, 2> scattergather;
        msghdr m;
        if (end < begin)
            throw std::invalid_argument("end < begin");
        if (end - begin > std::numeric_limits<uint16_t>::max())
            throw std::invalid_argument("data too long");
        header[0] = htons(static_cast<uint16_t>(type));
        header[1] = htons(static_cast<uint16_t>(end - begin));
        std::memset(&m, 0, sizeof(m));
        scattergather[0].iov_base = header.data();
        scattergather[0].iov_len = header.size() * sizeof(header[0]);
        scattergather[1].iov_base = (void*)begin;
        scattergather[1].iov_len = end - begin;
        m.msg_iov = scattergather.data();
        m.msg_iovlen = scattergather.size();
        m.msg_name = &destination;
        m.msg_namelen = sizeof(destination);
        ssize_t rec;
        rec = sendmsg(sockfd, &m, 0);
        if (rec < 0)
            throw "TODO"; //TODO
        if (static_cast<size_t>(rec) != scattergather[0].iov_len + scattergather[1].iov_len) {
            std::cerr << "err:" << rec << " " <<  strerror(errno) << "\n";
            throw "TODO"; //TODO
        }
    }

    void recvMessage(int sockfd, MessageType& type, std::vector<uint8_t>& data, sockaddr_in& source) {
        data.resize(std::numeric_limits<uint16_t>::max());
        std::array<uint16_t, 2> header;
        std::array<iovec, 2> scattergather;
        msghdr m;
        std::memset(&m, 0, sizeof(m));
        scattergather[0].iov_base = header.data();
        scattergather[0].iov_len = header.size() * sizeof(header[0]);
        scattergather[1].iov_base = data.data();
        scattergather[1].iov_len = data.size() * sizeof(data[0]);
        m.msg_iov = scattergather.data();
        m.msg_iovlen = scattergather.size();
        m.msg_name = &source;
        m.msg_namelen = sizeof(source);
        ssize_t received;
        if ((received = recvmsg(sockfd, &m, 0)) < 0)
            throw std::length_error { "recvmsg failed" }; // TODO
        if (static_cast<size_t>(received) < scattergather[0].iov_len) {
            std::cerr << "received=" << received << ",iov_len=" << scattergather[0].iov_len << "\n";
            throw std::length_error { "MESSAGE too short" }; // TODO
        }
        header[0] = ntohs(header[0]);
        header[1] = ntohs(header[1]);
        if (static_cast<size_t>(received) != scattergather[0].iov_len + header[1])
            throw "TODO"; // TODO
        data.resize(header[1]);
        switch (header[0]) {
        //case DISCOVER:      type = DISCOVER;    break;
        case IAM:           type = IAM;         break;
        //case KEEPALIVE:     type = KEEPALIVE;   break;
        case AUDIO:         type = AUDIO;       break;
        case METADATA:      type = METADATA;    break;
        default:
            throw "TODO"; // TODO
        }
    }
}
