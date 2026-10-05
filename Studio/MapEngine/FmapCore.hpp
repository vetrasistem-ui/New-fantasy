#pragma once

#include "Shared/Formats/FMAP/FmapCore.hpp"

namespace fantasy::studio::map {

using fantasy::fmap::Chunk;
using fantasy::fmap::MapDocument;
using fantasy::fmap::Position;
using fantasy::fmap::Region;
using fantasy::fmap::Size;
using fantasy::fmap::Tile;
using fantasy::fmap::TileLocator;
using fantasy::fmap::World;
using fantasy::fmap::WorldInfo;
using fantasy::fmap::isSemanticAssetKey;
using fantasy::fmap::loadFmap;
using fantasy::fmap::requireValidWorld;
using fantasy::fmap::saveFmap;
using fantasy::fmap::validateWorld;

} // namespace fantasy::studio::map
