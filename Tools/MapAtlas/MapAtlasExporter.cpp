#include "MapAtlasExporter.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace fantasy::atlas {
namespace {
using namespace studio::mapcore;
namespace fs = std::filesystem;
using Id = std::uint64_t;
using Asset = std::pair<std::uint32_t, std::uint32_t>;

struct Csv {
    std::ofstream file;
    Csv(const fs::path& path, const char* header) : file(path, std::ios::binary) {
        if (!file) throw std::runtime_error("Cannot create Atlas CSV: " + path.string());
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << header << '\n';
    }
    template<class T> static std::string cell(const T& value) {
        std::ostringstream text;
        text << value;
        std::string escaped = "\"";
        for (const char c : text.str()) { if (c == '"') escaped += '"'; escaped += c; }
        return escaped + '"';
    }
    template<class... T> void row(const T&... values) {
        bool first = true;
        auto put = [&](const auto& value) { if (!first) file << ','; first = false; file << cell(value); };
        (put(values), ...);
        file << '\n';
    }
    void close() { file.close(); }
};

std::string key(const Asset& a) { return std::to_string(a.first) + ":" + std::to_string(a.second); }
std::string kind(assets::LegacyAssetKind k) {
    switch (k) {
    case assets::LegacyAssetKind::Ground: return "ground";
    case assets::LegacyAssetKind::Border: return "border";
    case assets::LegacyAssetKind::Object: return "object";
    case assets::LegacyAssetKind::Creature: return "creature";
    case assets::LegacyAssetKind::Effect: return "effect";
    default: return "unknown";
    }
}
std::set<Asset> spatialAssets(const Tile& tile) {
    std::set<Asset> ids;
    if (tile.ground) ids.emplace(tile.ground->serverId, tile.ground->clientId);
    for (const auto& item : tile.items) ids.emplace(item.serverId, item.clientId);
    return ids;
}
struct Stats {
    Id uses = 0, ground = 0, top = 0, nested = 0, house = 0, nonHouse = 0, tiles = 0;
    std::set<std::uint32_t> houses;
    Position first, min, max;
    std::uint32_t floors = 0;
};
struct PairCounts { Id total = 0, sameHouse = 0; };
using AdjKey = std::tuple<Asset, Asset, int, int>;

// Persist only relative source paths, resolved by Python against the raw folder.
std::string relative(const fs::path& path, const fs::path& root) {
    if (path.empty()) throw std::runtime_error("Atlas requires resolved source paths");
    return fs::relative(fs::absolute(path), fs::absolute(root)).generic_string();
}
}

