#pragma once

#include "Shared/Network/TcpTransport.hpp"
#include "Shared/Protocol/FantasyProtocol.hpp"

namespace fantasy::net {

void sendFrame(TcpStream& stream, const protocol::Frame& frame);
protocol::Frame receiveFrame(TcpStream& stream);

} // namespace fantasy::net
