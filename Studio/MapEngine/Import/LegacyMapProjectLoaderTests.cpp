#include "LegacyMapProjectLoader.hpp"
#include "LegacyWorkspaceSession.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace fantasy::studio::mapcore;
namespace fs = std::filesystem;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

void str(std::vector<std::uint8_t>& out, const std::string& value) {
    u16(out, static_cast<std::uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

void escaped(std::vector<std::uint8_t>& out, std::uint8_t value) {
    if (value >= 0xFDU) out.push_back(0xFDU);
    out.push_back(value);
}

void props(std::vector<std::uint8_t>& out, const std::vector<std::uint8_t>& data) {
    for (const auto value : data) escaped(out, value);
}

void nodeBegin(std::vector<std::uint8_t>& out, std::uint8_t type) {
    out.push_back(0xFEU);
    out.push_back(type);
}

void nodeEnd(std::vector<std::uint8_t>& out) {
    out.push_back(0xFFU);
}

void writeBinary(const fs::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    require(stream.good(), "open binary fixture");
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(stream.good(), "write binary fixture");
}

void writeText(const fs::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    require(stream.good(), "open text fixture");
    stream << text;
    require(stream.good(), "write text fixture");
}

std::vector<std::uint8_t> makeSpr() {
    std::vector<std::uint8_t> bytes;
    u32(bytes, 0x57BBD603U);
    u32(bytes, 2U);
    u32(bytes, 0U);
    u32(bytes, 16U);
    bytes.push_back(255U); bytes.push_back(0U); bytes.push_back(255U);
    u16(bytes, 7U);
    u16(bytes, 1023U);
    u16(bytes, 1U);
    bytes.push_back(100U); bytes.push_back(200U); bytes.push_back(50U);
    return bytes;
}

std::vector<std::uint8_t> makeDat() {
    std::vector<std::uint8_t> bytes;
    u32(bytes, 0x000042A3U);
    u16(bytes, 203U);
    u16(bytes, 0U);
    u16(bytes, 0U);
    u16(bytes, 0U);

    for (std::uint32_t id = 100; id <= 203; ++id) {
        bytes.push_back(0xFFU);
        bytes.push_back(1U);
        bytes.push_back(1U);
        bytes.push_back(1U);
        bytes.push_back(1U);
        bytes.push_back(1U);
        bytes.push_back(1U);
        bytes.push_back(1U);
        u32(bytes, 2U);
    }
    return bytes;
}

void appendOtbItem(
    std::vector<std::uint8_t>& bytes,
    std::uint8_t group,
    std::uint16_t serverId,
    std::uint16_t clientId) {

    nodeBegin(bytes, group);
    std::vector<std::uint8_t> item;
    u32(item, 0U);
    item.push_back(0x10U); u16(item, 2U); u16(item, serverId);
    item.push_back(0x11U); u16(item, 2U); u16(item, clientId);
    props(bytes, item);
    nodeEnd(bytes);
}

std::vector<std::uint8_t> makeOtb() {
    std::vector<std::uint8_t> bytes(4, 0U);
    nodeBegin(bytes, 0U);

    std::vector<std::uint8_t> root(4, 0U);
    root.push_back(0x01U);
    u16(root, 140U);
    u32(root, 3U);
    u32(root, 57U);
    u32(root, 63U);
    std::string description = "OTB 3.57.63-10.98";
    root.insert(root.end(), description.begin(), description.end());
    root.resize(4U + 1U + 2U + 12U + 128U, 0U);
    props(bytes, root);

    appendOtbItem(bytes, 1U, 100U, 200U);
    appendOtbItem(bytes, 2U, 101U, 201U);
    appendOtbItem(bytes, 2U, 102U, 202U);
    appendOtbItem(bytes, 2U, 103U, 203U);
    nodeEnd(bytes);
    return bytes;
}

std::vector<std::uint8_t> makeOtbm() {
    std::vector<std::uint8_t> out{'O','T','B','M'};
    nodeBegin(out, 1U);
    std::vector<std::uint8_t> root;
    u32(root, 2U); u16(root, 512U); u16(root, 512U); u32(root, 3U); u32(root, 57U);
    props(out, root);

    nodeBegin(out, 2U);
    std::vector<std::uint8_t> header;
    header.push_back(1U); str(header, "Fantasy complete synthetic 10.98");
    header.push_back(11U); str(header, "map-spawn.xml");
    header.push_back(13U); str(header, "map-house.xml");
    props(out, header);

    nodeBegin(out, 4U);
    std::vector<std::uint8_t> area; u16(area, 100U); u16(area, 200U); area.push_back(7U); props(out, area);

    nodeBegin(out, 5U);
    std::vector<std::uint8_t> tile{1U, 2U, 9U}; u16(tile, 100U); props(out, tile);
    nodeBegin(out, 6U);
    std::vector<std::uint8_t> item; u16(item, 101U); item.push_back(4U); u16(item, 450U); item.push_back(6U); str(item, "hello"); props(out, item);
    nodeBegin(out, 6U);
    std::vector<std::uint8_t> nested; u16(nested, 103U); nested.push_back(15U); nested.push_back(4U); props(out, nested);
    nodeEnd(out);
    nodeEnd(out);
    nodeEnd(out);

    nodeBegin(out, 14U);
    std::vector<std::uint8_t> houseTile{3U, 4U}; u32(houseTile, 77U); houseTile.push_back(9U); u16(houseTile, 102U); props(out, houseTile);
    nodeEnd(out);
    nodeEnd(out);

    nodeBegin(out, 12U);
    nodeBegin(out, 13U);
    std::vector<std::uint8_t> town; u32(town, 9U); str(town, "Fantasy Town"); u16(town, 123U); u16(town, 234U); town.push_back(7U); props(out, town);
    nodeEnd(out);
    nodeEnd(out);

    nodeBegin(out, 15U);
    nodeBegin(out, 16U);
    std::vector<std::uint8_t> waypoint; str(waypoint, "Depot"); u16(waypoint, 321U); u16(waypoint, 432U); waypoint.push_back(7U); props(out, waypoint);
    nodeEnd(out);
    nodeEnd(out);

    nodeEnd(out);
    nodeEnd(out);
    return out;
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-complete-1098-loader-tests";
    fs::create_directories(root);

    const fs::path otbm = root / "synthetic.otbm";
    const fs::path otb = root / "items.otb";
    const fs::path dat = root / "Tibia.dat";
    const fs::path spr = root / "Tibia.spr";
    const fs::path houses = root / "map-house.xml";
    const fs::path spawns = root / "map-spawn.xml";

    writeBinary(otbm, makeOtbm());
    writeBinary(otb, makeOtb());
    writeBinary(dat, makeDat());
    writeBinary(spr, makeSpr());
    writeText(houses,
        "<?xml version=\"1.0\"?>\n"
        "<houses><house name=\"House 77\" houseid=\"77\" entryx=\"104\" entryy=\"204\" entryz=\"7\" rent=\"2500\" townid=\"9\" guildhall=\"0\"/></houses>\n");
    writeText(spawns,
        "<?xml version=\"1.0\"?>\n"
        "<spawns><spawn centerx=\"101\" centery=\"202\" centerz=\"7\" radius=\"3\"><monster name=\"Rat\" x=\"1\" y=\"0\" spawntime=\"30\" direction=\"2\"/></spawn></spawns>\n");

    MapDocument document;
    LegacyMapProjectConfig config;
    config.profileId = "synthetic1098";
    config.otbmPath = otbm;
    config.otbPath = otb;
    config.datPath = dat;
    config.sprPath = spr;

    const LegacyMapProjectLoadResult loaded = LegacyMapProjectLoader{}.load(document, config);
    require(loaded.report.success, "complete 10.98 project load succeeds");
    require(loaded.report.errors.empty(), "complete 10.98 project has no errors");
    require(loaded.report.datSignature == 0x42A3U, "DAT signature preserved");
    require(loaded.report.sprSignature == 0x57BBD603U, "SPR signature preserved");
    require(loaded.report.sprCount == 2U, "SPR count preserved");
    require(loaded.report.assetRegistry.registered == 4U, "OTB registry maps all four items");
    require(document.map().tileCount() == 2U, "OTBM tiles reach canonical map");
    require(document.map().houses().size() == 1U, "house XML reaches canonical map");
    require(document.map().spawnAreas().size() == 1U, "spawn XML reaches canonical map");
    require(document.map().towns().size() == 1U, "town reaches canonical map");
    require(document.map().waypoints().size() == 1U, "waypoint reaches canonical map");

    const Tile* tile = document.map().findTile(Position{101, 202, 7});
    require(tile != nullptr, "normal tile loaded at absolute position");
    require(tile->ground.has_value(), "ground classified through OTB");
    require(tile->ground->serverId == 100U && tile->ground->clientId == 200U, "ground server/client ids resolved");
    require(tile->items.size() == 1U, "top-level object loaded");
    require(tile->items[0].serverId == 101U && tile->items[0].clientId == 201U, "object server/client ids resolved");
    require(tile->items[0].contents.size() == 1U, "nested container item loaded");
    require(tile->items[0].contents[0].serverId == 103U && tile->items[0].contents[0].clientId == 203U, "nested ids resolved");

    const Tile* houseTile = document.map().findTile(Position{103, 204, 7});
    require(houseTile != nullptr && houseTile->houseId == 77U, "house tile preserved");
    require(houseTile->items.size() == 1U && houseTile->items[0].clientId == 202U, "house item resolved");
    require(document.map().houses().at(77U).rent == 2500U, "house metadata preserved");
    require(document.map().spawnAreas().front().entries.front().name == "Rat", "spawn creature preserved");

    LegacyWorkspaceSession session;
    require(session.open(config), "workspace session opens complete project");
    require(session.ready(), "workspace session reports ready");
    require(session.document().map().tileCount() == 2U, "workspace session owns canonical map");
    require(session.assets().size() == 4U, "workspace session owns asset registry");
    require(session.view().center == Position{123, 234, 7}, "workspace starts at deterministic lowest town temple");
    require(session.view().floor == 7, "workspace starts on center floor");
    session.view().center = Position{1, 2, 3};
    session.view().floor = 3;
    session.resetView();
    require(session.view().center == Position{123, 234, 7}, "workspace reset restores initial center");
    require(session.view().floor == 7, "workspace reset restores initial floor");

    std::error_code ignored;
    fs::remove_all(root, ignored);
    std::cout << "Complete synthetic 10.98 project loader/session tests PASS\n";
    return 0;
}
