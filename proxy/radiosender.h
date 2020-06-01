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

namespace std {
    template<> struct hash<sockaddr_in> {
        std::size_t operator()(sockaddr_in const& addr) {
            auto s_addr = addr.sin_addr.s_addr;
            auto port = addr.sin_port;
            static_assert(sizeof(s_addr) + sizeof(port) <= sizeof(std::size_t));
            return (s_addr << (CHAR_BIT * sizeof(port))) | port;
        }
    };
}

class RadioSender {
    using time_point = std::chrono::system_clock::time_point;
    using Buffer = std::vector<uint8_t>;
public:
    RadioSender(unsigned port, std::optional<std::string> broadcastAddr, unsigned timeout);
    ~RadioSender();
    void sendMusic(Buffer const& data);
    void sendMetadata(Buffer const& data);
private:
    ip_mreq ip_mreq;
    unsigned timeout;
    int sock;
    bool isBroadcasted;
    std::mutex clientsMutex;
    std::unordered_map<sockaddr_in, time_point> clients;
    std::thread controllerThread;
    void sendToAllClients(Buffer const& buffer);
};
