#pragma once
#include <netinet/in.h>
#include <climits>
#include <vector>
#include <variant>

// Commands that can be sent to Keepalive thread and Receiver thread.
enum class Command { NoAddress, Stop };
// We can send either new address of proxy, or a command from above.
using PipeMessage = std::variant<sockaddr_in, Command>;
static_assert(sizeof(PipeMessage) <= PIPE_BUF);
