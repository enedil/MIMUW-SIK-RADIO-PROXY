#include <iostream>
#include <thread>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include "message.h"
#include "keepalive.h"
#include "../common/message.h"
#include "../common/error.h"

static void loop(int sockfd, int pipefd) {
    // msg holds current state of operation, either address of current proxy, NoAddress (lack of current proxy), or Stop, 
    // which means stopping the thread.
    PipeMessage msg = Command::NoAddress;
    pollfd pollWatchedFd;
    pollWatchedFd.fd = pipefd;
    std::vector<uint8_t> dummy(0);
    Message message(sockfd, dummy);
    while (true) {
        // If there's some address, then send KEEPALIVE.
        try {
            // Address shall be nonnull.
            message.sendMessage(MessageType::KEEPALIVE, std::get<sockaddr_in>(msg));
        } catch (const std::bad_variant_access&) {
            if (std::get<Command>(msg) == Command::Stop)
                return;
        }
        pollWatchedFd.events = POLLIN;
        // Wait 3.5 seconds for a command. If no command comes in, send another KEEPALIVE.
        timespec ts{3, 500'000'000};
        int ret = ppoll(&pollWatchedFd, 1, &ts, NULL);
        if (ret < 0 && errno != EINTR)
            syserr("poll");
        else if (ret < 0 && errno == EINTR)
            continue;
        if (pollWatchedFd.revents & POLLIN) {
            if (read(pipefd, &msg, sizeof(msg)) != static_cast<ssize_t>(sizeof(msg)))
                syserr("reading PipeMessage failed");
        } else if (pollWatchedFd.revents & POLLERR) {
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
    // Instruct the thread to finish work.
    sendPipeMessage(Command::Stop);
    // Close both ends of the pipe. No syserr are used in destructors.
    if (close(pipefd[0]) != 0)
        std::cerr << "closing pipe[0] failed with code " << errno;
    if (close(pipefd[1]) != 0)
        std::cerr << "closing pipe[1] failed with code " << errno;
}

void KeepAlive::sendPipeMessage(PipeMessage&& msg) {
    if (write(pipefd[1], &msg, sizeof(msg)) != sizeof(msg))
        syserr("write on pipe");
}
