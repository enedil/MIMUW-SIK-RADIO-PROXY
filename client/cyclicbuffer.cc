#include "cyclicbuffer.h"

CyclicBuffer::CyclicBuffer(size_t size) {
    resize(size);
}

bool CyclicBuffer::endsWith(std::string const& other) const {
    if (length() < other.length())
        return false;
    for (size_t i = 0; i < other.length(); ++i) {
        if ((*this)[length()-i-1] != other[other.size()-i-1])
            return false;
    }
    return true;
}

void CyclicBuffer::append(char c) {
    for (size_t i = 0; i + 1 < length(); ++i) {
        (*this)[i] = (*this)[i+1];
    }
    (*this)[length()-1] = c;
}
