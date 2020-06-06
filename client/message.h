#pragma once
#include <climits>
#include <netinet/in.h>
#include <variant>
#include <vector>

// Commands that can be sent to Keepalive thread and Receiver thread.
enum class Command { NoAddress, Stop };
// We can send either new address of proxy, or a command from above.
using PipeMessage = std::variant<sockaddr_in, Command>;
static_assert(sizeof(PipeMessage) <= PIPE_BUF);
