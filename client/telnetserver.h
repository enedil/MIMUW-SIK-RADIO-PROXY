#pragma once
#include <netinet/in.h>
#include <string>
#include <vector>
#include <optional>


class TelnetServer {
public:
    TelnetServer(unsigned port);
    ~TelnetServer();
    void loop();
private:
    struct Proxy {
        std::string name;
        sockaddr_in address;
    };
    bool handleConnection(int sock);
    int listenerSock;
    std::vector<Proxy> availableProxies;
    std::optional<Proxy*> currentProxy;
    void startKeepalive();

    int keepAlivePipe;
    int receiverPipe;
};
