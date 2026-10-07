#pragma once

#include "MapCommand.hpp"
#include "../Core/MapDocument.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace fantasy::studio::mapcore {

enum class CommandStatus {
    Applied,
    Preview,
    RevisionConflict,
    Invalid,
};

struct CommandResult {
    CommandStatus status = CommandStatus::Invalid;
    std::string requestId;
    std::string message;
    std::uint64_t baseRevision = 0;
    std::uint64_t resultRevision = 0;
    std::size_t affectedTiles = 0;
    MapAction diff;

    [[nodiscard]] bool ok() const noexcept {
        return status == CommandStatus::Applied || status == CommandStatus::Preview;
    }
};

class CommandExecutor {
public:
    [[nodiscard]] CommandResult execute(MapDocument& document, const MapCommand& command) const;

private:
    [[nodiscard]] static CommandResult buildResult(
        const MapDocument& document,
        const MapCommand& command,
        MapAction action);
};

} // namespace fantasy::studio::mapcore
