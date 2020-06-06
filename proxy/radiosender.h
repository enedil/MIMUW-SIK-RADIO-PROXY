#pragma once
#include "../common/address.h"
#include "../common/message.h"
#include "../common/socket.h"
#include "radioreader.h"
#include <arpa/inet.h>
#include <chrono>
#include <climits>
#include <mutex>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Sends music and metadata over UDP to connected clients.
class RadioSender {
    // Types for representing last seen times for clients.
    using clock = std::chrono::system_clock;
    using time_point = clock::time_point;

  public:
    RadioSender(unsigned port, std::optional<std::string> multicastAddr,
                unsigned timeout);
    int sendrecvLoop(RadioReader &reader);
    ~RadioSender();

  private:
    // Shall we listen on multicast addres?
    bool isMulticasted;
    // Holds information about multicast group.
    ip_mreq ip_mreq_;
    unsigned timeout;
    // Socket for sending music and metadata.
    UdpSocket sock;
    // Mutex for clients map.
    std::mutex clientsMutex;
    // Registered clients: for each address we store the last time we've seen the client.
    std::unordered_map<sockaddr_in, time_point> clients;
    // Run controlling thread, which receives possible messages, i.e. DISCOVER and
    // KEEPALIVE.
    void controller(RadioReader &reader);
    // Send a message (as defined by Client-Proxy protocol) to a client.
    void sendToAClient(Message &message, MessageType type, sockaddr_in address);
    // Send a message (as defined by Client-Proxy protocol) to all registered clients.
    void sendToAllClients(Message &message, MessageType type);
    void sendData(MessageType type, std::vector<uint8_t> &data,
                  std::optional<sockaddr_in> address);
};
