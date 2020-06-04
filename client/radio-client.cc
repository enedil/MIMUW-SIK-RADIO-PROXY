#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "args.h"
#include "telnetserver.h"
#include "message.h"

int main(int argc, char* argv[]) {
    Args args(argc, argv);
    TelnetServer telnetServer(args.telnetPort, args.timeout, args.proxyAddr);
    telnetServer.loop();
}
