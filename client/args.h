#pragma once
#include <netinet/in.h>

struct Args {
    Args(int argc, char* argv[]);
    sockaddr_in proxyAddr;
    unsigned telnetPort;
    unsigned timeout;
};
