#include "Shared/Formats/Legacy/OtbmStreamReader.hpp"

#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fantasy::legacy {
namespace {

constexpr std::uint8_t kNodeStart = 0xFE;
constexpr std::uint8_t kNodeEnd = 0xFF;
constexpr std::uint8_t kEscape = 0xFD;

constexpr std::uint8_t kRoot = 1;
constexpr std::uint8_t kMapData = 2;
constexpr std::uint8_t kTileArea = 4;
constexpr std::uint8_t kTile = 5;
constexpr std::uint8_t kItem = 6;
constexpr std::uint8_t kTowns = 12;
constexpr std::uint8_t kTown = 13;
constexpr std::uint8_t kHouseTile = 14;
constexpr std::uint8_t kWaypoints = 15;
constexpr std::uint8_t kWaypoint = 16;

constexpr std::uint8_t kAttrDescription = 1;
constexpr std::uint8_t kAttrTileFlags = 3;
constexpr std::uint8_t kAttrActionId = 4;
constexpr std::uint8_t kAttrUniqueId = 5;
constexpr std::uint8_t kAttrText = 6;
constexpr std::uint8_t kAttrDesc = 7;
constexpr std::uint8_t kAttrTeleDest = 8;
constexpr std::uint8_t kAttrCompactItem = 9;
constexpr std::uint8_t kAttrDepotId = 10;
constexpr std::uint8_t kAttrSpawnFile = 11;
constexpr std::uint8_t kAttrRuneCharges = 12;
constexpr std::uint8_t kAttrHouseFile = 13;
constexpr std::uint8_t kAttrHouseDoorId = 14;
constexpr std::uint8_t kAttrCount = 15;
constexpr std::uint8_t kAttrDuration = 16;
constexpr std::uint8_t kAttrDecayingState = 17;
constexpr std::uint8_t kAttrWrittenDate = 18;
constexpr std::uint8_t kAttrWrittenBy = 19;
constexpr std::uint8_t kAttrSleeperGuid = 20;
constexpr std::uint8_t kAttrSleepStart = 21;
constexpr std::uint8_t kAttrCharges = 22;
constexpr std::uint8_t kAttrMap = 128;

struct Cursor {
    const std::vector<std::uint8_t>& bytes;
    std::size_t position = 0;
};

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Unable to open OTBM file: " + path.string());
    const std::streamsize size = stream.tellg();
    if (size < 0) throw std::runtime_error("Unable to determine OTBM file size");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0, std::ios::beg);
    if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
        throw std::runtime_error("Unable to read OTBM file: " + path.string());
    }
    return bytes;
}

std::uint8_t openNode(Cursor& cursor) {
    if (cursor.position >= cursor.bytes.size() || cursor.bytes[cursor.position] != kNodeStart) {
        throw std::runtime_error("OTBM node does not start with 0xFE");
    }
    ++cursor.position;
    if (cursor.position >= cursor.bytes.size()) throw std::runtime_error("OTBM node type is missing");
    return cursor.bytes[cursor.position++];
}

std::vector<std::uint8_t> readProperties(Cursor& cursor) {
    std::vector<std::uint8_t> properties;
    while (cursor.position < cursor.bytes.size()) {
        const auto value = cursor.bytes[cursor.position];
        if (value == kNodeStart || value == kNodeEnd) break;
        if (value == kEscape) {
            if (cursor.position + 1 >= cursor.bytes.size()) throw std::runtime_error("OTBM escape is truncated");
            properties.push_back(cursor.bytes[cursor.position + 1]);
            cursor.position += 2;
        } else {
            properties.push_back(value);
            ++cursor.position;
        }
    }
    return properties;
}

void closeNode(Cursor& cursor) {
    if (cursor.position >= cursor.bytes.size() || cursor.bytes[cursor.position] != kNodeEnd) {
        throw std::runtime_error("OTBM node is missing its 0xFF terminator");
    }
    ++cursor.position;
}

std::uint8_t u8(const std::vector<std::uint8_t>& bytes, std::size_t& cursor) {
    if (cursor >= bytes.size()) throw std::runtime_error("OTBM payload truncated (u8)");
    return bytes[cursor++];
}

std::uint16_t u16(const std::vector<std::uint8_t>& bytes, std::size_t& cursor) {
    if (cursor + 2 > bytes.size()) throw std::runtime_error("OTBM payload truncated (u16)");
    const auto value = static_cast<std::uint16_t>(bytes[cursor]) |
        (static_cast<std::uint16_t>(bytes[cursor + 1]) << 8U);
    cursor += 2;
    return value;
}

