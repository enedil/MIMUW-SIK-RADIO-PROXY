#pragma once
#include <string>

struct ProxyArguments {
    ProxyArguments(int argc, char* const argv[]);
    std::string host;
    std::string port;
    std::string resource;
    unsigned timeout;
    bool metadata;
};
