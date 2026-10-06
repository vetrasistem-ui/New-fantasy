#include "Shared/Formats/Legacy/OtbmReader.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fantasy::legacy {
namespace {

constexpr std::uint8_t kNodeStart = 0xFE;
constexpr std::uint8_t kNodeEnd = 0xFF;
constexpr std::uint8_t kEscape = 0xFD;

constexpr std::uint8_t kMapData = 2;
constexpr std::uint8_t kTileArea = 4;
constexpr std::uint8_t kTile = 5;
constexpr std::uint8_t kItem = 6;
constexpr std::uint8_t kTowns = 12;
constexpr std::uint8_t kTown = 13;
constexpr std::uint8_t kHouseTile = 14;
constexpr std::uint8_t kWaypoints = 15;

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
constexpr std::uint8_t kAttrHouseFile = 13;
constexpr std::uint8_t kAttrHouseDoorId = 14;
constexpr std::uint8_t kAttrCount = 15;
constexpr std::uint8_t kAttrCharges = 22;
constexpr std::uint8_t kAttrMap = 128;

struct Node {
    std::uint8_t type = 0;
    std::vector<std::uint8_t> properties;
    std::vector<Node> children;
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

Node parseNode(const std::vector<std::uint8_t>& bytes, std::size_t& cursor) {
    if (cursor >= bytes.size() || bytes[cursor] != kNodeStart) {
        throw std::runtime_error("OTBM node does not start with 0xFE");
    }
    ++cursor;
    if (cursor >= bytes.size()) throw std::runtime_error("OTBM node type is missing");
    Node node;
    node.type = bytes[cursor++];
    while (cursor < bytes.size()) {
        const auto token = bytes[cursor];
        if (token == kEscape) {
            if (cursor + 1 >= bytes.size()) throw std::runtime_error("OTBM escape is truncated");
            node.properties.push_back(bytes[cursor + 1]);
            cursor += 2;
        } else if (token == kNodeStart) {
            node.children.push_back(parseNode(bytes, cursor));
        } else if (token == kNodeEnd) {
            ++cursor;
            return node;
        } else {
            node.properties.push_back(token);
            ++cursor;
        }
    }
    throw std::runtime_error("OTBM node is missing its 0xFF terminator");
}

std::uint8_t u8(const std::vector<std::uint8_t>& b, std::size_t& p) {
    if (p >= b.size()) throw std::runtime_error("OTBM payload truncated (u8)");
    return b[p++];
}
std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t& p) {
    if (p + 2 > b.size()) throw std::runtime_error("OTBM payload truncated (u16)");
    const auto v = static_cast<std::uint16_t>(b[p]) | (static_cast<std::uint16_t>(b[p + 1]) << 8U);
    p += 2; return v;
}
std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t& p) {
    if (p + 4 > b.size()) throw std::runtime_error("OTBM payload truncated (u32)");
    const auto v = static_cast<std::uint32_t>(b[p]) |
        (static_cast<std::uint32_t>(b[p + 1]) << 8U) |
        (static_cast<std::uint32_t>(b[p + 2]) << 16U) |
        (static_cast<std::uint32_t>(b[p + 3]) << 24U);
    p += 4; return v;
}
std::string str(const std::vector<std::uint8_t>& b, std::size_t& p) {
    const auto n = u16(b, p);
    if (p + n > b.size()) throw std::runtime_error("OTBM string is truncated");
    std::string s(b.begin() + static_cast<std::ptrdiff_t>(p), b.begin() + static_cast<std::ptrdiff_t>(p + n));
    p += n; return s;
}
std::string posString(std::uint16_t x, std::uint16_t y, std::uint8_t z) {
    return std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
}

void parseItemAttributes(const std::vector<std::uint8_t>& p, std::size_t& c, LegacyImportedItem& item, LegacyImportDiagnostics& d) {
    while (c < p.size()) {
        const auto attr = u8(p, c);
        switch (attr) {
        case kAttrCount: item.attributes["count"] = std::to_string(u8(p, c)); break;
        case kAttrActionId: item.attributes["actionId"] = std::to_string(u16(p, c)); break;
        case kAttrUniqueId: item.attributes["uniqueId"] = std::to_string(u16(p, c)); break;
        case kAttrText: item.attributes["text"] = str(p, c); break;
        case kAttrDesc: item.attributes["description"] = str(p, c); break;
        case kAttrDepotId: item.attributes["depotId"] = std::to_string(u16(p, c)); break;
        case kAttrHouseDoorId: item.attributes["houseDoorId"] = std::to_string(u8(p, c)); break;
        case kAttrCharges: item.attributes["charges"] = std::to_string(u16(p, c)); break;
        case kAttrTeleDest: {
            const auto x = u16(p, c); const auto y = u16(p, c); const auto z = u8(p, c);
            item.attributes["teleportDestination"] = posString(x, y, z); break;
        }
        case kAttrMap:
            d.warnings.push_back("OTBM attribute-map encountered; raw OTBM v4 attribute map is not decoded yet");
            c = p.size();
            break;
        default:
            d.warnings.push_back("Unsupported item attribute " + std::to_string(attr) + " on server id " + std::to_string(item.serverId));
            c = p.size();
            break;
        }
    }
}