std::uint32_t u32(const std::vector<std::uint8_t>& bytes, std::size_t& cursor) {
    if (cursor + 4 > bytes.size()) throw std::runtime_error("OTBM payload truncated (u32)");
    const auto value = static_cast<std::uint32_t>(bytes[cursor]) |
        (static_cast<std::uint32_t>(bytes[cursor + 1]) << 8U) |
        (static_cast<std::uint32_t>(bytes[cursor + 2]) << 16U) |
        (static_cast<std::uint32_t>(bytes[cursor + 3]) << 24U);
    cursor += 4;
    return value;
}

std::int32_t i32(const std::vector<std::uint8_t>& bytes, std::size_t& cursor) {
    return std::bit_cast<std::int32_t>(u32(bytes, cursor));
}

std::string string16(const std::vector<std::uint8_t>& bytes, std::size_t& cursor) {
    const auto size = u16(bytes, cursor);
    if (cursor + size > bytes.size()) throw std::runtime_error("OTBM string is truncated");
    std::string value(
        bytes.begin() + static_cast<std::ptrdiff_t>(cursor),
        bytes.begin() + static_cast<std::ptrdiff_t>(cursor + size));
    cursor += size;
    return value;
}

std::string positionString(std::uint16_t x, std::uint16_t y, std::uint8_t z) {
    return std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
}

void skipOpenedNode(Cursor& cursor) {
    (void)readProperties(cursor);
    while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
        (void)openNode(cursor);
        skipOpenedNode(cursor);
    }
    closeNode(cursor);
}

void parseItemAttributes(
    const std::vector<std::uint8_t>& properties,
    std::size_t& cursor,
    LegacyImportedItem& item,
    LegacyImportDiagnostics& diagnostics) {

    while (cursor < properties.size()) {
        const auto attr = u8(properties, cursor);
        switch (attr) {
        case kAttrCount:
            item.attributes["count"] = std::to_string(u8(properties, cursor));
            break;
        case kAttrRuneCharges:
            item.attributes["runeCharges"] = std::to_string(u8(properties, cursor));
            break;
        case kAttrActionId:
            item.attributes["actionId"] = std::to_string(u16(properties, cursor));
            break;
        case kAttrUniqueId:
            item.attributes["uniqueId"] = std::to_string(u16(properties, cursor));
            break;
        case kAttrText:
            item.attributes["text"] = string16(properties, cursor);
            break;
        case kAttrDesc:
            item.attributes["description"] = string16(properties, cursor);
            break;
        case kAttrDepotId:
            item.attributes["depotId"] = std::to_string(u16(properties, cursor));
            break;
        case kAttrHouseDoorId:
            item.attributes["houseDoorId"] = std::to_string(u8(properties, cursor));
            break;
        case kAttrDuration:
            item.attributes["duration"] = std::to_string(i32(properties, cursor));
            break;
        case kAttrDecayingState:
            item.attributes["decayingState"] = std::to_string(u8(properties, cursor));
            break;
        case kAttrWrittenDate:
            item.attributes["writtenDate"] = std::to_string(u32(properties, cursor));
            break;
        case kAttrWrittenBy:
            item.attributes["writtenBy"] = string16(properties, cursor);
            break;
        case kAttrSleeperGuid:
            item.attributes["sleeperGuid"] = std::to_string(u32(properties, cursor));
            break;
        case kAttrSleepStart:
            item.attributes["sleepStart"] = std::to_string(u32(properties, cursor));
            break;
        case kAttrCharges:
            item.attributes["charges"] = std::to_string(u16(properties, cursor));
            break;
        case kAttrTeleDest: {
            const auto x = u16(properties, cursor);
            const auto y = u16(properties, cursor);
            const auto z = u8(properties, cursor);
            item.attributes["teleportDestination"] = positionString(x, y, z);
            break;
        }
        case kAttrMap:
            diagnostics.warnings.push_back(
                "OTBM attribute-map encountered; raw OTBM v4 attribute map is outside the current TFS 1.4.2/OTBM v3 gate");
            cursor = properties.size();
            break;
        default:
            diagnostics.warnings.push_back(
                "Unsupported item attribute " + std::to_string(attr) +
                " on server id " + std::to_string(item.serverId));
            cursor = properties.size();
            break;
        }
    }
}

