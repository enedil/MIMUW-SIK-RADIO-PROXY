#pragma once
#include <netinet/in.h>
#include <climits>
#include <vector>
#include <variant>

enum class Command { NoAddress, Stop };
using PipeMessage = std::variant<sockaddr_in, Command>;
static_assert(sizeof(PipeMessage) <= PIPE_BUF);

enum MessageType {
    DISCOVER = 1,
    IAM = 2,
    KEEPALIVE = 3,
    AUDIO = 4,
    METADATA = 6
};

namespace Message {
    void sendMessage(int sockfd, MessageType type, uint8_t* begin, uint8_t* end, sockaddr_in& destination);
    void recvMessage(int sockfd, MessageType& type, std::vector<uint8_t>& data, sockaddr_in& source);
}
