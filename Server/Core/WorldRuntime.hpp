#pragma once

#include "Core/Scheduler.hpp"
#include "Shared/Formats/FMAP/FmapCore.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

namespace fantasy::server {

using MapPosition = fantasy::fmap::Position;
using MapTile = fantasy::fmap::Tile;
using MapWorld = fantasy::fmap::World;

struct Entity {
    std::uint64_t id = 0;
    std::string name;
    MapPosition position;
};

enum class RuntimeState {
    Stopped,
    Starting,
    Ready
};

class WorldRuntime {
public:
    static WorldRuntime load(const std::filesystem::path& fmapPath);
    explicit WorldRuntime(MapWorld world);

    const MapWorld& world() const { return world_; }
    RuntimeState state() const { return state_; }
    std::size_t indexedTileCount() const { return tiles_.size(); }
    std::size_t entityCount() const { return entities_.size(); }
    std::uint64_t tickCount() const { return tickCount_; }
    std::size_t pendingTaskCount() const { return scheduler_.pendingTaskCount(); }

    void start();
    void stop();
    void tick();

    Scheduler::TaskId scheduleAfter(std::uint64_t delayTicks, std::function<void()> callback);
    bool cancelTask(Scheduler::TaskId id);

    const MapTile* tileAt(const MapPosition& position) const;
    bool isWalkable(const MapPosition& position) const;

    std::uint64_t spawnEntity(std::string name, const MapPosition& position);
    const Entity* entity(std::uint64_t id) const;
    bool moveEntity(std::uint64_t id, std::int32_t dx, std::int32_t dy);

private:
    static std::string coordinateKey(const MapPosition& position);
    void rebuildTileIndex();

    MapWorld world_;
    RuntimeState state_ = RuntimeState::Stopped;
    std::uint64_t tickCount_ = 0;
    std::uint64_t nextEntityId_ = 1;
    Scheduler scheduler_;
    std::unordered_map<std::string, const MapTile*> tiles_;
    std::unordered_map<std::uint64_t, Entity> entities_;
};

} // namespace fantasy::server
