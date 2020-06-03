#include <cstring>
#include <getopt.h>
#include <netdb.h>
#include <unordered_map>
#include <system_error>
#include "args.h"

Args::Args(int argc, char* argv[]) : timeout(5) {
    int opt;
    char * host, *port;
    std::unordered_map<int, bool> seen;
    seen[0] = true;
    while ((opt = getopt(argc, argv, "H:P:p:t:")) != -1) {
        seen[opt] = true;
        switch(opt) {
        case 'H':
            host = optarg;
            break;
        case 'P':
            port = optarg;
            break;
        case 'p':
            if (sscanf(optarg, "%u", &telnetPort) != 1 || telnetPort == 0)
                throw std::invalid_argument("port shall be a positive integer");
            break;
        case 't':
            if (sscanf(optarg, "%u", &timeout) != 1)
                throw std::invalid_argument("timeout shall be a nonnegative integer");
            break;
        }
    }
    for (auto c : "HPp") {
        if (!seen[c])
            throw std::invalid_argument("missing parameter");
    }
    std::memset(&proxyAddr, 0, sizeof(proxyAddr));
    int status;
    addrinfo hints = {};
    addrinfo* res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (0 != (status = getaddrinfo(host, port, &hints, &res))) {
        throw std::system_error { std::error_code(status, std::system_category()), gai_strerror(status) };
    }
    std::memcpy(&proxyAddr, reinterpret_cast<sockaddr_in*>(res->ai_addr), sizeof(proxyAddr));
    freeaddrinfo(res);
}
