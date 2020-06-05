#pragma once
#include <string>

class CyclicBuffer : public std::string {
public:
    CyclicBuffer(size_t size);
    bool endsWith(std::string const& other) const;
    void append(char c);
private:
    size_t size;
};
