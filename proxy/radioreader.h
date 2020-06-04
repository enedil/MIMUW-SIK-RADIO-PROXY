#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unistd.h>
#include <mutex>

enum ChunkType {
    ICY_AUDIO = STDOUT_FILENO,
    ICY_METADATA = STDERR_FILENO,
    END_OF_STREAM,
    ERROR
};

class RadioReader {
public:
    RadioReader(std::string& host, std::string& resource, std::string& port, unsigned timeout, bool metadata);
    bool init();
    std::string description();
    std::pair<ChunkType, const std::vector<uint8_t>&> readChunk();
    ~RadioReader();
private:
    int fd;
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
    void setTimeout();
    uint64_t metaint;
    size_t progress;
};
