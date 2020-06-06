#pragma once
#include <netinet/in.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unistd.h>
#include <mutex>
#include "../common/socket.h"

enum ChunkType {
    ICY_AUDIO = STDOUT_FILENO,
    ICY_METADATA = STDERR_FILENO,
    INTERRUPT
};

class RadioReader {
public:
    RadioReader(std::string const& host, std::string const& port, sockaddr_in const& address, std::string& resource, unsigned timeout, bool metadata);
    bool init();
    std::string description();
    std::pair<ChunkType, const std::vector<uint8_t>&> readChunk();
private:
    static constexpr size_t maxPacketSize = 0xffff - 4 /*SIKRadio*/ - 8 /*UDP*/ - 20 /*IP*/;
    TcpSocket fd;
    std::vector<uint8_t> buffer;
    unsigned timeout;
    std::string host;
    std::string port;
    std::string resource;
    bool metadata;
    bool sendAll(const std::string& data);
    int readAll(std::vector<uint8_t>& data);
    bool readHeaders(std::string& output);
    bool parseHeaders(const std::string& headers);
    std::unordered_map<std::string, std::string> icyOptions;
    void setTimeout(int type);
    uint64_t metaint;
    size_t progress;
};
