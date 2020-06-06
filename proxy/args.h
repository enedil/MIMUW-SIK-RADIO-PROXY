#pragma once
#include <netinet/in.h>
#include <optional>
#include <string>

// Parser of command line arguments for proxy.
struct ProxyArguments {
    // Construct arguments basing on command line arguments.
    ProxyArguments(int argc, char *const argv[]);
    // Parsed address of ICY server.
    sockaddr_in address;
    // Host of ICY server.
    std::string host;
    // Port of ICY server.
    std::string port;
    // Resource on ICY server.
    std::string resource;
    // ICY server timeout.
    unsigned timeout = 5;
    // Shall accept metadata?
    bool metadata = false;
    // Optional listening multicast port.
    std::optional<unsigned> udpport = std::nullopt;
    // Optional listening multicast address.
    std::optional<std::string> udpaddr = std::nullopt;
    // Optional listening udp timeout.
    unsigned udptimeout = 5;
};
