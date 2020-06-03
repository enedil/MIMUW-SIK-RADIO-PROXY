#pragma once
#include "message.h"

class KeepAlive {
public:
    KeepAlive(int sockfd);
    ~KeepAlive();
    void sendPipeMessage(PipeMessage&& msg);
private:
    int sockfd;
    int pipefd[2];
};
