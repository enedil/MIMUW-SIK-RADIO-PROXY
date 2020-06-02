#include <chrono>
#include <iterator>
#include <errno.h>
#include <cstring>
#include <limits>
#include <unistd.h>
#include <cassert>
#include <system_error>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <array>
#include <fstream>
#include "radiosender.h"

static std::array<uint16_t, 2> encodeHeader(MsgType type, size_t size);

RadioSender::RadioSender(unsigned port, std::optional<std::string> broadcastAddr, unsigned timeout) :
    timeout(timeout), isBroadcasted(broadcastAddr) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1) {
            throw std::system_error { std::error_code(errno, std::system_category()), "socket" };
        }
        if (broadcastAddr) {
            ip_mreq_.imr_interface.s_addr = htonl(INADDR_ANY);
            if (inet_aton(broadcastAddr.value().c_str(), &ip_mreq_.imr_multiaddr) == 0) {
                throw std::system_error { std::error_code(errno, std::system_category()), "invalid multicast adddress" };
            }
            if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ip_mreq_, sizeof ip_mreq_) < 0) {
                throw std::system_error { std::error_code(errno, std::system_category()), "setsockopt IP_ADD_MEMBERSHIP" };
            }
        }
        sockaddr_in local_address;
        local_address.sin_family = AF_INET;
        local_address.sin_addr.s_addr = htonl(INADDR_ANY);
        local_address.sin_port = htons(port);
        if (bind(sock, reinterpret_cast<struct sockaddr *>(&local_address), sizeof local_address) < 0)
            throw std::system_error { 
                std::error_code(errno, std::system_category()), "port already taken, or bind fails otherwise"
            };
}


RadioSender::~RadioSender() {
    if (isBroadcasted)
      if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &ip_mreq_, sizeof ip_mreq_) != 0)
          abort();
    close(sock);
}

class RAIIMsghdr {
public:
    RAIIMsghdr(uint8_t const* begin, uint8_t const* end, MsgType type);
    msghdr get();
private:
    std::array<iovec, 2> scattergather_array;
    std::array<uint16_t, 2> header;
};

RAIIMsghdr::RAIIMsghdr(uint8_t const* begin, uint8_t const* end, MsgType type) {
    assert(end >= begin);
    header = encodeHeader(type, end-begin);
    scattergather_array[0].iov_base = header.data();
    scattergather_array[0].iov_len = header.size() * sizeof(uint16_t);
    scattergather_array[1].iov_base = (void*)(begin);
    scattergather_array[1].iov_len = end - begin;
}

msghdr RAIIMsghdr::get() {
    msghdr ret;
    std::memset(&ret, 0, sizeof(ret));
    ret.msg_iov = scattergather_array.data();
    ret.msg_iovlen = scattergather_array.size();
    return ret;
}

static std::array<uint16_t, 2> encodeHeader(MsgType type, size_t size) {
    assert(size < std::numeric_limits<uint16_t>::max());
    std::array<uint16_t, 2> output;
    output[0] = htons(static_cast<uint16_t>(type));
    output[1] = htons(size);
    return output;
}

void RadioSender::sendData(MsgType type, std::vector<uint8_t> const& data, std::optional<sockaddr_in> address) {
    sendData(type, data.data(), data.data() + data.size(), address);
}

void RadioSender::sendData(MsgType type, uint8_t const* begin, uint8_t const* end, std::optional<sockaddr_in> address) {
    RAIIMsghdr m(begin, end, type);
    if (address)
        sendToAClient(m.get(), address.value());
    else
        sendToAllClients(m.get());
}

void RadioSender::sendToAClient(msghdr msghdr, sockaddr_in address) {
    msghdr.msg_name = reinterpret_cast<void*>(&address);
    msghdr.msg_namelen = sizeof(address);
    auto r = sendmsg(sock, &msghdr, 0);
    if (r < 0) {
        throw std::system_error { std::error_code(errno, std::system_category()), "sendmsg" };
    }
    //std::cerr << "sent " << r << "bytes\n";

}

void RadioSender::sendToAllClients(msghdr msghdr) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto clients_copy = clients;
    for (auto& client : clients) {
        //std::cerr << "Client: [" << ntohl(client.first.sin_addr.s_addr) << "," << ntohs(client.first.sin_port) << "]\n";
        auto now = clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - client.second);
        //std::cerr << "diff=" << diff.count() << "\n";
        if (diff.count() > timeout) {
            clients_copy.erase(client.first);
        } else {
            sendToAClient(msghdr, client.first);
        }
    }
    clients.swap(clients_copy);
}

int RadioSender::sendrecvLoop(RadioReader& reader) {
    std::thread controller {[&]() { this->controller(reader); }};
    int ret = -1;
    bool loop = true;
    std::ofstream logging("output");
    while (loop) {
        auto [type, buf] = reader.readChunk();
        switch(type) {
        case ICY_AUDIO:
            logging.write((char*)buf.data(), buf.size());
            sendData(AUDIO, buf, std::nullopt);
            break;
        case ICY_METADATA:
            sendData(METADATA, buf, std::nullopt);
            break;
        case END_OF_STREAM:
            ret = EXIT_SUCCESS;
            loop = false;
            exit(ret);
            break;
        case ERROR:
            ret = EXIT_FAILURE;
            loop = false;
            exit(ret);
            break;
        }
    }
    controller.join();
    return ret;
}

void RadioSender::controller(RadioReader& reader) {
    sockaddr_in clientAddress;
    socklen_t sendToAClientLen = sizeof(clientAddress);
    while (true) {
        uint16_t type;
        if (recvfrom(sock, &type, sizeof(type), 0, reinterpret_cast<sockaddr*>(&clientAddress), &sendToAClientLen) < sizeof(type)) {
//dfjsiofjdsi
        }
        type = ntohs(type);
        std::cerr << 
            "client addr:" << ntohl(clientAddress.sin_addr.s_addr)
            << " port:" << ntohs(clientAddress.sin_port) 
            << "type: " << type << std::endl;
        if (type == DISCOVER) {
            auto desc = reader.description();
            auto begin = reinterpret_cast<const uint8_t*>(desc.c_str());
            auto end = begin + desc.length();
            sendData(IAM, begin, end, clientAddress);
        } else if (type == KEEPALIVE) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients[clientAddress] = clock::now();
        }
    }
}
