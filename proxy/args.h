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
    unsigned timeout;
    bool metadata;
    std::optional<unsigned> udpport;
    std::optional<std::string> udpaddr;
    unsigned udptimeout;
};
