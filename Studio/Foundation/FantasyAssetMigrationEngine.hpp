#pragma once

#include "Foundation/FantasyFoundationV2.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <vector>

namespace fantasy::studio::foundation {

struct AssetMigrationResolution {
    std::uint32_t legacyId = 0;
    std::uint32_t resolvedTargetId = 0;
    AssetMigrationMode mode = AssetMigrationMode::AddAsNew;
};

struct AssetMigrationPreview {
    std::vector<AssetMigrationResolution> resolutions;
    std::vector<std::uint32_t> allocatedTargetIds;
};

class FantasyAssetMigrationEngine {
public:
    [[nodiscard]] static AssetMigrationPreview preview(
        const AssetMigrationPlan& plan,
        const std::set<std::uint32_t>& occupiedTargetIds,
        std::uint32_t firstAllocatableId = 1) {

        plan.validate();
        if (firstAllocatableId == 0) firstAllocatableId = 1;

        std::set<std::uint32_t> used = occupiedTargetIds;
        AssetMigrationPreview preview;
        preview.resolutions.reserve(plan.entries.size());

        std::uint32_t cursor = firstAllocatableId;
        auto allocate = [&]() mutable {
            while (cursor == 0 || used.find(cursor) != used.end()) {
                if (cursor == UINT32_MAX) {
                    throw std::runtime_error("no free target asset id remains for migration");
                }
                ++cursor;
            }
            const auto allocated = cursor;
            used.insert(allocated);
            preview.allocatedTargetIds.push_back(allocated);
            if (cursor != UINT32_MAX) ++cursor;
            return allocated;
        };

        for (const auto& entry : plan.entries) {
            AssetMigrationResolution resolution;
            resolution.legacyId = entry.legacyId;
            resolution.mode = entry.mode;

            if (entry.mode == AssetMigrationMode::AddAsNew) {
                if (entry.targetId != 0) {
                    if (used.find(entry.targetId) != used.end()) {
                        throw std::runtime_error("requested AddAsNew target id is already occupied");
                    }
                    resolution.resolvedTargetId = entry.targetId;
                    used.insert(entry.targetId);
                } else {
                    resolution.resolvedTargetId = allocate();
                }
            } else {
                if (occupiedTargetIds.find(entry.targetId) == occupiedTargetIds.end()) {
                    throw std::runtime_error("replace migration target id does not exist in target profile");
                }
                resolution.resolvedTargetId = entry.targetId;
            }

            preview.resolutions.push_back(resolution);
        }

        return preview;
    }
};

} // namespace fantasy::studio::foundation
