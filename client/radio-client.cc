#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "args.h"
#include "telnetserver.h"
#include "message.h"
/*
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(0);
    addr.sin_port = htons(5004);
    KeepAlive kal(sock);
    kal.sendPipeMessage(Command::NoAddress);
    sleep(5);
    kal.sendPipeMessage(addr);
    sleep(5);
    kal.sendPipeMessage(Command::Stop);
*/

int main(int argc, char* argv[]) {
    Args args(argc, argv);
    TelnetServer telnetServer(args.telnetPort, args.timeout, args.proxyAddr);
    telnetServer.loop();
}
