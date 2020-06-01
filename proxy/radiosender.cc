#include <limits>
#include <unistd.h>
#include <cassert>
#include <system_error>
#include <sys/types.h>
#include <sys/socket.h>
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

enum MsgType {
    DISCOVER = 1,
    IAM = 2,
    KEEPALIVE = 3,
    AUDIO = 4,
    METADATA = 6
};

static std::vector<uint8_t> encodeType(MsgType type, std::vector<uint8_t> const& buf) {
    std::size_t headerSize = 2*sizeof(uint16_t);
    assert(buf.size() < std::numeric_limits<uint16_t>::max());
    std::vector<uint8_t> output(buf.size() + headerSize);
    *reinterpret_cast<uint16_t*>(output.data()) = htons(static_cast<uint16_t>(type));
    *reinterpret_cast<uint16_t*>(output.data() + sizeof(uint16_t)) = htons(buf.size());
    return output;
}

void RadioSender::sendMusic(std::vector<uint8_t> const& music) {
    std::vector<uint8_t> toSend = encodeType(AUDIO, music);
}

void RadioSender::sendMetadata(std::vector<uint8_t> const& music) {
    std::vector<uint8_t> toSend = encodeType(METADATA, music);
}

void RadioSender::sendToAllClients(std::vector<uint8_t> const& buffer) {
    std::lock_guard<std::mutex> lock(clientsMutex);
}
