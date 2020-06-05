#pragma once
#include <netinet/in.h>
#include <string>
#include <optional>

struct ProxyArguments {
    ProxyArguments(int argc, char* const argv[]);
    sockaddr_in address;
    std::string host;
    std::string port;
    std::string resource;
    unsigned timeout = 5;
    bool metadata = false;
    std::optional<unsigned> udpport = std::nullopt;
    std::optional<std::string> udpaddr = std::nullopt;
    unsigned udptimeout = 5;
};
