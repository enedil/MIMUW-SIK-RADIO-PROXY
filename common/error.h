#pragma once
#include <exception>
#include <ostream>
#include <system_error>

// Throws std::system_error with appropriate message.
void syserr(const char *reason);

// This enables std::cout << exc, with more verbose error messages.
std::ostream &operator<<(std::ostream &out, std::system_error const &err);
std::ostream &operator<<(std::ostream &out, std::exception const &err);
