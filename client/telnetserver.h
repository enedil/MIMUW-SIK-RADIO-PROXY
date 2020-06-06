#pragma once
#include "../common/socket.h"
#include "keepalive.h"
#include "proxyreceiver.h"
#include <exception>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

struct Interrupt : std::exception {
    const char *what() const throw() { return "Interrupted"; }
};

class TelnetServer {
  public:
    // Holds information about a proxy.
    struct Proxy {
        std::string name;
        sockaddr_in address;
    };
    TelnetServer(unsigned port, int timeout, sockaddr_in proxyAddr);
    // Main loop handling telnet.
    void loop();
    // Remove current proxy, most probably due to timeout.
    void dropProxy();
    // Send new metadata.
    void METADATA(std::vector<uint8_t> const &metadata);
    // Send new available proxy.
    void IAM(std::vector<uint8_t> const &name, sockaddr_in &address);

  private:
    // Send discover packet.
    void discover(std::optional<sockaddr_in> &&recepient = std::nullopt);
    // Handle single telnet session.
    bool handleConnection(int sock);
    TcpSocket listenerSock;
    UdpSocket udpSock;
    sockaddr_in proxyAddr;

    std::mutex availableProxiesMutex;
    std::vector<Proxy> availableProxies;

    std::mutex currentProxyMutex;
    std::optional<Proxy *> currentProxy;

    std::mutex metadataMutex;
    std::string metadata;

    // Keepalive and proxyReceiver with whom we communicate.
    std::unique_ptr<KeepAlive> keepAlive;
    std::unique_ptr<ProxyReceiver> proxyReceiver;

    static std::string toString(std::vector<uint8_t> const &data);
};
