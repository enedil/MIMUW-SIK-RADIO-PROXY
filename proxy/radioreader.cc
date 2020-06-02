#include <netinet/in.h>
#include <utility>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <regex>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "radioreader.h"

static volatile sig_atomic_t interrupt_occured = 0;

static void interrupt_handler(int signo) {
    interrupt_occured = 1;
}

RadioReader::RadioReader(std::string& host, std::string& resource, std::string& port, unsigned timeout, bool metadata) :
    timeout(timeout), metadata(metadata), host(host), port(port), resource(resource), progress(0) {
        int status;
        addrinfo hints = {};
        addrinfo* res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (0 != (status = getaddrinfo(host.c_str(), port.c_str(), &hints, &res))) {
            throw std::system_error { std::error_code(status, std::system_category()), gai_strerror(status) };
        }

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            freeaddrinfo(res);
            throw std::system_error { std::error_code(errno, std::system_category()), "socket" };
        }

        status = connect(fd, res->ai_addr, sizeof(sockaddr_in));
        freeaddrinfo(res);
        if (status != 0) {
            close(fd);
            throw std::system_error { std::error_code(errno, std::system_category()), "connect" };
        }
}

bool RadioReader::init() {
    if (signal(SIGINT, interrupt_handler) == SIG_ERR) {
        return false;
    }
    std::string query =
        "GET " + resource + " HTTP/1.0\r\n" 
        "Host: " + host + ":" + port + "\r\n" 
        "User-Agent: SIK Proxy\r\n"
        "Accept: */*\r\n"
        "Icy-Metadata: " + (metadata ? "1" : "0") + "\r\n\r\n";
    if (!sendAll(query)) {
        return false;
    }
    std::string headers = "";
    if (!readHeaders(headers)) {
        return false;
    }
    return parseHeaders(headers);
}


bool RadioReader::readHeaders(std::string& output) {
    output = "";
    char c;
    setTimeout();
    while (read(fd, &c, 1) == 1) {
        output += c;
        if (output.length() >= 4 && output.substr(output.length() - 4, 4) == "\r\n\r\n") {
            output.resize(output.length()-2);
            return true;
        }
    }
    return false;
    //throw std::system_error { std::error_code(errno, std::system_category()), "incomplete read" };
}

void RadioReader::setTimeout() {
    timeval ts;
    ts.tv_usec = 0;
    ts.tv_sec = timeout;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &ts, sizeof(ts)) != 0) {
        throw std::system_error { std::error_code(errno, std::system_category()), "setsockopt" };
    }
}

bool RadioReader::parseHeaders(const std::string& headers) {
    std::regex line_regex("([^(\\r\\n)]*)\\r\\n");
    auto lines_begin = 
        std::sregex_token_iterator(headers.begin(), headers.end(), line_regex, 1);
    auto lines_end = std::sregex_token_iterator();

    for (auto i = lines_begin; i != lines_end; i++) {
        std::string line = *i;
        if (i == lines_begin) {
            if (line != "ICY 200 OK" && line != "HTTP/1.0 200 OK" && line != "HTTP/1.1 200 OK")
                return false;
        }
        auto j = line.find(':');
        if (j == std::string::npos)
            continue;
        if (line.length() <= j+1)
            continue;
        if (line.substr(0, 4) != "icy-")
            continue;
        icyOptions[line.substr(0, j)] = line.substr(j+1, line.length());
        std::cout << line << "\n";
    }
    auto icymetaint = icyOptions.find("icy-metaint");
    if (icymetaint != icyOptions.end() and !metadata) {
        return false;
    } else if (icymetaint != icyOptions.end()) {
        metaint = std::stoul(icymetaint->second);
    } else {
        metaint = (1<<16)-500; //todo change magic
    }
    return true;
}

std::string RadioReader::description() {
    return icyOptions["icy-description"] + ", " + icyOptions["icy-name"];
}


std::pair<ChunkType, const std::vector<uint8_t>&> RadioReader::readChunk() {
    decltype(readChunk()) ret = {END_OF_STREAM, buffer};
    if (interrupt_occured) {
        return ret;
    }
    if (progress == metaint) {
        progress = 0;
        if (metadata) {
            ret.first = ICY_METADATA;
            uint8_t metadataLength = 0;
            auto r = read(fd, &metadataLength, sizeof(metadataLength));
            if (r < 0 && errno != EINTR)
                ret.first = ERROR;
            if (r != 1)
                ret.first = END_OF_STREAM;
            else {
                buffer.resize(16 * static_cast<size_t>(metadataLength));
                r = readAll(buffer);
                if (!r) {
                    ret.first = interrupt_occured ? END_OF_STREAM : ERROR;
                }
            }
            return ret;
        }
    }
    buffer.resize(metaint - progress);
    setTimeout();
    auto sz = read(fd, buffer.data(), metaint - progress);
    if (sz == 0) {
        ret.first = END_OF_STREAM;
        return ret;
    }
    if (sz < 0 && errno != EAGAIN && !interrupt_occured) {
        ret.first = ERROR;
        return ret;
    } else if (sz < 0) {
        ret.first = END_OF_STREAM;
        return ret;
    }
    progress += static_cast<size_t>(sz);
    buffer.resize(static_cast<size_t>(sz));
    ret.first = ICY_AUDIO;
    return ret;
}

int RadioReader::readAll(std::vector<uint8_t>& data) {
    uint8_t* start = data.data();
    uint8_t* end = data.data() + data.size();
    while (start < end) {
        auto r = read(fd, start, end-start);
        if (r <= 0)
            return false;
        start += r;
    }
    return true;
}

bool RadioReader::sendAll(const std::string& data) {
    size_t size = data.size();
    const char* ptr = data.c_str();
    while (size > 0) {
        auto sent = write(fd, ptr, size);
        if (sent <= 0) {
            return false;
            //throw std::system_error { std::error_code(errno, std::system_category()), "write" };
        }
        size -= static_cast<decltype(size)>(sent);
        ptr += sent;
    }
    return true;
}

RadioReader::~RadioReader() {
    close(fd);
}
