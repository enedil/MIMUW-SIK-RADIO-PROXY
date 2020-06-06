#include "cyclicbuffer.h"

CyclicBuffer::CyclicBuffer(size_t size) {
    resize(size);
}

bool CyclicBuffer::endsWith(std::string const& other) const {
    // If the buffer is shorter than the string we're testing against, it cannot end with the string.
    if (length() < other.length())
        return false;
    size_t difference = length() - other.length();
    for (size_t i = 0; i < other.length(); ++i) {
        if ((*this)[difference + i] != other[i])
            return false;
    }
    return true;
}

void CyclicBuffer::append(char c) {
    // Move each byte one position to the left.
    for (size_t i = 0; i + 1 < length(); ++i) {
        (*this)[i] = (*this)[i+1];
    }
    (*this)[length()-1] = c;
}
