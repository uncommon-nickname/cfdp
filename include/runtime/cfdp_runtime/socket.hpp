#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstdint>
#include <string>

namespace cfdp::runtime::sockets
{
enum class SocketType
{
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM,
};

class Socket
{
  public:
    Socket(SocketType sType, uint16_t port, const std::string& ipAddr);

  private:
    int handle;
    sockaddr_in addr{};
};
} // namespace cfdp::runtime::sockets
