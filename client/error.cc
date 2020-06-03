#include <system_error>
#include <errno.h>

void syserr(char const* reason) {
    throw std::system_error { std::error_code(errno, std::system_category()), reason };
}
