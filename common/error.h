#pragma once
#include <exception>
#include <system_error>
#include <ostream>

void syserr(const char* reason);

std::ostream& operator<<(std::ostream& out, std::system_error const& err);
std::ostream& operator<<(std::ostream& out, std::exception const& err);
