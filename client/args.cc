#include <cstring>
#include <getopt.h>
#include <netdb.h>
#include <unordered_map>
#include <system_error>
#include "args.h"

using namespace std::literals;

Args::Args(int argc, char* argv[]) {
    // getopt option.
    int opt;
    const char* host = NULL, *port = NULL;
    // Mark seen options.
    std::unordered_map<int, bool> seen;
    seen[0] = true;
    while ((opt = getopt(argc, argv, "H:P:p:T:")) != -1) {
        seen[opt] = true;
        switch(opt) {
        case 'H':
            // Proxy host.
            host = optarg;
            break;
        case 'P':
            // Proxy port. To be used in getaddrinfo, that's why it's not parsed into a number.
            port = optarg;
            break;
        case 'p':
            // Telnet listening port.
            if (sscanf(optarg, "%u", &telnetPort) != 1 || telnetPort == 0)
                throw std::invalid_argument("port shall be a positive integer");
            break;
        case 'T':
            // Timeout for proxy connection.
            if (sscanf(optarg, "%u", &timeout) != 1)
                throw std::invalid_argument("timeout shall be a nonnegative integer");
            break;
        default:
            throw std::invalid_argument("invalid argument: ");
        }
    }
    for (auto c : "HPp") {
        if (!seen[c])
            throw std::invalid_argument("missing parameter");
    }
    int status;
    addrinfo hints = {};
    addrinfo* res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (0 != (status = getaddrinfo(host, port, &hints, &res))) {
        throw std::runtime_error { "getaddrinfo:"s + gai_strerror(status) };
    }
    proxyAddr = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
    freeaddrinfo(res);
}
