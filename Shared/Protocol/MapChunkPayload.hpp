#pragma once

#include "Shared/Formats/FMAP/FmapCore.hpp"
#include "Shared/Protocol/FantasyProtocol.hpp"

#include <cstdint>

namespace fantasy::protocol {

inline constexpr std::uint16_t kMapChunkPayloadVersion = 1;

// Encodes only the semantic tile content of one FMAP chunk. Chunk identity
// and region origin stay in the outer MapChunk message.
Bytes encodeFmapChunkPayload(const fmap::Chunk& chunk);

// Reconstructs the FMAP chunk-local data. Region origin remains available on
// the outer MapChunk for resolving global tile positions on the client.
fmap::Chunk decodeFmapChunkPayload(const MapChunk& message);

// Convenience helper used by Server snapshot generation.
MapChunk makeProtocolMapChunk(
    const fmap::Region& region,
    const fmap::Chunk& chunk,
    std::uint32_t revision = 1);

} // namespace fantasy::protocol
