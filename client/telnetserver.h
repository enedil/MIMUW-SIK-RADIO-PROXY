#pragma once
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <exception>
#include <mutex>
#include <vector>
#include <memory>
#include <optional>
#include "keepalive.h"
#include "proxyreceiver.h"
#include "../common/socket.h"

struct Interrupt : std::exception {
    const char* what() const throw () {
        return "Interrupted";
    }
};

class TelnetServer {
public:
    struct Proxy {
        std::string name;
        sockaddr_in address;
    };
    TelnetServer(unsigned port, int timeout, sockaddr_in proxyAddr);
    void loop();
    void dropProxy();
    void METADATA(std::vector<uint8_t> const& metadata);
    void IAM(std::vector<uint8_t> const& name, sockaddr_in& address);
private:
    void discover(std::optional<sockaddr_in>&& recepient = std::nullopt);
    bool handleConnection(int sock);
    TcpSocket listenerSock;
    UdpSocket udpSock;
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
