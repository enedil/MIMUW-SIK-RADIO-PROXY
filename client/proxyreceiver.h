#pragma once
#include "../common/message.h"
#include "message.h"

class TelnetServer;

class ProxyReceiver {
public:
    ProxyReceiver(int sockfd, TelnetServer* telnetServer, int timeout);
    ProxyReceiver(const ProxyReceiver&) = delete;
    void sendPipeMessage(PipeMessage const&& msg);
    ~ProxyReceiver();
private:
    void receivePipeMessage(PipeMessage& msg);
    int sockfd;
    int pipefd[2];
    int timeout;
    void loop();
    TelnetServer* telnetServer;
    void dispatchMessage(MessageType type, std::vector<uint8_t> const& data, sockaddr_in& address);
    void playMusic(std::vector<uint8_t> const& data);
};
