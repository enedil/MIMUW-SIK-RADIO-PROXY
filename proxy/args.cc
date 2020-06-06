#include "args.h"
#include <cctype>
#include <cstring>
#include <exception>
#include <iostream>
#include <netdb.h>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>

using namespace std::literals;

namespace {
// Perform getaddrinfo, with result checking.
sockaddr_in getAddress(const char *host, const char *port) {
    int status;
    addrinfo hints = {};
    addrinfo *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(host, port, &hints, &res)) != 0) {
        throw std::runtime_error{"getaddrinfo:"s + gai_strerror(status)};
    }
    sockaddr_in addr = *reinterpret_cast<sockaddr_in *>(res->ai_addr);
    freeaddrinfo(res);
    return addr;
}

// Convert string of digits into an unsigned number. Do not accept neither leading,
// nor trailing whitespace.
unsigned parseUnsigned(const char *str) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        if (!std::isdigit(str[i])) {
            throw std::invalid_argument("this isn't a number");
        }
    }
    unsigned u;
    if (sscanf(str, "%u", &u) != 1)
        throw std::invalid_argument("uncorrect number");
    return u;
}
} // namespace

ProxyArguments::ProxyArguments(int argc, char *const argv[]) {
    static std::unordered_map<char, const char *> long_names = {
        {'h', "ICY host"},      {'r', "resource"},    {'p', "ICY port"},
        {'m', "send metadata"}, {'t', "ICY timeout"}, {'P', "UDP listening port"},
        {'T', "client timeout"}};
    const char *host, *port;
    int opt;
    std::unordered_map<char, bool> found;
    found[0] = true;
    while ((opt = getopt(argc, argv, "h:r:p:m:t:P:B:T:")) != -1) {
        found[opt] = true;
        switch (opt) {
        case 'h':
            host = optarg;
            break;
        case 'r':
            resource = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        case 'm':
            if (optarg == "yes"sv)
                metadata = true;
            else if (optarg == "no"sv)
                metadata = false;
            else
                throw std::invalid_argument("-m yes|no");
            break;
        case 't':
            timeout = parseUnsigned(optarg);
            break;
        case 'P':
            udpport = parseUnsigned(optarg);
            if (udpport.value() == 0)
                throw std::invalid_argument("port shall not be zero");
            break;
        case 'B':
            udpaddr = optarg;
            break;
        case 'T':
            udptimeout = parseUnsigned(optarg);
            break;
        default:
            throw std::invalid_argument("provided argument is not known");
        }
    }
    for (char f : "hrp") {
        if (!found[f]) {
            throw std::invalid_argument("missing parameter: -"s + f + " (" +
                                        long_names[f] + ")");
        }
    }
    if (!found['P'] && (found['B'] || found['T'])) {
        throw std::invalid_argument("extraneous argument");
    }
    address = getAddress(host, port);
    this->host = host;
    this->port = port;
}
