#pragma once
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include <optional>
#include "keepalive.h"
#include "proxyreceiver.h"


class TelnetServer {
public:
    struct Proxy {
        std::string name;
        sockaddr_in address;
        friend
        constexpr bool operator==(const Proxy& lhs, const Proxy& rhs) {
            return lhs.address == rhs.address && lhs.name == rhs.name;
        }
    };
    TelnetServer(unsigned port, int timeout, sockaddr_in proxyAddr);
    ~TelnetServer();
    void loop();
    void dropProxy();
    void METADATA(std::vector<uint8_t> const& metadata);
    void IAM(std::vector<uint8_t> const& name, sockaddr_in& address);
private:
    void discover(std::optional<sockaddr_in>&& recepient = std::nullopt);
    bool handleConnection(int sock);
    int listenerSock;
    int udpSock;
    sockaddr_in proxyAddr;

    std::mutex availableProxiesMutex;
    std::vector<Proxy> availableProxies;
    std::mutex currentProxyMutex;
    std::optional<Proxy*> currentProxy;

    std::mutex metadataMutex;
    std::string metadata; 

    std::unique_ptr<KeepAlive> keepAlive;
    std::unique_ptr<ProxyReceiver> proxyReceiver;

    static std::string toString(std::vector<uint8_t> const& data);
};
