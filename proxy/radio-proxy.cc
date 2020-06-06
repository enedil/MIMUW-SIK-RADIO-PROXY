#include "../common/error.h"
#include "args.h"
#include "radioreader.h"
#include "radiosender.h"
#include "stdoutsender.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <system_error>
#include <unistd.h>

int main(int argc, char *const argv[]) {
    int exit_code = EXIT_SUCCESS;
    try {
        ProxyArguments args(argc, argv);
        RadioReader reader(args.host, args.port, args.address, args.resource,
                           args.timeout, args.metadata);
        if (!reader.init()) {
            exit_code = EXIT_FAILURE;
        } else if (!args.udpport) {
            StdoutSender sender;
            exit_code = sender.sendrecvLoop(reader);
        } else {
            RadioSender sender(args.udpport.value(), args.udpaddr, args.udptimeout);
            exit_code = sender.sendrecvLoop(reader);
        }
    } catch (std::system_error &exc) {
        std::cerr << exc;
        exit_code = EXIT_FAILURE;
    } catch (std::exception &exc) {
        std::cerr << exc;
        exit_code = EXIT_FAILURE;
    }
    std::exit(exit_code);
}
