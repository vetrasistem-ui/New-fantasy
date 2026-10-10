#include "MapAtlasExporter.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    using namespace fantasy::studio::mapcore;
    namespace fs = std::filesystem;
    try {
        if (argc != 2) throw std::runtime_error("provide an empty test output directory");
        const fs::path root = argv[1];
        if (fs::exists(root) && !fs::is_empty(root)) throw std::runtime_error("test directory must be empty");
        fs::create_directories(root);
        MapDocument document;
        document.metadata().width = 64;
        document.metadata().height = 64;
        document.metadata().name = "synthetic";
        Tile t;
        t.position = {3,4,7}; t.houseId = 9; t.flags = 42;
        Item ground; ground.serverId = 100; ground.clientId = 200;
        t.ground = ground;
        Item container; container.serverId = 101; container.clientId = 201;
        Item child; child.serverId = 102; child.clientId = 202;
        child.attributes["note"] = std::string("comma,quote\"\nline");
        child.attributes["integer"] = std::int64_t{-7};
        child.attributes["position"] = Position{8,9,7};
        container.contents.push_back(child);
        t.items.push_back(container);
        document.map().setTile(t);
        Tile neighbor; neighbor.position = {4,4,7}; neighbor.houseId = 9; neighbor.ground = ground;
        document.map().setTile(neighbor);
        Tile south; south.position = {3,5,7}; south.ground = ground;
        document.map().setTile(south);
        House h; h.id=9; h.name="quoted,house"; document.map().houses()[9]=h;
        SpawnArea area; area.center={4,4,7}; area.radius=2;
        SpawnEntry entry; entry.kind=SpawnEntryKind::MonsterSet; entry.position={4,4,7};
        entry.monsters.push_back({"test",100}); area.entries.push_back(entry);
        document.map().spawnAreas().push_back(area);
        fantasy::assets::FantasyAssetRegistry registry;
        std::string error;
        if (!registry.registerLegacyAsset({"test.ground",fantasy::assets::LegacyAssetKind::Ground,100,200,{1}},&error)) throw std::runtime_error(error);
        LegacyMapProjectConfig config;
        config.otbmPath=root/"map.otbm"; config.otbPath=root/"items.otb";
        config.datPath=root/"Tibia.dat"; config.sprPath=root/"Tibia.spr";
        config.houseXmlPath=root/"house.xml"; config.spawnXmlPath=root/"spawn.xml";
        for (const auto& name : {"map.otbm","items.otb","Tibia.dat","Tibia.spr","house.xml","spawn.xml"})
            std::ofstream(root/name) << "synthetic";
        const auto report = fantasy::atlas::MapAtlasExporter{}.exportMap(document,registry,config,root/"raw");
        if (report.tiles!=3 || report.items!=5 || report.assets!=3) throw std::runtime_error("synthetic counts mismatch");
        // A second export must be byte-identical despite unordered canonical storage.
        fantasy::atlas::MapAtlasExporter{}.exportMap(document,registry,config,root/"raw-copy");
        for (const auto& file : fs::directory_iterator(root/"raw")) {
            std::ifstream a(file.path(),std::ios::binary), b(root/"raw-copy"/file.path().filename(),std::ios::binary);
            const std::string left((std::istreambuf_iterator<char>(a)),{}), right((std::istreambuf_iterator<char>(b)),{});
            if (left!=right) throw std::runtime_error("nondeterministic export: "+file.path().filename().string());
        }
        std::cout << "ATLAS_SYNTHETIC PASS tiles=3 items=5 assets=3\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
