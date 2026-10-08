#include "MapEngine/Core/MapDocument.hpp"
#include "MapEngine/Export/LegacyOtbmWriter.hpp"
#include "Runtime/Tfs1098RuntimeBackend.hpp"
#include "Runtime/Tfs1098RuntimeProfile.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using fantasy::studio::mapcore::Item;
using fantasy::studio::mapcore::LegacyOtbmWriter;
using fantasy::studio::mapcore::LegacyOtbmWriterConfig;
using fantasy::studio::mapcore::MapDocument;
using fantasy::studio::mapcore::Position;
using fantasy::studio::runtime::RuntimePackageRequest;
using fantasy::studio::runtime::RuntimeState;
using fantasy::studio::runtime::Tfs1098RuntimeBackend;
using fantasy::studio::runtime::Tfs1098RuntimeProfile;
using fantasy::studio::runtime::Tfs1098TargetConfig;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void mark(const char* stage) {
    std::cout << "TFS1098_WORKFLOW " << stage << std::endl;
}

void writeText(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "unable to open synthetic fixture: " + path.string());
    output << value;
    require(output.good(), "unable to write synthetic fixture: " + path.string());
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-tfs1098-server-workflow";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    try {
        const fs::path projectRoot = root / "project";
        const fs::path runtimeTemplate = root / "tfs-template";
        const fs::path legacyInputs = root / "legacy";
        const fs::path exportDirectory = projectRoot / "build" / "export" / "tfs1098";

        mark("MAP_DOCUMENT");
        MapDocument document;
        document.metadata().width = 512;
        document.metadata().height = 512;
        document.metadata().name = "Workflow Test";
        document.metadata().houseFile = "map-house.xml";
        document.metadata().spawnFile = "map-spawn.xml";
        auto& tile = document.map().ensureTile(Position{100, 100, 7});
        tile.ground = Item{100, 100, 1};
        require(document.map().tileCount() == 1, "canonical test map should contain exactly one tile");

        mark("PROFILE_SAVE_LOAD");
        Tfs1098TargetConfig profile;
        profile.mapName = "workflow_test";
        profile.outputDirectory = fs::path{"build"} / "runtime" / "tfs1098";
        Tfs1098RuntimeProfile::save(projectRoot, profile);
        const auto reopenedProfile = Tfs1098RuntimeProfile::load(projectRoot);
        require(reopenedProfile.mapName == profile.mapName, "reopened mapName differs from saved profile");
        require(reopenedProfile.outputDirectory == profile.outputDirectory, "reopened outputDirectory differs from saved profile");

        mark("OTBM_EXPORT");
        const fs::path exportedMap = exportDirectory / (profile.mapName + ".otbm");
        LegacyOtbmWriter writer;
        LegacyOtbmWriterConfig writerConfig;
        writerConfig.overwrite = true;
        const auto writeReport = writer.write(exportedMap, document, writerConfig);
        require(writeReport.success,
            writeReport.errors.empty() ? "OTBM writer failed without error detail" : writeReport.errors.front());
        require(writeReport.tileCount == 1, "OTBM writer did not preserve the synthetic tile count");
        require(fs::is_regular_file(exportedMap), "OTBM writer did not create the exported map");

        mark("SYNTHETIC_RUNTIME");
        writeText(runtimeTemplate / "config.lua.dist",
            "serverName = \"Fantasy Workflow\"\n"
            "mapName = \"forgotten\"\n");
        writeText(runtimeTemplate / "data" / "marker.txt", "runtime-template\n");
        writeText(legacyInputs / "items.otb", "synthetic-10.98-otb\n");
        writeText(legacyInputs / "map-house.xml", "<houses/>\n");
        writeText(legacyInputs / "map-spawn.xml", "<spawns/>\n");

        mark("PACKAGE");
        Tfs1098RuntimeBackend backend;
        RuntimePackageRequest request;
        request.projectRoot = projectRoot;
        request.outputDirectory = Tfs1098RuntimeProfile::resolveOutputDirectory(projectRoot, reopenedProfile);
        request.runtimeTemplateDirectory = runtimeTemplate;
        request.exportedMapPath = exportedMap;
        request.itemsOtbPath = legacyInputs / "items.otb";
        request.houseXmlPath = legacyInputs / "map-house.xml";
        request.spawnXmlPath = legacyInputs / "map-spawn.xml";
        request.mapName = reopenedProfile.mapName;

        const auto packageReport = backend.packageProject(request);
        require(packageReport.success,
            packageReport.errors.empty() ? "TFS1098 package failed without error detail" : packageReport.errors.front());
        require(packageReport.errors.empty(), "TFS1098 package reported errors despite success");
        require(backend.status().state == RuntimeState::Stopped, "packaged runtime should be in Stopped state");

        mark("VERIFY_LAYOUT");
        const fs::path runtimeRoot = request.outputDirectory;
        require(fs::is_regular_file(runtimeRoot / "data" / "world" / "workflow_test.otbm"), "packaged OTBM missing");
        require(fs::is_regular_file(runtimeRoot / "data" / "world" / "map-house.xml"), "packaged house XML missing");
        require(fs::is_regular_file(runtimeRoot / "data" / "world" / "map-spawn.xml"), "packaged spawn XML missing");
        require(fs::is_regular_file(runtimeRoot / "data" / "items" / "items.otb"), "packaged items.otb missing");
        require(fs::is_regular_file(runtimeRoot / "config.lua"), "packaged config.lua missing");

        std::ifstream config(runtimeRoot / "config.lua", std::ios::binary);
        require(config.good(), "unable to open packaged config.lua");
        const std::string configText{
            std::istreambuf_iterator<char>{config},
            std::istreambuf_iterator<char>{}};
        require(configText.find("mapName = \"workflow_test\"") != std::string::npos,
            "packaged config.lua does not contain the target mapName");
        config.close();

        mark("PASS");
        fs::remove_all(root, ignored);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "TFS1098_WORKFLOW FAIL: " << error.what() << std::endl;
        fs::remove_all(root, ignored);
        return 1;
    }
}
