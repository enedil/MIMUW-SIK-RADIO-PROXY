#pragma once
#include <netinet/in.h>
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include <optional>
#include "keepalive.h"
#include "proxyreceiver.h"


class TelnetServer {
    struct Proxy {
        std::string name;
        sockaddr_in address;
    };
public:
    TelnetServer(unsigned port);
    ~TelnetServer();
    void loop();
    void dropProxy();
    void METADATA(std::vector<uint8_t> const& metadata);
    void IAM(std::vector<uint8_t> const& name, sockaddr_in& address);
private:
    bool handleConnection(int sock);
    int listenerSock;
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
