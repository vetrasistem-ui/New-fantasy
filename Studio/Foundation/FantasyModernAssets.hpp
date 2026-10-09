#pragma once

#include "Foundation/FantasyFoundationV2.hpp"

#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

enum class ModernAssetKind {
    Generic,
    Ground,
    Border,
    Wall,
    Doodad,
    Item,
    Creature,
    Outfit,
    Effect,
    Missile,
    Ui,
};

struct ModernAssetFrame {
    std::string visualRef;
    std::uint32_t durationMs = 100;
};

struct ModernAssetLayer {
    std::string id;
    std::int32_t order = 0;
    std::int32_t offsetX = 0;
    std::int32_t offsetY = 0;
    std::vector<ModernAssetFrame> frames;
};

struct TerrainTransitionRule {
    std::string neighborTag;
    std::string borderAssetRef;
    std::int32_t priority = 0;
};

struct ModernAssetDefinition {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string id;
    ModernAssetKind kind = ModernAssetKind::Generic;
    std::uint16_t width = 1;
    std::uint16_t height = 1;
    std::vector<std::string> tags;
    std::vector<ModernAssetLayer> layers;
    std::vector<TerrainTransitionRule> transitions;

    void validate() const {
        requireIdentifier(id, "modern asset id");
        if (width == 0 || height == 0) throw std::invalid_argument("modern asset dimensions must be non-zero");
        if (layers.empty()) throw std::invalid_argument("modern asset requires at least one visual layer");

        std::set<std::string> tagIds;
        for (const auto& tag : tags) {
            requireIdentifier(tag, "modern asset tag");
            if (!tagIds.insert(tag).second) throw std::invalid_argument("duplicate modern asset tag: " + tag);
        }

        std::set<std::string> layerIds;
        for (const auto& layer : layers) {
            requireIdentifier(layer.id, "modern asset layer id");
            if (!layerIds.insert(layer.id).second) throw std::invalid_argument("duplicate modern asset layer: " + layer.id);
            if (layer.frames.empty()) throw std::invalid_argument("modern asset layer requires at least one frame");
            for (const auto& frame : layer.frames) {
                requireIdentifier(frame.visualRef, "modern asset frame visual reference");
                if (frame.durationMs == 0) throw std::invalid_argument("modern asset frame duration must be greater than zero");
            }
        }

        for (const auto& transition : transitions) {
            requireIdentifier(transition.neighborTag, "terrain transition neighbor tag");
            requireIdentifier(transition.borderAssetRef, "terrain transition border asset");
        }
    }
};

} // namespace fantasy::studio::foundation
