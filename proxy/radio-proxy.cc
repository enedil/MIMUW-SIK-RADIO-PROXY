#include <cassert>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include "args.h"
#include "radioreader.h"



int main(int argc, char* const argv[]) {
    ProxyArguments args(argc, argv);
    RadioReader reader(args.host, args.resource, args.port, args.timeout, args.metadata);
    if (!reader.init()) {
        return EXIT_FAILURE;
    }
    while (true) {
        auto p = reader.readChunk();
        ChunkType type = p.first;
        const std::vector<uint8_t>& buf = p.second;
        switch(type) {
        case AUDIO:
        case METADATA:
            break;
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
