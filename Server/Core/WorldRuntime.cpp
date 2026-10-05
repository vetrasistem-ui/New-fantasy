#include "Core/WorldRuntime.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace fantasy::server {

WorldRuntime WorldRuntime::load(const std::filesystem::path& fmapPath) {
    return WorldRuntime(fantasy::studio::map::loadFmap(fmapPath));
}

WorldRuntime::WorldRuntime(MapWorld world) : world_(std::move(world)) {
    fantasy::studio::map::requireValidWorld(world_);
    rebuildTileIndex();
}

std::string WorldRuntime::coordinateKey(const MapPosition& position) {
    return std::to_string(position.x) + ":" + std::to_string(position.y) + ":" + std::to_string(position.z);
}

void WorldRuntime::rebuildTileIndex() {
    tiles_.clear();
    for (const auto& region : world_.regions) {
        for (const auto& chunk : region.chunks) {
            for (const auto& tile : chunk.tiles) {
                const MapPosition global{
                    region.origin.x + chunk.x + tile.x,
                    region.origin.y + chunk.y + tile.y,
                    chunk.floor
                };
                const auto key = coordinateKey(global);
                if (!tiles_.emplace(key, &tile).second) {
                    throw std::runtime_error("FMAP runtime duplicate global tile: " + key);
                }
            }
        }
    }
}

void WorldRuntime::start() {
    if (state_ != RuntimeState::Stopped) {
        throw std::runtime_error("WorldRuntime can only start from Stopped state");
    }
    state_ = RuntimeState::Starting;
    tickCount_ = 0;
    state_ = RuntimeState::Ready;
}

void WorldRuntime::stop() {
    entities_.clear();
    state_ = RuntimeState::Stopped;
}

void WorldRuntime::tick() {
    if (state_ != RuntimeState::Ready) {
        throw std::runtime_error("WorldRuntime tick requires Ready state");
    }
    ++tickCount_;
}

const MapTile* WorldRuntime::tileAt(const MapPosition& position) const {
    const auto it = tiles_.find(coordinateKey(position));
    return it == tiles_.end() ? nullptr : it->second;
}

bool WorldRuntime::isWalkable(const MapPosition& position) const {
    const auto* tile = tileAt(position);
    if (tile == nullptr) return false;
    return std::none_of(tile->tags.begin(), tile->tags.end(), [](const std::string& tag) {
        return tag == "blocked" || tag == "non-walkable";
    });
}

std::uint64_t WorldRuntime::spawnEntity(std::string name, const MapPosition& position) {
    if (state_ != RuntimeState::Ready) {
        throw std::runtime_error("Cannot spawn entity while world runtime is not Ready");
    }
    if (!isWalkable(position)) {
        throw std::runtime_error("Cannot spawn entity on non-walkable or missing tile");
    }
    const auto id = nextEntityId_++;
    entities_.emplace(id, Entity{id, std::move(name), position});
    return id;
}

const Entity* WorldRuntime::entity(std::uint64_t id) const {
    const auto it = entities_.find(id);
    return it == entities_.end() ? nullptr : &it->second;
}

bool WorldRuntime::moveEntity(std::uint64_t id, std::int32_t dx, std::int32_t dy) {
    if (state_ != RuntimeState::Ready) return false;
    if (std::abs(dx) + std::abs(dy) != 1) return false;

    const auto it = entities_.find(id);
    if (it == entities_.end()) return false;

    MapPosition target = it->second.position;
    target.x += dx;
    target.y += dy;
    if (!isWalkable(target)) return false;

    for (const auto& [otherId, other] : entities_) {
        if (otherId != id && other.position == target) return false;
    }

    it->second.position = target;
    return true;
}

} // namespace fantasy::server
