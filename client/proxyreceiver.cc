#include <thread>
#include <unistd.h>
#include <poll.h>
#include "proxyreceiver.h"
#include "error.h"

ProxyReceiver::ProxyReceiver(int sockfd, TelnetServer& telnetServer) : 
    telnetServer(telnetServer), sockfd(sockfd) {
    if (pipe(pipefd) != 0)
        syserr("pipe");
    std::thread t { &ProxyReceiver::loop, this };
    t.detach();
}


void ProxyReceiver::sendPipeMessage(PipeMessage&& msg) {
    if (write(pipefd[1], &msg, sizeof(msg)) != sizeof(msg))
        syserr("write on pipe");
}

void ProxyReceiver::loop() {
    PipeMessage msg;
    while (true) {

    }
}

ProxyReceiver::~ProxyReceiver() {
    sendPipeMessage(Command::Stop);
    close(pipefd[0]);
    close(pipefd[1]);
}
