#pragma once
#include <netinet/in.h>

// Command line arguments for client.
struct Args {
    // Construct Args from command line arguments.
    Args(int argc, char *argv[]);
    // Address of radio proxy.
    sockaddr_in proxyAddr;
    // Listening port for telnet.
    unsigned telnetPort;
    // Timeout value (in seconds) after which, nonresponding proxy is dropped (default
    // value is 5).
    unsigned timeout = 5;
};
