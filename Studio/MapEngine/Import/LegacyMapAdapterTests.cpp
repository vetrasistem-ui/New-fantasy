#include "LegacyMapAdapter.hpp"

#include <cstdlib>
#include <iostream>
#include <utility>

using namespace fantasy::studio::mapcore;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

fantasy::assets::LegacyAssetRecord asset(
    std::string key,
    fantasy::assets::LegacyAssetKind kind,
    std::uint32_t serverId,
    std::uint32_t clientId) {

    fantasy::assets::LegacyAssetRecord record;
    record.semanticKey = std::move(key);
    record.kind = kind;
    record.serverId = serverId;
    record.clientId = clientId;
    return record;
}

} // namespace

int main() {
    fantasy::assets::FantasyAssetRegistry assets;
    require(assets.registerLegacyAsset(asset("1098:100", fantasy::assets::LegacyAssetKind::Ground, 100, 200)), "register ground asset");
    require(assets.registerLegacyAsset(asset("1098:101", fantasy::assets::LegacyAssetKind::Object, 101, 201)), "register object asset");

    fantasy::legacy::OtbmReadResult source;
    source.header.formatVersion = 2;
    source.header.width = 512;
    source.header.height = 512;
    source.header.itemsMajorVersion = 3;
    source.header.itemsMinorVersion = 57;
    source.metadata.description = "Synthetic 10.98 map";
    source.metadata.spawnFile = "map-spawn.xml";
    source.metadata.houseFile = "map-house.xml";
    source.import.model.width = 512;
    source.import.model.height = 512;
    source.import.model.sourceMapName = "synthetic.otbm";
    source.import.model.sourceProfileId = "pokefans-1098";

    fantasy::legacy::LegacyImportedTile sourceTile;
    sourceTile.position = {100, 200, 7};
    sourceTile.houseId = 7;

    fantasy::legacy::LegacyImportedItem flags;
    flags.serverId = 0;
    flags.attributes["tileFlags"] = "3";
    sourceTile.items.push_back(flags);

    fantasy::legacy::LegacyImportedItem ground;
    ground.serverId = 100;
    sourceTile.items.push_back(ground);

    fantasy::legacy::LegacyImportedItem object;
    object.serverId = 101;
    object.attributes["actionId"] = "42";
    object.attributes["teleportDestination"] = "120,220,7";
    sourceTile.items.push_back(object);

    fantasy::legacy::LegacyImportedItem unresolved;
    unresolved.serverId = 999;
    sourceTile.items.push_back(unresolved);

    source.import.model.tiles.push_back(sourceTile);

    fantasy::legacy::LegacyImportedTown town;
    town.id = 1;
    town.name = "Center";
    town.templePosition = {110, 210, 7};
    source.import.model.towns.push_back(town);

    fantasy::legacy::LegacyImportedHouse house;
    house.id = 7;
    house.name = "House Seven";
    house.entry = {101, 200, 7};
    source.import.model.houses.push_back(house);

    fantasy::legacy::LegacyImportedSpawn spawn;
    spawn.position = {100, 200, 7};
    spawn.radius = 3;
    spawn.creatureName = "Rat";
    source.import.model.spawns.push_back(spawn);

    const LegacyMapAdaptResult adapted = LegacyMapAdapter{}.adapt(source, assets);
    require(adapted.metadata.width == 512 && adapted.metadata.height == 512, "metadata dimensions");
    require(adapted.metadata.name == "synthetic.otbm", "metadata source name");
    require(adapted.metadata.description == "Synthetic 10.98 map", "metadata description");
    require(adapted.map.tileCount() == 1, "adapted tile count");
    require(adapted.map.towns().size() == 1, "town imported");
    require(adapted.map.houses().size() == 1, "house imported");
    require(adapted.report.unresolvedServerIds.size() == 1 && adapted.report.unresolvedServerIds.front() == 999, "unknown server id reported");

    const Tile* tile = adapted.map.findTile(Position{100, 200, 7});
    require(tile != nullptr, "adapted tile exists");
    require(tile->ground.has_value(), "ground resolved by asset kind");
    require(tile->ground->serverId == 100 && tile->ground->clientId == 200, "ground ids resolved");
    require(tile->items.size() == 2, "object and unresolved item preserved");
    require(tile->items[0].serverId == 101 && tile->items[0].clientId == 201, "object ids resolved");
    require(tile->items[1].serverId == 999 && tile->items[1].clientId == 0, "unresolved item preserved losslessly by server id");
    require(tile->flags == 3, "tile flags imported");
    require(tile->houseId == 7, "house tile id imported");
    require(tile->spawn.has_value() && tile->spawn->radius == 3, "spawn imported");
    require(tile->creature.has_value() && tile->creature->name == "Rat", "spawn creature imported");

    const auto actionIt = tile->items[0].attributes.find("actionId");
    require(actionIt != tile->items[0].attributes.end(), "action id preserved");
    const auto* actionId = std::get_if<std::int64_t>(&actionIt->second);
    require(actionId && *actionId == 42, "action id typed as integer");

    const auto teleIt = tile->items[0].attributes.find("teleportDestination");
    require(teleIt != tile->items[0].attributes.end(), "teleport destination preserved");
    const auto* destination = std::get_if<Position>(&teleIt->second);
    require(destination && *destination == Position{120, 220, 7}, "teleport destination typed as position");

    MapDocument document;
    document.selection().add(Position{1, 1, 7});
    const LegacyMapAdaptReport loadReport = LegacyMapAdapter{}.load(document, source, assets);
    require(loadReport.unresolvedServerIds.size() == 1, "load report propagated");
    require(document.revision() == 0, "loading map starts clean revision");
    require(document.selection().empty(), "loading map clears selection");
    require(document.metadata().name == "synthetic.otbm", "document metadata loaded");
    require(document.map().findTile(Position{100, 200, 7}) != nullptr, "document map loaded");

    std::cout << "Legacy Map Adapter tests PASS\n";
    return 0;
}
