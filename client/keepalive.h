#pragma once
#include "message.h"

class KeepAlive {
public:
    KeepAlive(int sockfd);
    void sendPipeMessage(PipeMessage&& msg);
private:
    int sockfd;
    int pipefd[2];
};
