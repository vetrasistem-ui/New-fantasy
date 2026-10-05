#include "Network/WorldSnapshot.hpp"
#include "Shared/Protocol/MapChunkPayload.hpp"

#include <algorithm>
#include <stdexcept>

namespace fantasy::server::network {
namespace {

bool contains(const fmap::Region& region, const fmap::Position& position) {
    return position.x >= region.origin.x &&
           position.y >= region.origin.y &&
           position.x < region.origin.x + region.size.width &&
           position.y < region.origin.y + region.size.height;
}

} // namespace

std::vector<protocol::MapChunk> makeInitialMapSnapshot(
    const fmap::World& world,
    const fmap::Position& position,
    std::uint32_t revision) {

    const auto regionIt = std::find_if(world.regions.begin(), world.regions.end(), [&](const fmap::Region& region) {
        return contains(region, position);
    });
    if (regionIt == world.regions.end()) {
        throw std::runtime_error("No FMAP region contains initial snapshot position");
    }

    std::vector<const fmap::Chunk*> chunks;
    for (const auto& chunk : regionIt->chunks) {
        if (chunk.floor == position.z) chunks.push_back(&chunk);
    }
    if (chunks.empty()) {
        throw std::runtime_error("No FMAP chunks exist on initial snapshot floor");
    }

    std::sort(chunks.begin(), chunks.end(), [](const fmap::Chunk* left, const fmap::Chunk* right) {
        if (left->y != right->y) return left->y < right->y;
        return left->x < right->x;
    });

    std::vector<protocol::MapChunk> messages;
    messages.reserve(chunks.size());
    for (const auto* chunk : chunks) {
        messages.push_back(protocol::makeProtocolMapChunk(*regionIt, *chunk, revision));
    }
    return messages;
}

} // namespace fantasy::server::network
