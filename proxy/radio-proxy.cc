#include <cassert>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include "args.h"
#include "radioreader.h"
#include "radiosender.h"

[[noreturn]]
static void partA(RadioReader& reader) {
    while (true) {
        auto p = reader.readChunk();
        ChunkType type = p.first;
        const std::vector<uint8_t>& buf = p.second;
        switch(type) {
        case AUDIO:
        case METADATA:
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

static void partB(RadioReader& reader, RadioSender& sender) {
}

int main(int argc, char* const argv[]) {
    ProxyArguments args(argc, argv);
    RadioReader reader(args.host, args.resource, args.port, args.timeout, args.metadata);
    if (!reader.init()) {
        return EXIT_FAILURE;
    }
    if (!args.udpport) {
        partA(reader);
    } else {
        RadioSender sender(args.udpport.value(), args.udpaddr, args.udptimeout);
        //partB(reader);
    }
}
