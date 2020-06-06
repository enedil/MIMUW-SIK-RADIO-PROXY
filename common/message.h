#pragma once
#include <array>
#include <netinet/in.h>
#include <vector>

// Message types for Client-Proxy protocol.
enum MessageType { DISCOVER = 1, IAM = 2, KEEPALIVE = 3, AUDIO = 4, METADATA = 6 };

// Message class for sending and receiving messages from Client-Proxy protocol.
class Message {
  public:
    // filedes is the udp socket file descriptor.
    Message(int filedes, std::vector<uint8_t> &buffer);
    // Sends data in buffer as type, to destination.
    void sendMessage(MessageType type, sockaddr_in &destination);
    // Receive data into buffer, store type in type and source in source.
    void recvMessage(MessageType &type, sockaddr_in &source);

  private:
    int filedes;
    std::array<uint16_t, 2> header;
    std::array<iovec, 2> scattergather;
    std::vector<uint8_t> &buffer;
    msghdr hdr;
};
