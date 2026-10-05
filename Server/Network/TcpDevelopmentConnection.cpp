#include "Network/TcpDevelopmentConnection.hpp"

#include "Network/DevelopmentSession.hpp"
#include "Shared/Network/FrameStream.hpp"

namespace fantasy::server::network {

ConnectionStats serveDevelopmentConnection(net::TcpStream stream, WorldRuntime& world) {
    DevelopmentSession session(world);
    ConnectionStats stats;

    try {
        while (session.state() != SessionState::Closed) {
            const protocol::Frame incoming = net::receiveFrame(stream);
            ++stats.framesReceived;

            const auto replies = session.handle(incoming);
            for (const auto& reply : replies) {
                net::sendFrame(stream, reply);
                ++stats.framesSent;
            }

            if (incoming.messageType == protocol::MessageType::Disconnect && session.state() == SessionState::Closed) {
                stats.cleanDisconnect = true;
                break;
            }
        }
    } catch (...) {
        session.close();
        stream.close();
        throw;
    }

    session.close();
    stream.close();
    return stats;
}

} // namespace fantasy::server::network
