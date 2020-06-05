#pragma once
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <climits>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <mutex>
#include <vector>
#include <thread>
#include "radioreader.h"
#include "../common/socket.h"

template<> struct std::hash<sockaddr_in> {
    std::size_t operator()(sockaddr_in const& addr) const {
        auto s_addr = addr.sin_addr.s_addr;
        auto port = addr.sin_port;
        static_assert(sizeof(s_addr) + sizeof(port) <= sizeof(std::size_t));
        return (s_addr << (CHAR_BIT * sizeof(port))) | port;
    }
};
template<> struct std::equal_to<sockaddr_in> {
    bool operator()(sockaddr_in const& lhs, sockaddr_in const& rhs) const {
        return lhs.sin_addr.s_addr == rhs.sin_addr.s_addr && lhs.sin_port == rhs.sin_port;
    }
};

enum MsgType {
    DISCOVER = 1,
    IAM = 2,
    KEEPALIVE = 3,
    AUDIO = 4,
    METADATA = 6
};

class RadioSender {
    using clock = std::chrono::system_clock;
    using time_point = clock::time_point;
public:
    RadioSender(unsigned port, std::optional<std::string> broadcastAddr, unsigned timeout);
    int sendrecvLoop(RadioReader& reader);
    ~RadioSender();
private:
    ip_mreq ip_mreq_;
    unsigned timeout;
    UdpSocket sock;
    bool isBroadcasted;
    std::mutex clientsMutex;
    std::unordered_map<sockaddr_in, time_point> clients;
    void controller(RadioReader& reader);
    void sendToAClient(msghdr msghdr, sockaddr_in address);
    void sendToAllClients(msghdr msghdr);
    void sendData(MsgType type, std::vector<uint8_t> const& data, std::optional<sockaddr_in> address);
    void sendData(MsgType type, uint8_t const* begin, uint8_t const* end, std::optional<sockaddr_in> address);
};
