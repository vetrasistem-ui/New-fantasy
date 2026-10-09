#pragma once

#include "Foundation/FantasyFoundationV2.hpp"
#include "Foundation/FantasyJsonOptional.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

enum class BrushKind {
    Terrain,
    AutoBorder,
    Wall,
    Doodad,
    Carpet,
    Table,
    Erase,
};

struct BrushVariant {
    std::string assetRef;
    std::uint32_t weight = 1;
};

struct BrushTransition {
    std::string neighborTag;
    std::string assetRef;
    std::int32_t priority = 0;
};

struct BrushDefinition {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string id;
    BrushKind kind = BrushKind::Terrain;
    std::vector<std::string> requiredTags;
    std::vector<BrushVariant> variants;
    std::vector<BrushTransition> transitions;

    void validate() const {
        requireIdentifier(id, "brush id");
        if (kind != BrushKind::Erase && variants.empty()) {
            throw std::invalid_argument("Fantasy brush requires at least one variant unless it is Erase");
        }
        for (const auto& tag : requiredTags) requireIdentifier(tag, "brush required tag");
        for (const auto& variant : variants) {
            requireIdentifier(variant.assetRef, "brush asset reference");
            if (variant.weight == 0) throw std::invalid_argument("brush variant weight must be greater than zero");
        }
        for (const auto& transition : transitions) {
            requireIdentifier(transition.neighborTag, "brush transition neighbor tag");
            requireIdentifier(transition.assetRef, "brush transition asset reference");
        }
    }
};

class FantasyBrushSelector {
public:
    [[nodiscard]] static std::string chooseVariant(const BrushDefinition& brush, std::uint64_t seed) {
        brush.validate();
        if (brush.kind == BrushKind::Erase) return {};
        std::uint64_t total = 0;
        for (const auto& variant : brush.variants) total += variant.weight;
        if (total == 0) throw std::runtime_error("brush variant total weight cannot be zero");

        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        const std::uint64_t value = (seed * 2685821657736338717ULL) % total;

        std::uint64_t cursor = 0;
        for (const auto& variant : brush.variants) {
            cursor += variant.weight;
            if (value < cursor) return variant.assetRef;
        }
        return brush.variants.back().assetRef;
    }

    [[nodiscard]] static std::vector<BrushTransition> orderedTransitions(const BrushDefinition& brush) {
        brush.validate();
        auto transitions = brush.transitions;
        std::sort(transitions.begin(), transitions.end(), [](const auto& left, const auto& right) {
            if (left.priority != right.priority) return left.priority > right.priority;
            if (left.neighborTag != right.neighborTag) return left.neighborTag < right.neighborTag;
            return left.assetRef < right.assetRef;
        });
        return transitions;
    }
};

} // namespace fantasy::studio::foundation
