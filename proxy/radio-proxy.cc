#include <cassert>
#include <exception>
#include <iostream>
#include <cstdlib>
#include <system_error>
#include <cstring>
#include <unistd.h>
#include "args.h"
#include "error.h"
#include "radioreader.h"
#include "radiosender.h"

[[noreturn]]
static void partA(RadioReader& reader) {
    while (true) {
        auto p = reader.readChunk();
        ChunkType type = p.first;
        const std::vector<uint8_t>& buf = p.second;
        switch(type) {
            case ICY_AUDIO:
            case ICY_METADATA:
                if (write(type, buf.data(), buf.size()) != buf.size()) {
                    exit(EXIT_FAILURE);
                }
                break;
            case END_OF_STREAM:
                exit(EXIT_SUCCESS);
            case ERROR:
                exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* const argv[]) {
    try {
        ProxyArguments args(argc, argv);
        RadioReader reader(args.host, args.port, args.address, args.resource, args.timeout, args.metadata);
        if (!reader.init()) {
            return EXIT_FAILURE;
        }
        if (!args.udpport) {
            partA(reader);
        } else {
            RadioSender sender(args.udpport.value(), args.udpaddr, args.udptimeout);
            return sender.sendrecvLoop(reader);
        }
    } catch (std::system_error& exc) {
        std::cerr << exc;
        return EXIT_FAILURE;
    } catch (std::exception& exc) {
        std::cerr << exc;
        return EXIT_FAILURE;
    }
}
