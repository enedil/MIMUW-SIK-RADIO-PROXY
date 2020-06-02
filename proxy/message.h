#include <vector>
#include <cstdint>
#include <arpa/inet.h>
enum MessageType {
    DISCOVER = 1;
    IAM = 2;
    KEEPALIVE = 3;
    AUDIO = 4;
    METADATA = 5;
};

struct MessageHeader {
    MessageType type;
    uint16_t len;
    MessageHeader() : type(), len() {};
    std::vector<uint8_t> serialize() {
        std::vector<uint8_t> ret(4);
        uint16_t stype = htons(type);
        uint16_t slen = htons(len);
    }
};
