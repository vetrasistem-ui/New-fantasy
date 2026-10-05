#pragma once

#include "MapEngine/FmapCore.hpp"

#include <cstddef>
#include <string>

namespace fantasy::studio::map {

class EditorOperations {
public:
    static void paintGround(MapDocument& document, const TileLocator& locator, const std::string& ground);
    static void addObject(MapDocument& document, const TileLocator& locator, const std::string& objectKey);
    static void removeObject(MapDocument& document, const TileLocator& locator, const std::string& objectKey);

    // Repaints the 4-neighbour connected area of existing tiles that share the
    // selected tile's current ground. Returns the number of changed tiles.
    static std::size_t fillConnectedGround(
        MapDocument& document,
        const TileLocator& locator,
        const std::string& replacementGround);

    // Removes all objects from one existing tile in a single undoable action.
    // Returns the number of removed objects.
    static std::size_t eraseObjects(MapDocument& document, const TileLocator& locator);
};

} // namespace fantasy::studio::map
