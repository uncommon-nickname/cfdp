#include <cfdp_core/utils.hpp>
#include <cfdp_runtime/logger.hpp>
#include <cfdp_runtime/socket.hpp>

#include <stdexcept>

namespace
{
constexpr int invalid_socket    = -1;
constexpr int ip_addr_converted = 1;
} // namespace

cfdp::runtime::sockets::Socket::Socket(SocketType sType, uint16_t port, const std::string& ipAddr)
    : handle(socket(AF_INET, utils::toUnderlying(sType), 0))
{
    if (handle == invalid_socket)
    {
        logging::error("Could not create a valid socket.");
        throw std::runtime_error{"Could not create a socket."};
    }

    addr.sin_family = AF_INET;
    addr.sin_port   = port;

    auto result = inet_pton(AF_INET, ipAddr.c_str(), &addr.sin_addr);

    if (result != ip_addr_converted)
    {
        logging::error("Could not parse ip address: {}.", result);
        throw std::runtime_error{"Could not parse passed IP address."};
    }
}
