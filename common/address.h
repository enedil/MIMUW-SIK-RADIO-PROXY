#include <netinet/in.h>
#include <unordered_map>
#include <climits>

// Compare addresses.
inline constexpr bool operator==(sockaddr_in const& lhs, sockaddr_in const& rhs) {
    return lhs.sin_addr.s_addr == rhs.sin_addr.s_addr
        && lhs.sin_port == rhs.sin_port;
}

// Hash addresses.
template<> struct std::hash<sockaddr_in> {
    inline constexpr std::size_t operator()(sockaddr_in const& addr) const {
        auto s_addr = addr.sin_addr.s_addr;
        auto port = addr.sin_port;
        static_assert(sizeof(s_addr) + sizeof(port) <= sizeof(std::size_t));
        return (s_addr << (CHAR_BIT * sizeof(port))) | port;
    }
};
