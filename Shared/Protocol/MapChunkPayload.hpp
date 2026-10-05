#pragma once

#include "Shared/Formats/FMAP/FmapCore.hpp"
#include "Shared/Protocol/FantasyProtocol.hpp"

#include <cstdint>

namespace fantasy::protocol {

inline constexpr std::uint16_t kMapChunkPayloadVersion = 1;

// Encodes only the semantic tile content of one FMAP chunk. Chunk identity
// (region, chunk x/y, floor, revision) stays in the outer MapChunk message.
Bytes encodeFmapChunkPayload(const fmap::Chunk& chunk);

// Reconstructs an FMAP chunk using identity from the outer MapChunk message
// and semantic tiles from its payload bytes.
fmap::Chunk decodeFmapChunkPayload(const MapChunk& message);

// Convenience helper used by Server snapshot generation.
MapChunk makeProtocolMapChunk(
    const std::string& regionId,
    const fmap::Chunk& chunk,
    std::uint32_t revision = 1);

} // namespace fantasy::protocol
