#include <iostream>
#include "stdoutsender.h"
#include "error.h"

int StdoutSender::sendrecvLoop(RadioReader& reader) {
    while (true) {
        auto [type, buf] = reader.readChunk();
        switch(type) {
            case ICY_AUDIO:
            case ICY_METADATA:
                if (write(type, buf.data(), buf.size()) != buf.size())
                    syserr("write");
                break;
            case INTERRUPT:
                return EXIT_SUCCESS;
        }
    }
}