LegacyImportedItem parseOpenedItem(Cursor& cursor, OtbmStreamReadResult& result) {
    const auto properties = readProperties(cursor);
    std::size_t propertyCursor = 0;
    LegacyImportedItem item;
    item.serverId = u16(properties, propertyCursor);
    parseItemAttributes(properties, propertyCursor, item, result.diagnostics);
    ++result.diagnostics.itemCount;

    while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
        const auto childType = openNode(cursor);
        if (childType == kItem) {
            item.contents.push_back(parseOpenedItem(cursor, result));
        } else {
            result.diagnostics.warnings.push_back(
                "Unsupported child node " + std::to_string(childType) +
                " inside item server id " + std::to_string(item.serverId));
            skipOpenedNode(cursor);
        }
    }
    closeNode(cursor);
    return item;
}

LegacyImportedTile parseOpenedTile(
    Cursor& cursor,
    std::uint8_t type,
    std::uint16_t baseX,
    std::uint16_t baseY,
    std::uint8_t baseZ,
    OtbmStreamReadResult& result) {

    const auto properties = readProperties(cursor);
    std::size_t propertyCursor = 0;
    LegacyImportedTile tile;
    tile.position.x = static_cast<std::int32_t>(baseX + u8(properties, propertyCursor));
    tile.position.y = static_cast<std::int32_t>(baseY + u8(properties, propertyCursor));
    tile.position.z = baseZ;
    if (type == kHouseTile) tile.houseId = u32(properties, propertyCursor);

    while (propertyCursor < properties.size()) {
        const auto attr = u8(properties, propertyCursor);
        if (attr == kAttrTileFlags) {
            LegacyImportedItem metadata;
            metadata.serverId = 0;
            metadata.attributes["tileFlags"] = std::to_string(u32(properties, propertyCursor));
            tile.items.push_back(std::move(metadata));
        } else if (attr == kAttrCompactItem) {
            LegacyImportedItem item;
            item.serverId = u16(properties, propertyCursor);
            tile.items.push_back(std::move(item));
            ++result.diagnostics.itemCount;
        } else {
            result.diagnostics.warnings.push_back("Unsupported tile attribute " + std::to_string(attr));
            propertyCursor = properties.size();
        }
    }

    while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
        const auto childType = openNode(cursor);
        if (childType == kItem) {
            tile.items.push_back(parseOpenedItem(cursor, result));
        } else {
            result.diagnostics.warnings.push_back("Unsupported tile child node " + std::to_string(childType));
            skipOpenedNode(cursor);
        }
    }
    closeNode(cursor);
    ++result.diagnostics.tileCount;
    return tile;
}

void parseOpenedTileArea(
    Cursor& cursor,
    OtbmStreamReadResult& result,
    const OtbmStreamCallbacks& callbacks) {

    const auto properties = readProperties(cursor);
    std::size_t propertyCursor = 0;
    const auto baseX = u16(properties, propertyCursor);
    const auto baseY = u16(properties, propertyCursor);
    const auto baseZ = u8(properties, propertyCursor);

    while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
        const auto childType = openNode(cursor);
        if (childType == kTile || childType == kHouseTile) {
            auto tile = parseOpenedTile(cursor, childType, baseX, baseY, baseZ, result);
            if (callbacks.onTile) callbacks.onTile(std::move(tile));
        } else {
            result.diagnostics.warnings.push_back(
                "Unsupported node in tile area " + std::to_string(childType));
            skipOpenedNode(cursor);
        }
    }
    closeNode(cursor);
}

void parseOpenedTowns(
    Cursor& cursor,
    OtbmStreamReadResult& result,
    const OtbmStreamCallbacks& callbacks) {

    (void)readProperties(cursor);
    while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
        const auto childType = openNode(cursor);
        if (childType != kTown) {
            skipOpenedNode(cursor);
            continue;
        }

        const auto properties = readProperties(cursor);
        std::size_t propertyCursor = 0;
        LegacyImportedTown town;
        town.id = u32(properties, propertyCursor);
        town.name = string16(properties, propertyCursor);
        town.templePosition.x = u16(properties, propertyCursor);
        town.templePosition.y = u16(properties, propertyCursor);
        town.templePosition.z = u8(properties, propertyCursor);
        while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
            (void)openNode(cursor);
            skipOpenedNode(cursor);
        }
        closeNode(cursor);
        ++result.diagnostics.townCount;
        if (callbacks.onTown) callbacks.onTown(std::move(town));
    }
    closeNode(cursor);
}

