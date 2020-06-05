#include <iostream>
#include "args.h"
#include "telnetserver.h"
#include "message.h"

int main(int argc, char* argv[]) {
    int ret_val = EXIT_SUCCESS;
    try {
        Args args(argc, argv);
        TelnetServer telnetServer(args.telnetPort, args.timeout, args.proxyAddr);
        telnetServer.loop();
    } catch (Interrupt& exc) {
        std::cerr << "error: " << exc.what() << "\n";
    } catch (std::exception& exc) {
        std::cerr << "error: " << exc.what() << "\n";
        ret_val = EXIT_FAILURE;
    }
    return ret_val;
}
