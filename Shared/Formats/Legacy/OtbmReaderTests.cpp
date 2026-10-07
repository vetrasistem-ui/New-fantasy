#include "Shared/Formats/Legacy/OtbmReader.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using fantasy::legacy::OtbmReader;

namespace {
void u16(std::vector<std::uint8_t>& out, std::uint16_t v) { out.push_back(v & 0xFFU); out.push_back((v >> 8U) & 0xFFU); }
void u32(std::vector<std::uint8_t>& out, std::uint32_t v) { out.push_back(v & 0xFFU); out.push_back((v >> 8U) & 0xFFU); out.push_back((v >> 16U) & 0xFFU); out.push_back((v >> 24U) & 0xFFU); }
void str(std::vector<std::uint8_t>& out, const std::string& s) { u16(out, static_cast<std::uint16_t>(s.size())); out.insert(out.end(), s.begin(), s.end()); }
void escaped(std::vector<std::uint8_t>& out, std::uint8_t v) { if (v >= 0xFDU) out.push_back(0xFDU); out.push_back(v); }
void props(std::vector<std::uint8_t>& out, const std::vector<std::uint8_t>& p) { for (auto v : p) escaped(out, v); }
void nodeBegin(std::vector<std::uint8_t>& out, std::uint8_t type) { out.push_back(0xFEU); out.push_back(type); }
void nodeEnd(std::vector<std::uint8_t>& out) { out.push_back(0xFFU); }

std::vector<std::uint8_t> makeMap() {
    std::vector<std::uint8_t> out{'O','T','B','M'};
    nodeBegin(out, 1);
    std::vector<std::uint8_t> root;
    u32(root, 2); u16(root, 512); u16(root, 512); u32(root, 3); u32(root, 57);
    props(out, root);

    nodeBegin(out, 2);
    std::vector<std::uint8_t> header;
    header.push_back(1); str(header, "Fantasy synthetic 10.98");
    header.push_back(11); str(header, "map-spawn.xml");
    header.push_back(13); str(header, "map-house.xml");
    props(out, header);

    nodeBegin(out, 4);
    std::vector<std::uint8_t> area; u16(area, 100); u16(area, 200); area.push_back(7); props(out, area);

    nodeBegin(out, 5);
    std::vector<std::uint8_t> tile{1,2,9}; u16(tile, 100); props(out, tile);
    nodeBegin(out, 6);
    std::vector<std::uint8_t> item;
    u16(item, 101);
    item.push_back(4); u16(item, 450);
    item.push_back(6); str(item, "hello");
    item.push_back(12); item.push_back(7);
    item.push_back(16); u32(item, static_cast<std::uint32_t>(static_cast<std::int32_t>(-500)));
    item.push_back(17); item.push_back(1);
    item.push_back(18); u32(item, 123456);
    item.push_back(19); str(item, "Fantasy Writer");
    item.push_back(20); u32(item, 42);
    item.push_back(21); u32(item, 98765);
    item.push_back(22); u16(item, 55);
    props(out, item);

    nodeBegin(out, 6);
    std::vector<std::uint8_t> nested; u16(nested, 103); nested.push_back(15); nested.push_back(4); props(out, nested);
    nodeEnd(out);
    nodeEnd(out);
    nodeEnd(out);

    nodeBegin(out, 14);
    std::vector<std::uint8_t> house{3,4}; u32(house, 77); house.push_back(9); u16(house, 102); props(out, house);
    nodeEnd(out);
    nodeEnd(out);

    nodeBegin(out, 12);
    nodeBegin(out, 13);
    std::vector<std::uint8_t> town; u32(town, 9); str(town, "Fantasy Town"); u16(town, 123); u16(town, 234); town.push_back(7); props(out, town);
    nodeEnd(out);
    nodeEnd(out);

    nodeBegin(out, 15);
    nodeBegin(out, 16);
    std::vector<std::uint8_t> waypoint; str(waypoint, "Depot"); u16(waypoint, 321); u16(waypoint, 432); waypoint.push_back(7); props(out, waypoint);
    nodeEnd(out);
    nodeEnd(out);

    nodeEnd(out); // map data
    nodeEnd(out); // root
    return out;
}

void writeFile(const fs::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    assert(f.good()); f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); assert(f.good());
}
}

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-otbm-reader-tests";
    fs::create_directories(root);
    const auto path = root / "synthetic.otbm";
    writeFile(path, makeMap());

    const OtbmReader reader(path);
    const auto& r = reader.result();
    assert(r.header.formatVersion == 2U);
    assert(r.header.width == 512U && r.header.height == 512U);
    assert(r.header.itemsMajorVersion == 3U && r.header.itemsMinorVersion == 57U);
    assert(r.metadata.description == "Fantasy synthetic 10.98");
    assert(r.metadata.spawnFile == "map-spawn.xml");
    assert(r.metadata.houseFile == "map-house.xml");
    assert(r.import.model.tiles.size() == 2U);
    assert(r.import.diagnostics.tileCount == 2U);
    assert(r.import.diagnostics.itemCount == 4U);
    assert(r.import.model.tiles[0].position.x == 101 && r.import.model.tiles[0].position.y == 202 && r.import.model.tiles[0].position.z == 7);
    assert(r.import.model.tiles[0].items[0].serverId == 100U);

    const auto& parent = r.import.model.tiles[0].items[1];
    assert(parent.serverId == 101U);
    assert(parent.attributes.at("actionId") == "450");
    assert(parent.attributes.at("text") == "hello");
    assert(parent.attributes.at("runeCharges") == "7");
    assert(parent.attributes.at("duration") == "-500");
    assert(parent.attributes.at("decayingState") == "1");
    assert(parent.attributes.at("writtenDate") == "123456");
    assert(parent.attributes.at("writtenBy") == "Fantasy Writer");
    assert(parent.attributes.at("sleeperGuid") == "42");
    assert(parent.attributes.at("sleepStart") == "98765");
    assert(parent.attributes.at("charges") == "55");
    assert(parent.contents.size() == 1U);
    assert(parent.contents[0].serverId == 103U);
    assert(parent.contents[0].attributes.at("count") == "4");

    assert(r.import.model.tiles[1].houseId.has_value() && *r.import.model.tiles[1].houseId == 77U);
    assert(r.import.model.towns.size() == 1U);
    assert(r.import.model.towns[0].name == "Fantasy Town");
    assert(r.import.model.towns[0].templePosition.x == 123);
    assert(r.import.model.waypoints.size() == 1U);
    assert(r.import.diagnostics.waypointCount == 1U);
    assert(r.import.model.waypoints[0].name == "Depot");
    assert(r.import.model.waypoints[0].position.x == 321 && r.import.model.waypoints[0].position.y == 432 && r.import.model.waypoints[0].position.z == 7);

    std::error_code ignored; fs::remove_all(root, ignored);
    return 0;
}
