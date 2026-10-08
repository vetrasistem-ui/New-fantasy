#include "MapEngine/Core/MapDocument.hpp"
#include "MapEngine/Export/LegacyOtbmWriter.hpp"
#include "Runtime/Tfs1098RuntimeBackend.hpp"
#include "Runtime/Tfs1098RuntimeProfile.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
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

void writeText(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    assert(output.good());
    output << value;
    assert(output.good());
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-tfs1098-server-workflow";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    const fs::path projectRoot = root / "project";
    const fs::path runtimeTemplate = root / "tfs-template";
    const fs::path legacyInputs = root / "legacy";
    const fs::path exportDirectory = projectRoot / "build" / "export" / "tfs1098";

    MapDocument document;
    document.metadata().width = 512;
    document.metadata().height = 512;
    document.metadata().name = "Workflow Test";
    document.metadata().houseFile = "map-house.xml";
    document.metadata().spawnFile = "map-spawn.xml";
    auto& tile = document.map().ensureTile(Position{100, 100, 7});
    tile.ground = Item{100, 100, 1};

    Tfs1098TargetConfig profile;
    profile.mapName = "workflow_test";
    profile.outputDirectory = fs::path{"build"} / "runtime" / "tfs1098";
    Tfs1098RuntimeProfile::save(projectRoot, profile);
    const auto reopenedProfile = Tfs1098RuntimeProfile::load(projectRoot);
    assert(reopenedProfile.mapName == profile.mapName);
    assert(reopenedProfile.outputDirectory == profile.outputDirectory);

    const fs::path exportedMap = exportDirectory / (profile.mapName + ".otbm");
    LegacyOtbmWriter writer;
    LegacyOtbmWriterConfig writerConfig;
    writerConfig.overwrite = true;
    const auto writeReport = writer.write(exportedMap, document, writerConfig);
    assert(writeReport.success);
    assert(writeReport.tileCount == 1);
    assert(fs::is_regular_file(exportedMap));

    writeText(runtimeTemplate / "config.lua.dist",
        "serverName = \"Fantasy Workflow\"\n"
        "mapName = \"forgotten\"\n");
    writeText(runtimeTemplate / "data" / "marker.txt", "runtime-template\n");
    writeText(legacyInputs / "items.otb", "synthetic-10.98-otb\n");
    writeText(legacyInputs / "map-house.xml", "<houses/>\n");
    writeText(legacyInputs / "map-spawn.xml", "<spawns/>\n");

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
    assert(packageReport.success);
    assert(packageReport.errors.empty());
    assert(backend.status().state == RuntimeState::Stopped);

    const fs::path runtimeRoot = request.outputDirectory;
    assert(fs::is_regular_file(runtimeRoot / "data" / "world" / "workflow_test.otbm"));
    assert(fs::is_regular_file(runtimeRoot / "data" / "world" / "map-house.xml"));
    assert(fs::is_regular_file(runtimeRoot / "data" / "world" / "map-spawn.xml"));
    assert(fs::is_regular_file(runtimeRoot / "data" / "items" / "items.otb"));
    assert(fs::is_regular_file(runtimeRoot / "config.lua"));

    std::ifstream config(runtimeRoot / "config.lua", std::ios::binary);
    const std::string configText(
        std::istreambuf_iterator<char>(config),
        std::istreambuf_iterator<char>());
    assert(configText.find("mapName = \"workflow_test\"") != std::string::npos);

    fs::remove_all(root, ignored);
    return 0;
}
