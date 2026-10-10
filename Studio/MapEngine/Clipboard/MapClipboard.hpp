#pragma once

#include "../Commands/CommandExecutor.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fantasy::studio::mapcore {

enum class PasteMode {
    Replace,
    Merge,
};

struct ClipboardSnapshot {
    Position anchor;
    std::uint64_t sourceRevision = 0;
    std::vector<Tile> tiles;
};

struct PasteOptions {
    PasteMode mode = PasteMode::Replace;
    bool preserveHouseIds = true;
    bool preserveTileFlags = true;
    bool previewOnly = false;
    CommandOrigin origin = CommandOrigin::Human;
    std::optional<std::uint64_t> expectedRevision;
    std::string requestId = "clipboard-paste";
};

class MapClipboard {
public:
    bool capture(const MapDocument& document, std::string* error = nullptr);
    void clear() noexcept;

    [[nodiscard]] bool empty() const noexcept { return !snapshot_.has_value() || snapshot_->tiles.empty(); }
    [[nodiscard]] const std::optional<ClipboardSnapshot>& snapshot() const noexcept { return snapshot_; }

    [[nodiscard]] CommandResult cut(
        MapDocument& document,
        CommandOrigin origin = CommandOrigin::Human,
        std::string requestId = "clipboard-cut",
        bool previewOnly = false);

    [[nodiscard]] CommandResult paste(
        MapDocument& document,
        const Position& targetAnchor,
        const PasteOptions& options = {}) const;

private:
    [[nodiscard]] static Tile mergeTile(
        const Tile* destination,
        const Tile& source,
        const Position& targetPosition,
        const PasteOptions& options);

    std::optional<ClipboardSnapshot> snapshot_;
};

} // namespace fantasy::studio::mapcore
