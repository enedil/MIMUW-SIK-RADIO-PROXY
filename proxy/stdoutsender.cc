#include <iostream>
#include "stdoutsender.h"
#include "../common/error.h"

int StdoutSender::sendrecvLoop(RadioReader& reader) {
    ssize_t ret;
    size_t totalWritten;
    while (true) {
        auto [type, buf] = reader.readChunk();
        switch(type) {
            case ICY_AUDIO:
            case ICY_METADATA:
                totalWritten = 0;
                while (totalWritten < buf.size()) {
                    // ICY_AUDIO and ICY_METADATA are set correspondingly to constants
                    // STDOUT_FILENO and STDERR_FILENO.
                    ret = write(type, buf.data() + totalWritten, buf.size() - totalWritten);
                    if (ret < 0)
                        syserr("write");
                    totalWritten += static_cast<size_t>(ret);
                }
                break;
            case INTERRUPT:
                return EXIT_SUCCESS;
        }
    }
}

