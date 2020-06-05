#include <exception>
#include "error.h"

void syserr(const char* reason) {
    throw std::system_error { 
        std::error_code(errno, std::system_category()), 
        reason 
    };
}

std::ostream& operator<<(std::ostream& out, std::system_error const& err) {
    return out << "[category=" << err.code().category().name() <<
        ",code=" << err.code().value() << "]: " <<
        err.what() << "\n";
}

std::ostream& operator<<(std::ostream& out, std::exception const& err) {
    return out << "[category=?]: " << err.what() << "\n";
}
