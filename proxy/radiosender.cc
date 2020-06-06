#include <algorithm>
#include <iostream>
#include <cstring>
#include "../common/error.h"
#include "radiosender.h"

RadioSender::RadioSender(unsigned port, std::optional<std::string> multicastAddr, unsigned timeout) :
    timeout(timeout), isMulticasted(multicastAddr) {
        if (multicastAddr) {
            ip_mreq_.imr_interface.s_addr = htonl(INADDR_ANY);
            if (inet_aton(multicastAddr.value().c_str(), &ip_mreq_.imr_multiaddr) == 0)
                throw std::domain_error("inet_aton: invalid multicast adddress");
            sock.setSockOpt(IPPROTO_IP, IP_ADD_MEMBERSHIP, ip_mreq_);
            sock.setSockOpt(IPPROTO_IP, IP_MULTICAST_TTL, 16);
        }
        sockaddr_in local_address;
        local_address.sin_family = AF_INET;
        local_address.sin_addr.s_addr = htonl(INADDR_ANY);
        local_address.sin_port = htons(port);
        sock.bind(local_address);
}


RadioSender::~RadioSender() {
    if (isMulticasted)
        sock.setSockOpt(IPPROTO_IP, IP_DROP_MEMBERSHIP, ip_mreq_);
}


void RadioSender::sendData(MessageType type, std::vector<uint8_t>& buffer, std::optional<sockaddr_in> address) {
    Message message(sock, buffer);
    if (address)
        sendToAClient(message, type, address.value());
    else
        sendToAllClients(message, type);
}

void RadioSender::sendToAClient(Message& message, MessageType type, sockaddr_in address) {
    message.sendMessage(type, address);
}

void RadioSender::sendToAllClients(Message& message, MessageType type) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto clients_copy = clients;
    for (auto& client : clients) {
        auto now = clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - client.second);
        // If already exceeded timeout, drop client.
        if (diff.count() > timeout) {
            clients_copy.erase(client.first);
        } else {
            auto address = client.first;
            message.sendMessage(type, address);
        }
    }
    clients.swap(clients_copy);
}

int RadioSender::sendrecvLoop(RadioReader& reader) {
    std::thread controller {[&]() { this->controller(reader); }};
    controller.detach();
    int ret = -1;
    while (true) {
        auto [type, buf] = reader.readChunk();
        switch(type) {
        case ICY_AUDIO:
            // Don't forward empty messages.
            if (buf.size() > 0)
                sendData(AUDIO, buf, std::nullopt);
            break;
        case ICY_METADATA:
            // Don't forward empty messages.
            if (buf.size() > 0)
                sendData(METADATA, buf, std::nullopt);
            break;
        case INTERRUPT:
            return EXIT_SUCCESS;
            break;
        }
    }
    return ret;
}

void RadioSender::controller(RadioReader& reader) {
    sockaddr_in clientAddress;
    sockaddr* addrPtr = reinterpret_cast<sockaddr*>(&clientAddress);
    socklen_t sendToAClientLen = sizeof(clientAddress);
    while (true) {
        uint16_t type;
        ssize_t size = sizeof(type);
        if (recvfrom(sock, &type, size, 0, addrPtr, &sendToAClientLen) < size) {
            continue; // drop packet
        }
        type = ntohs(type);
        if (type == DISCOVER) {
            auto desc = reader.description();
            std::vector<uint8_t> buffer(desc.length());
            std::copy(desc.begin(), desc.end(), buffer.begin());
            try {
                sendData(IAM, buffer, clientAddress);
            } catch (std::system_error& exc) {
                std::cerr << exc;
            }
        }
        if (type == DISCOVER || type == KEEPALIVE) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            // Register client.
            if (type == DISCOVER)
                clients[clientAddress] = clock::now();
            // Update client, if was already registered.
            else {
                auto it = clients.find(clientAddress);
                if (it != clients.end())
                    it->second = clock::now();
            }
        }
    }
}
