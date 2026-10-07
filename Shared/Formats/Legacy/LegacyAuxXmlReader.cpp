#include "Shared/Formats/Legacy/LegacyAuxXmlReader.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <set>
#include <string>

namespace fantasy::legacy {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

LegacyPosition relativePosition(
    const LegacyPosition& center,
    const pugi::xml_node& node) {

    return LegacyPosition{
        center.x + node.attribute("x").as_int(0),
        center.y + node.attribute("y").as_int(0),
        center.z
    };
}

bool hasMapHouseId(const LegacyMapImportModel& model, std::uint32_t houseId) {
    return std::any_of(model.tiles.begin(), model.tiles.end(), [houseId](const LegacyImportedTile& tile) {
        return tile.houseId.has_value() && *tile.houseId == houseId;
    });
}

} // namespace

LegacyAuxXmlReport LegacyAuxXmlReader::loadHouses(
    const std::filesystem::path& path,
    LegacyMapImportModel& model) {

    LegacyAuxXmlReport report;
    pugi::xml_document document;
    const pugi::xml_parse_result parsed = document.load_file(path.string().c_str());
    if (!parsed) {
        report.errors.push_back("Unable to parse house XML: " + std::string(parsed.description()));
        return report;
    }

    const pugi::xml_node root = document.child("houses");
    if (!root) {
        report.errors.push_back("House XML root <houses> is missing.");
        return report;
    }

    model.houses.clear();
    std::set<std::uint32_t> seenIds;

    for (const pugi::xml_node node : root.children()) {
        if (lower(node.name()) != "house") continue;

        const pugi::xml_attribute idAttribute = node.attribute("houseid");
        if (!idAttribute) {
            report.errors.push_back("House entry without houseid.");
            continue;
        }

        const std::uint32_t id = idAttribute.as_uint(0);
        if (id == 0) {
            report.errors.push_back("House entry has invalid houseid=0.");
            continue;
        }
        if (!seenIds.insert(id).second) {
            report.errors.push_back("Duplicate house id " + std::to_string(id) + ".");
            continue;
        }

        LegacyImportedHouse house;
        house.id = id;
        house.name = node.attribute("name").as_string();
        if (house.name.empty()) house.name = "House #" + std::to_string(id);
        house.entry = LegacyPosition{
            node.attribute("entryx").as_int(0),
            node.attribute("entryy").as_int(0),
            static_cast<std::int16_t>(node.attribute("entryz").as_int(0))
        };
        house.rent = node.attribute("rent").as_uint(0);
        house.townId = node.attribute("townid").as_uint(0);
        house.guildhall = node.attribute("guildhall").as_bool(false);

        if (!hasMapHouseId(model, id)) {
            report.warnings.push_back(
                "House id " + std::to_string(id) + " is not referenced by any OTBM house tile.");
        }
        if (house.townId == 0) {
            report.warnings.push_back("House id " + std::to_string(id) + " has no townid.");
        }

        model.houses.push_back(std::move(house));
        ++report.houseCount;
    }

    report.success = report.errors.empty();
    return report;
}

LegacyAuxXmlReport LegacyAuxXmlReader::loadSpawns(
    const std::filesystem::path& path,
    LegacyMapImportModel& model) {

    LegacyAuxXmlReport report;
    pugi::xml_document document;
    const pugi::xml_parse_result parsed = document.load_file(path.string().c_str());
    if (!parsed) {
        report.errors.push_back("Unable to parse spawn XML: " + std::string(parsed.description()));
        return report;
    }

    const pugi::xml_node root = document.child("spawns");
    if (!root) {
        report.errors.push_back("Spawn XML root <spawns> is missing.");
        return report;
    }

    model.spawns.clear();

    for (const pugi::xml_node spawnNode : root.children()) {
        if (lower(spawnNode.name()) != "spawn") continue;

        LegacyImportedSpawn spawn;
        spawn.center = LegacyPosition{
            spawnNode.attribute("centerx").as_int(0),
            spawnNode.attribute("centery").as_int(0),
            static_cast<std::int16_t>(spawnNode.attribute("centerz").as_int(0))
        };
        spawn.radius = spawnNode.attribute("radius").as_int(-1);

        if (spawn.center.x == 0 || spawn.center.y == 0) {
            report.errors.push_back("Spawn has invalid center position.");
            continue;
        }
        if (spawn.radius > 30) {
            report.warnings.push_back(
                "Spawn radius greater than 30 at " + std::to_string(spawn.center.x) + "," +
                std::to_string(spawn.center.y) + "," + std::to_string(spawn.center.z) + ".");
        }

        for (const pugi::xml_node entryNode : spawnNode.children()) {
            const std::string nodeName = lower(entryNode.name());
            if (nodeName != "monster" && nodeName != "npc" && nodeName != "monsters") continue;

            LegacyImportedSpawnEntry entry;
            entry.position = relativePosition(spawn.center, entryNode);
            entry.direction = static_cast<std::uint16_t>(entryNode.attribute("direction").as_uint(0));
            entry.spawnTimeSeconds = entryNode.attribute("spawntime").as_uint(0);

            if (nodeName == "monster" || nodeName == "npc") {
                entry.kind = nodeName == "npc" ? LegacySpawnEntryKind::Npc : LegacySpawnEntryKind::Monster;
                entry.name = entryNode.attribute("name").as_string();
                if (entry.name.empty()) {
                    report.warnings.push_back("Spawn creature entry without a name was preserved as an unnamed entry.");
                }
            } else {
                entry.kind = LegacySpawnEntryKind::MonsterSet;
                std::size_t optionCount = 0;
                for (const pugi::xml_node optionNode : entryNode.children()) {
                    if (optionNode.type() != pugi::node_element) continue;
                    ++optionCount;
                }

                for (const pugi::xml_node optionNode : entryNode.children()) {
                    if (optionNode.type() != pugi::node_element) continue;
                    LegacySpawnMonsterOption option;
                    option.name = optionNode.attribute("name").as_string();
                    option.chance = static_cast<std::uint16_t>(
                        optionNode.attribute("chance").as_uint(optionCount == 0 ? 0 : static_cast<unsigned int>(100 / optionCount)));
                    if (option.name.empty()) {
                        report.warnings.push_back("Monster-set option without a name was preserved as an unnamed option.");
                    }
                    entry.monsters.push_back(std::move(option));
                }

                if (entry.monsters.empty()) {
                    report.warnings.push_back("Empty <monsters> spawn set encountered.");
                }
            }

            spawn.entries.push_back(std::move(entry));
            ++report.spawnEntryCount;
        }

        if (spawn.entries.empty()) {
            report.warnings.push_back(
                "Empty spawn area at " + std::to_string(spawn.center.x) + "," +
                std::to_string(spawn.center.y) + "," + std::to_string(spawn.center.z) + ".");
        }

        model.spawns.push_back(std::move(spawn));
        ++report.spawnAreaCount;
    }

    report.success = report.errors.empty();
    return report;
}

} // namespace fantasy::legacy
