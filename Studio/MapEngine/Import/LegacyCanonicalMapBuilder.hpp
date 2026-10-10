#pragma once

#include "LegacyMapAdapter.hpp"
#include "Shared/Formats/Legacy/LegacyMapImportModel.hpp"

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>

namespace fantasy::studio::mapcore {

// Converts neutral legacy records directly into canonical Fantasy map storage.
// It is designed for streaming import: callers do not need to retain the full
// LegacyMapImportModel in memory.
class LegacyCanonicalMapBuilder {
public:
    explicit LegacyCanonicalMapBuilder(const fantasy::assets::FantasyAssetRegistry& assets);

    void setMetadata(MapMetadata metadata);
    void setItemCount(std::size_t itemCount) noexcept { report_.itemCount = itemCount; }
    void appendWarning(std::string warning);

    void addTile(fantasy::legacy::LegacyImportedTile&& source);
    void addTown(fantasy::legacy::LegacyImportedTown&& source);
    void addWaypoint(fantasy::legacy::LegacyImportedWaypoint&& source);
    void addHouse(fantasy::legacy::LegacyImportedHouse&& source);
    void addSpawn(fantasy::legacy::LegacyImportedSpawn&& source);

    [[nodiscard]] LegacyMapAdaptResult finish();

private:
    [[nodiscard]] Item convertItem(const fantasy::legacy::LegacyImportedItem& source);
    [[nodiscard]] bool isGround(std::uint32_t serverId) const;

    const fantasy::assets::FantasyAssetRegistry& assets_;
    MapStorage map_;
    MapMetadata metadata_;
    LegacyMapAdaptReport report_;
    std::set<std::uint32_t> unresolved_;
};

} // namespace fantasy::studio::mapcore
