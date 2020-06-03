#include <thread>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include "message.h"
#include "keepalive.h"
#include "error.h"
#include <iostream>

static void loop(int sockfd, int pipefd) {
    PipeMessage msg = Command::NoAddress;
    pollfd fds;
    fds.fd = pipefd;
    while (true) {
        try {
            uint8_t dummydata[1] = {};
            Message::sendMessage(sockfd, MessageType::KEEPALIVE, dummydata, dummydata, std::get<sockaddr_in>(msg));
            std::cout << "has address\n";
        } catch (const std::bad_variant_access&) {
            std::cout << "has command\n";
            if (std::get<Command>(msg) == Command::Stop) {
                std::cout << "stop\n";
                return;
            }
        }
        fds.events = POLLIN;
        fds.revents = 0;
        timespec ts{3, 500'000'000};
        int ret = ppoll(&fds, 1, &ts, NULL);
        if (ret < 0 && errno != EINTR)
            syserr("poll");
        else if (ret < 0 && errno == EINTR)
            continue;
        if (fds.revents & POLLIN) {
            if (read(pipefd, &msg, sizeof(msg)) < static_cast<ssize_t>(sizeof(msg)))
                syserr("reading PipeMessage failed");
            std::cout << "received data\n";
        } else if (fds.revents & POLLERR) {
            syserr("POLLERR");
        }
    }
}

KeepAlive::KeepAlive(int sockfd) {
    if (pipe(pipefd) != 0)
        syserr("pipe");
    std::thread t { loop, sockfd, pipefd[0] };
    t.detach();
}

void KeepAlive::sendPipeMessage(PipeMessage&& msg) {
    if (write(pipefd[1], &msg, sizeof(msg)) != sizeof(msg))
        syserr("write on pipe");
}