AtlasExportReport MapAtlasExporter::exportMap(
    const studio::mapcore::MapDocument& document,
    const assets::FantasyAssetRegistry& registry,
    const studio::mapcore::LegacyMapProjectConfig& sources,
    const std::filesystem::path& directory) const {
    using namespace studio::mapcore;
    if (fs::exists(directory) && !fs::is_empty(directory))
        throw std::runtime_error("Atlas output must be empty; existing exports are never overwritten");
    const auto stage = fs::path(directory.string() + ".partial");
    if (fs::exists(stage)) throw std::runtime_error("Atlas partial output already exists: " + stage.string());
    fs::create_directories(stage);
    // On failure retain .partial diagnostics, but never publish a completed manifest.
    Csv tiles(stage / "tiles.csv", "tileId,x,y,z,houseId,flags,groundServerId,groundClientId,topLevelItemCount");
    Csv items(stage / "items.csv", "itemId,tileId,parentItemId,role,stackIndex,depth,serverId,clientId,countOrSubtype");
    Csv attrs(stage / "attributes.csv", "ownerType,ownerId,key,valueType,intValue,textValue,positionX,positionY,positionZ");
    Csv examples(stage / "asset-examples.csv", "serverId,clientId,x,y,z,houseId,role,stackIndex");
    Csv assetsCsv(stage / "assets.csv", "serverId,clientId,kind,totalUses,groundUses,topLevelUses,nestedUses,houseTileUses,nonHouseTileUses,uniqueTiles,uniqueHouses,firstX,firstY,firstZ,minX,minY,minZ,maxX,maxY,maxZ,floorMask");
    Csv adjacency(stage / "adjacency.csv", "assetA,assetB,dx,dy,dz,count,sameHouseCount");
    Csv cooccurrence(stage / "cooccurrence.csv", "assetA,assetB,sameTileCount,houseTileCount");
    Csv houses(stage / "houses.csv", "id,name,exitX,exitY,exitZ,rent,townId,guildhall");
    Csv towns(stage / "towns.csv", "id,name,templeX,templeY,templeZ");
    Csv waypoints(stage / "waypoints.csv", "name,x,y,z");
    Csv areas(stage / "spawn-areas.csv", "spawnAreaId,centerX,centerY,centerZ,radius");
    Csv entries(stage / "spawn-entries.csv", "spawnAreaId,entryIndex,kind,x,y,z,name,direction,intervalSeconds");
    Csv options(stage / "spawn-monster-options.csv", "spawnAreaId,entryIndex,optionIndex,name,chance");
    AtlasExportReport report;
    std::map<Asset, Stats> stats;
    std::map<Asset, std::size_t> exampleCounts;
    std::map<AdjKey, PairCounts> neighbors;
    std::map<std::pair<Asset, Asset>, PairCounts> pairs;
    Id nextItem = 0;
    auto attributes = [&](const char* owner, Id id, const AttributeMap& values) {
        for (const auto& [attributeKey, value] : values) {
            if (const auto* n = std::get_if<std::int64_t>(&value))
                attrs.row(owner, id, attributeKey, "int64", *n, "", "", "", "");
            else if (const auto* s = std::get_if<std::string>(&value))
                attrs.row(owner, id, attributeKey, "string", "", *s, "", "", "");
            else {
                const auto& p = std::get<Position>(value);
                attrs.row(owner, id, attributeKey, "Position", "", "", p.x, p.y, p.z);
            }
        }
    };
    auto exportTile = [&](const Tile& tile) {
        const Id tileId = ++report.tiles;
        const auto p = tile.position;
        if (p.x < 0 || p.x > 65535 || p.y < 0 || p.y > 65535 || p.z < 0 || p.z > 15)
            throw std::runtime_error("Atlas V1 coordinate outside OTBM domain");
        tiles.row(tileId, p.x, p.y, p.z, tile.houseId, tile.flags,
            tile.ground ? std::to_string(tile.ground->serverId) : "",
            tile.ground ? std::to_string(tile.ground->clientId) : "", tile.items.size());
        attributes("tile", tileId, tile.attributes);
        std::set<Asset> usedOnTile;
        std::function<void(const Item&, Id, const char*, std::size_t, std::size_t)> emit;
        emit = [&](const Item& item, Id parent, const char* role, std::size_t stack, std::size_t depth) {
            const Id itemId = ++nextItem;
            const Asset a{item.serverId, item.clientId};
            items.row(itemId, tileId, parent ? std::to_string(parent) : "", role,
                stack, depth, item.serverId, item.clientId, item.countOrSubtype);
            attributes("item", itemId, item.attributes);
            auto& s = stats[a];
            if (s.uses == 0) s.first = s.min = s.max = p;
            ++s.uses;
            if (depth > 0) ++s.nested;
            else if (std::string(role) == "ground") ++s.ground;
            else ++s.top;
            if (tile.houseId) { ++s.house; s.houses.insert(tile.houseId); } else ++s.nonHouse;
            s.min.x = std::min(s.min.x, p.x); s.min.y = std::min(s.min.y, p.y); s.min.z = std::min(s.min.z, p.z);
            s.max.x = std::max(s.max.x, p.x); s.max.y = std::max(s.max.y, p.y); s.max.z = std::max(s.max.z, p.z);
            s.floors |= 1U << static_cast<unsigned>(p.z);
            usedOnTile.insert(a);
            if (exampleCounts[a] < 50) {
                examples.row(a.first, a.second, p.x, p.y, p.z, tile.houseId, role, stack);
                ++exampleCounts[a];
            }
            std::size_t childStack = 0;
            for (const auto& child : item.contents) emit(child, itemId, "content", childStack++, depth + 1);
        };
        if (tile.ground) emit(*tile.ground, 0, "ground", 0, 0);
        std::size_t stack = 0;
        for (const auto& item : tile.items) emit(item, 0, "item", stack++, 0);
        for (const auto& a : usedOnTile) ++stats[a].tiles;
        const auto spatial = spatialAssets(tile);
        for (auto a = spatial.begin(); a != spatial.end(); ++a) {
            for (auto b = std::next(a); b != spatial.end(); ++b) {
                auto& count = pairs[{*a, *b}]; ++count.total;
                if (tile.houseId) ++count.sameHouse;
            }
        }
        for (const auto& [dx, dy] : std::array<std::pair<int,int>, 2>{{{1,0},{0,1}}}) {
            const auto* neighbor = document.map().findTile(Position{p.x + dx, p.y + dy, p.z});
            if (!neighbor) continue;
            const auto other = spatialAssets(*neighbor);
            for (const auto& a : spatial) for (const auto& b : other) {
                auto& count = neighbors[{a,b,dx,dy}]; ++count.total;
                if (tile.houseId && tile.houseId == neighbor->houseId) ++count.sameHouse;
            }
        }
    };
    // Sparse stripe traversal; bounded pointer buffer, sorted z/y/x for stable IDs/examples.
    // Full OTBM coordinate bounds also preserve tiles beyond advertised map dimensions.
    for (std::int16_t z = 0; z < 16; ++z) for (std::int32_t y = 0; y <= 65535; y += MapStorage::ChunkSize) {
        std::vector<const Tile*> stripe;
        document.map().forEachTileInRect(z, {0,y,65535,y + MapStorage::ChunkSize - 1},
            [&](const Tile& tile) { stripe.push_back(&tile); });
        std::sort(stripe.begin(), stripe.end(), [](const Tile* a, const Tile* b) {
            return std::tie(a->position.y, a->position.x) < std::tie(b->position.y, b->position.x);
        });
        for (const auto* tile : stripe) exportTile(*tile);
    }
    if (report.tiles != document.map().tileCount()) throw std::runtime_error("Atlas sparse traversal tile count mismatch");
    report.items = nextItem;
    report.assets = stats.size();
    for (const auto& [a,s] : stats) {
        const auto* record = registry.findByLegacyServerId(a.first);
        assetsCsv.row(a.first,a.second, record && record->clientId == a.second ? kind(record->kind) : "unknown",
            s.uses,s.ground,s.top,s.nested,s.house,s.nonHouse,s.tiles,s.houses.size(),
            s.first.x,s.first.y,s.first.z,s.min.x,s.min.y,s.min.z,s.max.x,s.max.y,s.max.z,s.floors);
    }
    for (const auto& [k,n] : neighbors) adjacency.row(key(std::get<0>(k)),key(std::get<1>(k)),std::get<2>(k),std::get<3>(k),0,n.total,n.sameHouse);
    for (const auto& [k,n] : pairs) cooccurrence.row(key(k.first),key(k.second),n.total,n.sameHouse);
    std::vector<std::uint32_t> ids;
    for (const auto& [id,h] : document.map().houses()) { (void)h; ids.push_back(id); }
    std::sort(ids.begin(),ids.end());
    for (const auto id : ids) { const auto& h = document.map().houses().at(id); houses.row(h.id,h.name,h.exit.x,h.exit.y,h.exit.z,h.rent,h.townId,h.guildhall ? 1 : 0); }
    ids.clear();
    for (const auto& [id,t] : document.map().towns()) { (void)t; ids.push_back(id); }
    std::sort(ids.begin(),ids.end());
    for (const auto id : ids) { const auto& t = document.map().towns().at(id); towns.row(t.id,t.name,t.templePosition.x,t.templePosition.y,t.templePosition.z); }
    std::vector<std::string> names;
    for (const auto& [name,w] : document.map().waypoints()) { (void)w; names.push_back(name); }
    std::sort(names.begin(),names.end());
    for (const auto& name : names) { const auto& w = document.map().waypoints().at(name); waypoints.row(w.name,w.position.x,w.position.y,w.position.z); }
    Id areaId = 0;
    for (const auto& area : document.map().spawnAreas()) {
        ++areaId;
        areas.row(areaId,area.center.x,area.center.y,area.center.z,area.radius);
        std::size_t entryIndex = 0;
        for (const auto& e : area.entries) {
            const char* entryKind = e.kind == SpawnEntryKind::Npc ? "npc" : e.kind == SpawnEntryKind::MonsterSet ? "monsterSet" : "monster";
            entries.row(areaId,entryIndex,entryKind,e.position.x,e.position.y,e.position.z,e.name,e.direction,e.intervalSeconds);
            std::size_t optionIndex = 0;
            for (const auto& option : e.monsters) options.row(areaId,entryIndex,optionIndex++,option.name,option.chance);
            ++entryIndex;
        }
    }
    for (auto* csv : {&tiles,&items,&attrs,&examples,&assetsCsv,&adjacency,&cooccurrence,&houses,&towns,&waypoints,&areas,&entries,&options}) csv->close();
    const auto aux = [&](const std::optional<fs::path>& overridePath, const std::string& metadataPath, const char* suffix) {
        if (overridePath) return *overridePath;
        const fs::path path = metadataPath.empty() ? sources.otbmPath.stem().string() + suffix : metadataPath;
        return path.is_absolute() ? path : sources.otbmPath.parent_path() / path;
    };
    const auto& m = document.metadata();
    nlohmann::json manifest{
        {"schemaVersion",1},{"profileId",sources.profileId},{"mapName",m.name},{"width",m.width},{"height",m.height},
        {"canonicalTileCount",report.tiles},{"itemCount",report.items},{"assetCount",report.assets},
        {"houseCount",document.map().houses().size()},{"spawnAreaCount",areaId},
        {"townCount",document.map().towns().size()},{"waypointCount",document.map().waypoints().size()},
        {"otbmPath",relative(sources.otbmPath,directory)},{"otbPath",relative(sources.otbPath,directory)},
        {"datPath",relative(sources.datPath,directory)},{"sprPath",relative(sources.sprPath,directory)},
        {"housePath",relative(aux(sources.houseXmlPath,m.houseFile,"-house.xml"),directory)},
        {"spawnPath",relative(aux(sources.spawnXmlPath,m.spawnFile,"-spawn.xml"),directory)},
        {"adjacencyDirections","E,S; unique assets per tile; nested contents excluded"},
        {"sameHouseCountDefinition","both tiles share the same nonzero houseId"},
        {"assetKeyFormat","serverId:clientId"},{"exampleOrder","z,y,x then ground/top-level stack/depth-first contents; first 50 uses"}
    };
    std::ofstream manifestFile(stage / "atlas-manifest.json",std::ios::binary);
    manifestFile.exceptions(std::ios::failbit | std::ios::badbit);
    manifestFile << manifest.dump(2) << '\n';
    manifestFile.close();
    if (fs::exists(directory)) fs::remove(directory); // only the verified empty destination
    fs::rename(stage,directory);
    return report;
}
}
