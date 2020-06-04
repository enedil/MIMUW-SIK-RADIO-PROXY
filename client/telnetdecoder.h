#pragma once

enum class TelnetCommand {

};

using TelnetOutput = std::variant<char, TelnetCommand>;
