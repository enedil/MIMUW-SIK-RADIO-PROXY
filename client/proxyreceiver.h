#pragma once
#include "message.h"
#include "telnetserver.h"

class ProxyReceiver {
public:
    ProxyReceiver(int sockfd, TelnetServer& telnetServer);
    void sendPipeMessage(PipeMessage&& msg);
    ~ProxyReceiver();
private:
    int sockfd;
    int pipefd[2];
    void loop();
    TelnetServer& telnetServer;
};
