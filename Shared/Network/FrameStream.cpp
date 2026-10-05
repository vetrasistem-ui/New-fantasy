#include "Shared/Network/FrameStream.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fantasy::net {

void sendFrame(TcpStream& stream, const protocol::Frame& frame) {
    const protocol::Bytes wire = protocol::encodeFrame(frame);
    stream.sendAll(wire);
}

protocol::Frame receiveFrame(TcpStream& stream) {
    std::vector<std::uint8_t> wire = stream.receiveExact(protocol::kEnvelopeBytes);
    const auto total = protocol::expectedFrameSize(wire);
    if (!total.has_value()) {
        throw std::runtime_error("Fantasy Protocol envelope size could not be determined");
    }

    const std::size_t payloadBytes = *total - protocol::kEnvelopeBytes;
    if (payloadBytes > 0) {
        auto payload = stream.receiveExact(payloadBytes);
        wire.insert(wire.end(), payload.begin(), payload.end());
    }
    return protocol::decodeFrame(wire);
}

} // namespace fantasy::net
