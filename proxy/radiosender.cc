#include <chrono>
#include <iterator>
#include <errno.h>
#include <string.h>
#include <limits>
#include <unistd.h>
#include <cassert>
#include <system_error>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <vector>
#include <cstdlib>
#include "radiosender.h"

RadioSender::RadioSender(unsigned port, std::optional<std::string> broadcastAddr, unsigned timeout) :
    timeout(timeout), isBroadcasted(broadcastAddr) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1) {
            throw std::system_error { std::error_code(errno, std::system_category()), "socket" };
        }
        if (broadcastAddr) {
            ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            if (inet_aton(broadcastAddr.value().c_str(), &ip_mreq.imr_multiaddr) == 0) {
                throw std::system_error { std::error_code(errno, std::system_category()), "invalid multicast adddress" };
            }
            if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ip_mreq, sizeof ip_mreq) < 0) {
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
      if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &ip_mreq, sizeof ip_mreq) != 0)
          abort();
    close(sock);
}

class RAIIMsghdr {
public:
    RAIIMsghdr(uint8_t const* begin, uint8_t const* end, const std::vector<uint8_t>& header, size_t packetSize = 1500);
    msghdr get();
private:
    std::vector<iovec> scattergather_array;
    std::vector<std::vector<uint8_t>> parts;
};

RAIIMsghdr::RAIIMsghdr(uint8_t const* begin, uint8_t const* end, const std::vector<uint8_t>& header, size_t packetSize) {
    assert(end > begin);
    size_t packetCount = (static_cast<size_t>(end - begin) + packetSize - header.size() - 1) / (packetSize - header.size());
    size_t chunkLength = packetSize - header.size();
    scattergather_array.resize(packetCount);
    parts.resize(packetCount);
    for (size_t i = 0; i < packetCount; ++i) {
        parts[i].resize(packetSize);
        std::copy(std::begin(header), std::end(header), std::begin(parts[i]));
        if (static_cast<size_t>(end - begin) < chunkLength)
            parts[i].resize(static_cast<size_t>(end - begin));
        std::copy(begin, std::min(begin + chunkLength, end), std::begin(parts[i]) + static_cast<ssize_t>(header.size()));
        scattergather_array[i].iov_base = parts[i].data();
        scattergather_array[i].iov_len = parts[i].size();
        begin += chunkLength;
    }
}

msghdr RAIIMsghdr::get() {
    msghdr ret{};
    ret.msg_iov = scattergather_array.data();
    ret.msg_iovlen = scattergather_array.size();
    return ret;
}

static std::vector<uint8_t> encodeTypeHeader(MsgType type, std::vector<uint8_t> const& buf) {
    std::size_t headerSize = 2*sizeof(uint16_t);
    assert(buf.size() < std::numeric_limits<uint16_t>::max());
    std::vector<uint8_t> output(headerSize);
    *reinterpret_cast<uint16_t*>(output.data()) = htons(static_cast<uint16_t>(type));
    *reinterpret_cast<uint16_t*>(output.data() + sizeof(uint16_t)) = htons(buf.size());
    return output;
}

void RadioSender::sendData(MsgType type, std::vector<uint8_t> const& data, std::optional<sockaddr_in> address) {
    std::vector<uint8_t> header = encodeTypeHeader(type, data);
    RAIIMsghdr m(data.data(), data.data() + data.size(), header);
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
        throw std::system_error {
            std::error_code(errno, std::system_category()), 
                std::string("sendmsg: ") + strerror(errno) 
        };
    }

}

void RadioSender::sendToAllClients(msghdr msghdr) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto clients_copy = clients;
    for (auto& client : clients) {
        auto now = RadioSender::clock::now();
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
    std::thread controller {&RadioSender::controller, this};
    int ret = EXIT_SUCCESS;
    bool loop = true;
    while (loop) {
        auto [type, buf] = reader.readChunk();
        switch(type) {
        case ICY_AUDIO:
            sendData(AUDIO, buf, std::nullopt);
            break;
        case ICY_METADATA:
            sendData(METADATA, buf, std::nullopt);
            break;
        case END_OF_STREAM:
            ret = EXIT_SUCCESS;
            loop = false;
            break;
        case ERROR:
            ret = EXIT_FAILURE;
            loop = false;
            break;
        }
    }
    controller.join();
    return ret;
}

void RadioSender::controller() {
    //sockaddr_in
}