void parseMapHeaderAttributes(const Node& mapData, OtbmMetadata& m, LegacyImportDiagnostics& d) {
    std::size_t c = 0;
    while (c < mapData.properties.size()) {
        const auto attr = u8(mapData.properties, c);
        if (attr == kAttrDescription) m.description = str(mapData.properties, c);
        else if (attr == kAttrSpawnFile) m.spawnFile = str(mapData.properties, c);
        else if (attr == kAttrHouseFile) m.houseFile = str(mapData.properties, c);
        else {
            d.warnings.push_back("Unsupported OTBM map header attribute " + std::to_string(attr));
            break;
        }
    }
}

void parseTileNode(const Node& node, std::uint16_t baseX, std::uint16_t baseY, std::uint8_t baseZ,
                   LegacyMapImportResult& out) {
    std::size_t c = 0;
    LegacyImportedTile tile;
    tile.position.x = static_cast<std::int32_t>(baseX + u8(node.properties, c));
    tile.position.y = static_cast<std::int32_t>(baseY + u8(node.properties, c));
    tile.position.z = baseZ;
    if (node.type == kHouseTile) tile.houseId = u32(node.properties, c);

    while (c < node.properties.size()) {
        const auto attr = u8(node.properties, c);
        if (attr == kAttrTileFlags) {
            const auto flags = u32(node.properties, c);
            if (flags != 0) {
                LegacyImportedItem meta; meta.serverId = 0;
                meta.attributes["tileFlags"] = std::to_string(flags);
                tile.items.push_back(std::move(meta));
            }
        } else if (attr == kAttrCompactItem) {
            LegacyImportedItem item; item.serverId = u16(node.properties, c);
            tile.items.push_back(std::move(item));
            ++out.diagnostics.itemCount;
        } else {
            out.diagnostics.warnings.push_back("Unsupported tile attribute " + std::to_string(attr));
            break;
        }
    }

    for (const auto& child : node.children) {
        if (child.type != kItem) {
            out.diagnostics.warnings.push_back("Unsupported tile child node " + std::to_string(child.type));
            continue;
        }
        std::size_t ic = 0;
        LegacyImportedItem item;
        item.serverId = u16(child.properties, ic);
        parseItemAttributes(child.properties, ic, item, out.diagnostics);
        tile.items.push_back(std::move(item));
        ++out.diagnostics.itemCount;
    }

    out.model.tiles.push_back(std::move(tile));
    ++out.diagnostics.tileCount;
}

void parseMapData(const Node& mapData, OtbmReadResult& result) {
    parseMapHeaderAttributes(mapData, result.metadata, result.import.diagnostics);
    for (const auto& node : mapData.children) {
        if (node.type == kTileArea) {
            std::size_t c = 0;
            const auto baseX = u16(node.properties, c);
            const auto baseY = u16(node.properties, c);
            const auto baseZ = u8(node.properties, c);
            for (const auto& tileNode : node.children) {
                if (tileNode.type == kTile || tileNode.type == kHouseTile) {
                    parseTileNode(tileNode, baseX, baseY, baseZ, result.import);
                } else {
                    result.import.diagnostics.warnings.push_back("Unsupported node in tile area " + std::to_string(tileNode.type));
                }
            }
        } else if (node.type == kTowns) {
            for (const auto& townNode : node.children) {
                if (townNode.type != kTown) continue;
                std::size_t c = 0;
                LegacyImportedTown town;
                town.id = u32(townNode.properties, c);
                town.name = str(townNode.properties, c);
                town.templePosition.x = u16(townNode.properties, c);
                town.templePosition.y = u16(townNode.properties, c);
                town.templePosition.z = u8(townNode.properties, c);
                result.import.model.towns.push_back(std::move(town));
                ++result.import.diagnostics.townCount;
            }
        } else if (node.type == kWaypoints) {
            result.import.diagnostics.warnings.push_back("Waypoints preserved only as diagnostics in the first OTBM milestone");
        } else {
            result.import.diagnostics.warnings.push_back("Unsupported OTBM map-data node " + std::to_string(node.type));
        }
    }
}

} // namespace

OtbmReader::OtbmReader(const std::filesystem::path& path) {
    const auto bytes = readFile(path);
    if (bytes.size() < 5) throw std::runtime_error("OTBM file is too small");
    if (!(bytes[0] == 'O' && bytes[1] == 'T' && bytes[2] == 'B' && bytes[3] == 'M')) {
        throw std::runtime_error("OTBM identifier is missing");
    }
    std::size_t cursor = 4;
    const Node root = parseNode(bytes, cursor);
    if (cursor != bytes.size()) throw std::runtime_error("OTBM contains trailing bytes after root node");

    std::size_t c = 0;
    result_.header.formatVersion = u32(root.properties, c);
    result_.header.width = u16(root.properties, c);
    result_.header.height = u16(root.properties, c);
    result_.header.itemsMajorVersion = u32(root.properties, c);
    result_.header.itemsMinorVersion = u32(root.properties, c);
    result_.import.model.width = result_.header.width;
    result_.import.model.height = result_.header.height;
    result_.import.model.sourceMapName = path.filename().string();

    const auto it = std::find_if(root.children.begin(), root.children.end(), [](const Node& n) { return n.type == kMapData; });
    if (it == root.children.end()) throw std::runtime_error("OTBM MAP_DATA node is missing");
    parseMapData(*it, result_);
}

const OtbmReadResult& OtbmReader::result() const noexcept { return result_; }

} // namespace fantasy::legacy