void parseOpenedWaypoints(
    Cursor& cursor,
    OtbmStreamReadResult& result,
    const OtbmStreamCallbacks& callbacks) {

    (void)readProperties(cursor);
    while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
        const auto childType = openNode(cursor);
        if (childType != kWaypoint) {
            result.diagnostics.warnings.push_back(
                "Unsupported node in waypoint container " + std::to_string(childType));
            skipOpenedNode(cursor);
            continue;
        }

        const auto properties = readProperties(cursor);
        std::size_t propertyCursor = 0;
        LegacyImportedWaypoint waypoint;
        waypoint.name = string16(properties, propertyCursor);
        waypoint.position.x = u16(properties, propertyCursor);
        waypoint.position.y = u16(properties, propertyCursor);
        waypoint.position.z = u8(properties, propertyCursor);
        while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
            (void)openNode(cursor);
            skipOpenedNode(cursor);
        }
        closeNode(cursor);
        ++result.diagnostics.waypointCount;
        if (callbacks.onWaypoint) callbacks.onWaypoint(std::move(waypoint));
    }
    closeNode(cursor);
}

void parseMapMetadata(
    const std::vector<std::uint8_t>& properties,
    OtbmStreamReadResult& result) {

    std::size_t cursor = 0;
    while (cursor < properties.size()) {
        const auto attr = u8(properties, cursor);
        if (attr == kAttrDescription) result.metadata.description = string16(properties, cursor);
        else if (attr == kAttrSpawnFile) result.metadata.spawnFile = string16(properties, cursor);
        else if (attr == kAttrHouseFile) result.metadata.houseFile = string16(properties, cursor);
        else {
            result.diagnostics.warnings.push_back("Unsupported OTBM map header attribute " + std::to_string(attr));
            break;
        }
    }
}

void parseOpenedMapData(
    Cursor& cursor,
    OtbmStreamReadResult& result,
    const OtbmStreamCallbacks& callbacks) {

    parseMapMetadata(readProperties(cursor), result);
    while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
        const auto childType = openNode(cursor);
        if (childType == kTileArea) parseOpenedTileArea(cursor, result, callbacks);
        else if (childType == kTowns) parseOpenedTowns(cursor, result, callbacks);
        else if (childType == kWaypoints) parseOpenedWaypoints(cursor, result, callbacks);
        else {
            result.diagnostics.warnings.push_back(
                "Unsupported OTBM map-data node " + std::to_string(childType));
            skipOpenedNode(cursor);
        }
    }
    closeNode(cursor);
}

} // namespace

OtbmStreamReader::OtbmStreamReader(
    const std::filesystem::path& path,
    OtbmStreamCallbacks callbacks) {

    const auto bytes = readFile(path);
    if (bytes.size() < 5) throw std::runtime_error("OTBM file is too small");

    const bool nullIdentifier =
        bytes[0] == 0U && bytes[1] == 0U && bytes[2] == 0U && bytes[3] == 0U;
    const bool asciiIdentifier =
        bytes[0] == 'O' && bytes[1] == 'T' && bytes[2] == 'B' && bytes[3] == 'M';
    if (!nullIdentifier && !asciiIdentifier) throw std::runtime_error("Unsupported OTBM identifier");

    Cursor cursor{bytes, 4U};
    const auto rootType = openNode(cursor);
    if (rootType != kRoot) throw std::runtime_error("OTBM root node has unexpected type");

    const auto rootProperties = readProperties(cursor);
    std::size_t propertyCursor = 0;
    result_.header.formatVersion = u32(rootProperties, propertyCursor);
    result_.header.width = u16(rootProperties, propertyCursor);
    result_.header.height = u16(rootProperties, propertyCursor);
    result_.header.itemsMajorVersion = u32(rootProperties, propertyCursor);
    result_.header.itemsMinorVersion = u32(rootProperties, propertyCursor);

    bool foundMapData = false;
    while (cursor.position < cursor.bytes.size() && cursor.bytes[cursor.position] == kNodeStart) {
        const auto childType = openNode(cursor);
        if (childType == kMapData && !foundMapData) {
            parseOpenedMapData(cursor, result_, callbacks);
            foundMapData = true;
        } else {
            skipOpenedNode(cursor);
        }
    }
    closeNode(cursor);

    if (!foundMapData) throw std::runtime_error("OTBM MAP_DATA node is missing");
    if (cursor.position != cursor.bytes.size()) {
        throw std::runtime_error("OTBM contains trailing bytes after root node");
    }
}

} // namespace fantasy::legacy
