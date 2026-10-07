#include "Shared/Formats/Legacy/LegacyAuxXmlReader.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace fantasy::legacy;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::filesystem::path writeFixture(const std::string& name, const std::string& content) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
    out.close();
    return path;
}

} // namespace

int main() {
    LegacyMapImportModel model;

    LegacyImportedTile houseTile;
    houseTile.position = {100, 100, 7};
    houseTile.houseId = 7;
    model.tiles.push_back(houseTile);

    const auto houseFile = writeFixture(
        "fantasy-house-fixture.xml",
        "<?xml version=\"1.0\"?>\n"
        "<houses>\n"
        "  <house name=\"House Seven\" houseid=\"7\" entryx=\"101\" entryy=\"102\" entryz=\"7\" rent=\"2500\" townid=\"3\" guildhall=\"1\"/>\n"
        "</houses>\n");

    const LegacyAuxXmlReport houseReport = LegacyAuxXmlReader::loadHouses(houseFile, model);
    require(houseReport.success, "house XML parses");
    require(houseReport.houseCount == 1, "one house imported");
    require(model.houses.size() == 1, "house stored in import model");
    require(model.houses[0].id == 7, "house id preserved");
    require(model.houses[0].name == "House Seven", "house name preserved");
    require(model.houses[0].entry.x == 101 && model.houses[0].entry.y == 102 && model.houses[0].entry.z == 7, "house entry preserved");
    require(model.houses[0].rent == 2500, "house rent preserved");
    require(model.houses[0].townId == 3, "house town preserved");
    require(model.houses[0].guildhall, "guildhall flag preserved");

    const auto spawnFile = writeFixture(
        "fantasy-spawn-fixture.xml",
        "<?xml version=\"1.0\"?>\n"
        "<spawns>\n"
        "  <spawn centerx=\"500\" centery=\"600\" centerz=\"7\" radius=\"5\">\n"
        "    <monster name=\"Rat\" x=\"-2\" y=\"1\" spawntime=\"30\" direction=\"2\"/>\n"
        "    <npc name=\"Guide\" x=\"1\" y=\"0\" direction=\"1\"/>\n"
        "    <monsters x=\"0\" y=\"2\" spawntime=\"45\">\n"
        "      <monster name=\"Dragon\" chance=\"70\"/>\n"
        "      <monster name=\"Dragon Lord\" chance=\"30\"/>\n"
        "    </monsters>\n"
        "  </spawn>\n"
        "</spawns>\n");

    const LegacyAuxXmlReport spawnReport = LegacyAuxXmlReader::loadSpawns(spawnFile, model);
    require(spawnReport.success, "spawn XML parses");
    require(spawnReport.spawnAreaCount == 1, "one spawn area imported");
    require(spawnReport.spawnEntryCount == 3, "three spawn entries imported");
    require(model.spawns.size() == 1, "spawn area stored in import model");

    const LegacyImportedSpawn& spawn = model.spawns.front();
    require(spawn.center.x == 500 && spawn.center.y == 600 && spawn.center.z == 7, "spawn center preserved");
    require(spawn.radius == 5, "spawn radius preserved");
    require(spawn.entries.size() == 3, "spawn entries preserved");

    const auto& rat = spawn.entries[0];
    require(rat.kind == LegacySpawnEntryKind::Monster, "monster entry kind");
    require(rat.name == "Rat", "monster name");
    require(rat.position.x == 498 && rat.position.y == 601, "signed relative spawn offset");
    require(rat.spawnTimeSeconds == 30 && rat.direction == 2, "monster spawn properties");

    const auto& npc = spawn.entries[1];
    require(npc.kind == LegacySpawnEntryKind::Npc && npc.name == "Guide", "npc entry preserved");
    require(npc.position.x == 501 && npc.position.y == 600, "npc relative position");

    const auto& set = spawn.entries[2];
    require(set.kind == LegacySpawnEntryKind::MonsterSet, "monster set kind");
    require(set.position.x == 500 && set.position.y == 602, "monster set position");
    require(set.monsters.size() == 2, "monster set options preserved");
    require(set.monsters[0].name == "Dragon" && set.monsters[0].chance == 70, "first monster-set option");
    require(set.monsters[1].name == "Dragon Lord" && set.monsters[1].chance == 30, "second monster-set option");

    std::error_code ec;
    std::filesystem::remove(houseFile, ec);
    std::filesystem::remove(spawnFile, ec);

    std::cout << "Legacy auxiliary XML tests PASS\n";
    return 0;
}
