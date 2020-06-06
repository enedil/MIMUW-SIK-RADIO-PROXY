#pragma once
#include <string>

// Cyclic buffer of fixed length, supporting checking if it ends with specific strings.
// Append operation pushes char at the end of the string and pops a char from the beginning.
class CyclicBuffer : public std::string {
public:
    // Cyclic buffer will have size given in the argument. Initially all bytes are zero.
    CyclicBuffer(size_t size);
    // Checks if the buffer ends with specific string.
    bool endsWith(std::string const& other) const;
    // Add a char at the end and pop one from the beginning.
    void append(char c);
private:
    size_t size;
};
