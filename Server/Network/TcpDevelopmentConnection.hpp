#pragma once

#include "Core/WorldRuntime.hpp"
#include "Shared/Network/TcpTransport.hpp"

#include <cstddef>

namespace fantasy::server::network {

struct ConnectionStats {
    std::size_t framesReceived = 0;
    std::size_t framesSent = 0;
    bool cleanDisconnect = false;
};

ConnectionStats serveDevelopmentConnection(net::TcpStream stream, WorldRuntime& world);

} // namespace fantasy::server::network
