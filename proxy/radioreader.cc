#include "radioreader.h"
#include <cstring>
#include "../common/error.h"
#include <signal.h>
#include <sstream>
#include <iostream>
#include <stdexcept>

static volatile sig_atomic_t interrupt_occured = 0;

namespace {
struct FatalCondition : public std::exception {
    const char *reason;
    FatalCondition(const char *reason) : reason(reason) {}
    const char *what() const throw() { return reason; }
};

void interrupt_handler([[maybe_unused]] int signo) { interrupt_occured = 1; }
} // namespace

RadioReader::RadioReader(std::string const &host, std::string const &port,
                         sockaddr_in const &address, std::string &resource,
                         unsigned timeout, bool metadata)
    : timeout(timeout), metadata(metadata), host(host), port(port), resource(resource),
      progress(0) {
    if (signal(SIGINT, interrupt_handler) == SIG_ERR) {
        syserr("signal");
    }
    setTimeout(SO_SNDTIMEO);
    int status =
        connect(fd, reinterpret_cast<const sockaddr *>(&address), sizeof(address));
    if (status != 0) {
        syserr("connect");
    }
}

bool RadioReader::init() {
    std::string query = "GET " + resource +
                        " HTTP/1.0\r\n"
                        "Host: " +
                        host + ":" + port +
                        "\r\n"
                        "User-Agent: SIK Proxy\r\n"
                        "Accept: */*\r\n"
                        "Icy-Metadata: " +
                        (metadata ? "1" : "0") + "\r\n\r\n";
    fd.sendAll(query.c_str(), query.length());
    std::string headers = "";
    if (!readHeaders(headers)) {
        return false;
    }
    return parseHeaders(headers);
}

bool RadioReader::readHeaders(std::string &output) {
    output = "";
    char c;
    setTimeout(SO_RCVTIMEO);
    while (read(fd, &c, 1) == 1) {
        output += c;
        if (output.length() >= 4 && output.substr(output.length() - 4, 4) == "\r\n\r\n") {
            output.resize(output.length() - 2);
            return true;
        }
    }
    return false;
}

void RadioReader::setTimeout(int type) {
    if (type != SO_SNDTIMEO && type != SO_RCVTIMEO)
        throw std::invalid_argument("type != SO_SNDTIMEO && type != SO_RCVTIMEO");
    timeval ts;
    ts.tv_usec = 0;
    ts.tv_sec = timeout;
    fd.setSockOpt(SOL_SOCKET, type, ts);
}

bool RadioReader::parseHeaders(const std::string &headers) {
    std::stringstream s(headers);
    std::string line;
    if (!std::getline(s, line))
        return false;
    if (line != "ICY 200 OK\r" && line != "HTTP/1.0 200 OK\r" &&
        line != "HTTP/1.1 200 OK\r")
        return false;
    while (std::getline(s, line)) {
        if (line[line.length() - 1] == '\r')
            line.resize(line.length() - 1);
        auto j = line.find(':');
        if (j == std::string::npos)
            continue;
        if (line.length() <= j + 1)
            continue;
        if (line.substr(0, 4) != "icy-")
            continue;
        icyOptions[line.substr(0, j)] = line.substr(j + 1, line.length());
    }
    auto icymetaint = icyOptions.find("icy-metaint");
    if (icymetaint != icyOptions.end() and !metadata) {
        return false;
    } else if (icymetaint != icyOptions.end()) {
        metaint = std::stoul(icymetaint->second);
    } else {
        metaint = 1e9; // Some big number.
    }
    return true;
}

std::string RadioReader::description() {
    return icyOptions["icy-description"] + ", " + icyOptions["icy-name"];
}

std::pair<ChunkType, std::vector<uint8_t> &> RadioReader::readChunk() {
    std::pair<ChunkType, std::vector<uint8_t> &> ret = {INTERRUPT, buffer};
    try {
        if (interrupt_occured) {
            return ret;
        }
        // We finished chunk of data. Now it's time for metadata (skip metadata if not
        // declared to come).
        if (progress == metaint) {
            progress = 0;
            if (metadata) {
                ret.first = ICY_METADATA;
                uint8_t metadataLength = 0;
                ssize_t r = read(fd, &metadataLength, sizeof(metadataLength));
                if (r < 0)
                    throw FatalCondition("read");
                if (r != sizeof(metadataLength))
                    throw FatalCondition("short read of metadata");
                else {
                    buffer.resize(16 * static_cast<size_t>(metadataLength));
                    try {
                        fd.readAll(reinterpret_cast<char *>(buffer.data()),
                                   buffer.size());
                    } catch (...) {
                        throw FatalCondition("short read");
                    }
                }
                return ret;
            }
        }
        buffer.resize(std::min(metaint - progress, maxPacketSize));
        setTimeout(SO_RCVTIMEO);
        ssize_t sz = read(fd, buffer.data(), buffer.size());
        if (sz < 0) {
            throw FatalCondition("short or bad read");
        }
        progress += static_cast<size_t>(sz);
        buffer.resize(static_cast<size_t>(sz));
        ret.first = ICY_AUDIO;
        return ret;
    } catch (FatalCondition &exc) {
        if (interrupt_occured)
            return ret;
        throw;
    }
}
