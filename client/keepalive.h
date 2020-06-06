#pragma once
#include "message.h"

// This class sends regularily KEEPALIVE messages to currently held proxy.
class KeepAlive {
public:
    // Constructs class with a socket for sending messages.
    KeepAlive(int sockfd);
    ~KeepAlive();
    // Make the keepalive thread receive a message from the main thread.
    void sendPipeMessage(PipeMessage&& msg);
private:
    // UDP socket for sending messages to the proxy.
    int sockfd;
    // Pipe used for internal communication.
    int pipefd[2];
};
