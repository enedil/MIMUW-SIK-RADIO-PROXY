#include <thread>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include "message.h"
#include "keepalive.h"
#include "../common/error.h"

static void loop(int sockfd, int pipefd) {
    PipeMessage msg = Command::NoAddress;
    pollfd fds;
    fds.fd = pipefd;
    while (true) {
        try {
            uint8_t dummy[1] = {};
            Message::sendMessage(sockfd, MessageType::KEEPALIVE, dummy, dummy, std::get<sockaddr_in>(msg));
        } catch (const std::bad_variant_access&) {
            if (std::get<Command>(msg) == Command::Stop) {
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

KeepAlive::~KeepAlive() {
    sendPipeMessage(Command::Stop);
    close(pipefd[0]);
    close(pipefd[1]);
}

void KeepAlive::sendPipeMessage(PipeMessage&& msg) {
    if (write(pipefd[1], &msg, sizeof(msg)) != sizeof(msg))
        syserr("write on pipe");
}
