#pragma once
#include <netinet/in.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unistd.h>
#include <mutex>
#include "../common/socket.h"

// Represent received data type (or INTERRUPT, to signal Ctrl+C).
enum ChunkType {
    ICY_AUDIO = STDOUT_FILENO,
    ICY_METADATA = STDERR_FILENO,
    INTERRUPT
};

class RadioReader {
public:
    RadioReader(std::string const& host, std::string const& port, sockaddr_in const& address, std::string& resource, unsigned timeout, bool metadata);
    bool init();
    // Return description of current ICY radio station.
    std::string description();
    // Read next chunk of data from ICY radio.
    std::pair<ChunkType, std::vector<uint8_t>&> readChunk();
private:
    static constexpr size_t maxPacketSize = 0xffff - 4 /*SIKRadio*/ - 8 /*UDP*/ - 20 /*IP*/;
    // Socket for connection with ICY server.
    TcpSocket fd;
    std::vector<uint8_t> buffer;
    unsigned timeout;
    std::string host;
    std::string port;
    std::string resource;
    bool metadata;
    bool readHeaders(std::string& output);
    // Parse received headers.
    bool parseHeaders(const std::string& headers);
    // Parsed ICY options.
    std::unordered_map<std::string, std::string> icyOptions;
    // Sets timeout of type SO_SNDTIMEO or SO_RCVTIMEO.
    void setTimeout(int type);
    uint64_t metaint;
    size_t progress;
};
