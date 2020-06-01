#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <unistd.h>
#include <string_view>
#include <unordered_map>
#include "args.h"

using namespace std::literals;

ProxyArguments::ProxyArguments(int argc, char* const argv[]) : metadata(true), timeout(5) {
    int opt;
    std::unordered_map<int, bool> found;
    found[0] = true;
    while ((opt = getopt(argc, argv, "h:r:p:m:t:")) != -1) {
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
                if (sscanf(optarg, "%u", &timeout) != 1)
                    throw std::invalid_argument("timeout shall be a nonnegative integer");
                break;
            default:
                throw std::invalid_argument("provided argument is not known");
        }
    }
    for (int f : "hrp") {
        if (!found[f]) {
            throw std::invalid_argument("missing parameter");
        }
    }
}
