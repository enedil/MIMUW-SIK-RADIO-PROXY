#include <cstring>
#include <iostream>
#include "../common/error.h"
#include "radiosender.h"

static std::array<uint16_t, 2> encodeHeader(MsgType type, size_t size);

RadioSender::RadioSender(unsigned port, std::optional<std::string> broadcastAddr, unsigned timeout) :
    timeout(timeout), isBroadcasted(broadcastAddr) {
        if (broadcastAddr) {
            ip_mreq_.imr_interface.s_addr = htonl(INADDR_ANY);
            if (inet_aton(broadcastAddr.value().c_str(), &ip_mreq_.imr_multiaddr) == 0)
                throw std::domain_error("inet_aton: invalid multicast adddress");
            sock.setSockOpt(IPPROTO_IP, IP_ADD_MEMBERSHIP, ip_mreq_);
            sock.setSockOpt(IPPROTO_IP, IP_MULTICAST_TTL, 16);
        }
        sockaddr_in local_address;
        local_address.sin_family = AF_INET;
        local_address.sin_addr.s_addr = htonl(INADDR_ANY);
        local_address.sin_port = htons(port);
        sock.bind(local_address);
}


RadioSender::~RadioSender() {
    if (isBroadcasted)
      if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &ip_mreq_, sizeof ip_mreq_) != 0)
          abort();
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
    if (end < begin)
        throw std::range_error("end < begin");
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
    if (size >= std::numeric_limits<uint16_t>::max())
        throw std::range_error("size >= std::numeric_limits<uint16_t>::max()");
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
    if (r < 0)
        syserr("sendmsg");
}

void RadioSender::sendToAllClients(msghdr msghdr) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto clients_copy = clients;
    for (auto& client : clients) {
        auto now = clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - client.second);
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
    controller.detach();
    int ret = -1;
    while (true) {
        auto [type, buf] = reader.readChunk();
        switch(type) {
        case ICY_AUDIO:
            if (buf.size() > 0)
                sendData(AUDIO, buf, std::nullopt);
            break;
        case ICY_METADATA:
            if (buf.size() > 0)
                sendData(METADATA, buf, std::nullopt);
            break;
        case INTERRUPT:
            return EXIT_SUCCESS;
            break;
        }
    }
    return ret;
}

void RadioSender::controller(RadioReader& reader) {
    sockaddr_in clientAddress;
    socklen_t sendToAClientLen = sizeof(clientAddress);
    while (true) {
        uint16_t type;
        if (recvfrom(sock, &type, sizeof(type), 0, reinterpret_cast<sockaddr*>(&clientAddress), &sendToAClientLen) < static_cast<ssize_t>(sizeof(type))) {
            std::cerr << "recvfrom bad" << "\n";
        }
        type = ntohs(type);
        if (type == DISCOVER) {
            auto desc = reader.description();
            auto begin = reinterpret_cast<const uint8_t*>(desc.c_str());
            auto end = begin + desc.length();
            sendData(IAM, begin, end, clientAddress);
        }
        if (type == DISCOVER || type == KEEPALIVE) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (type == DISCOVER)
                clients[clientAddress] = clock::now();
            else {
                auto it = clients.find(clientAddress);
                if (it != clients.end())
                    it->second = clock::now();
            }
        }
    }
}
