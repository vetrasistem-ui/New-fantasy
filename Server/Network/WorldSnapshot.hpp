#pragma once

#include "Shared/Formats/FMAP/FmapCore.hpp"
#include "Shared/Protocol/FantasyProtocol.hpp"

#include <cstdint>
#include <vector>

namespace fantasy::server::network {

// Builds the deterministic initial chunk set required to render a position.
// F05 intentionally sends the whole containing region on the player's floor;
// later streaming can replace this without changing the MapChunk contract.
std::vector<protocol::MapChunk> makeInitialMapSnapshot(
    const fmap::World& world,
    const fmap::Position& position,
    std::uint32_t revision = 1);

} // namespace fantasy::server::network
