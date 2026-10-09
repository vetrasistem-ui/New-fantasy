#include "MapEngine/Core/MapDocument.hpp"
#include "MapEngine/Export/LegacyOtbmWriter.hpp"
#include "Runtime/Tfs1098OtcExtendedAdapter.hpp"
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
using fantasy::studio::runtime::SystemChannelId;
using fantasy::studio::runtime::Tfs1098CompatibilityProfile;
using fantasy::studio::runtime::Tfs1098OtcExtendedAdapter;
using fantasy::studio::runtime::Tfs1098OtcExtendedAdapterRequest;
using fantasy::studio::runtime::Tfs1098RuntimeBackend;
using fantasy::studio::runtime::Tfs1098RuntimeProfile;
using fantasy::studio::runtime::Tfs1098SystemChannelRegistry;
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

std::string readText(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "unable to read synthetic fixture: " + path.string());
    return std::string(std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
}

std::size_t countOccurrences(const std::string& text, const std::string& needle) {
    std::size_t count = 0;
    std::size_t offset = 0;
    while ((offset = text.find(needle, offset)) != std::string::npos) {
        ++count;
        offset += needle.size();
    }
    return count;
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
        require(
            reopenedProfile.compatibilityProfile == Tfs1098CompatibilityProfile::Vanilla,
            "default TFS1098 workflow must remain vanilla-compatible");

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
        writeText(
            runtimeTemplate / "data" / "creaturescripts" / "creaturescripts.xml",
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<creaturescripts>\n"
            "\t<event type=\"login\" name=\"PlayerLogin\" script=\"login.lua\" />\n"
            "</creaturescripts>\n");
        writeText(
            runtimeTemplate / "data" / "creaturescripts" / "scripts" / "login.lua",
            "function onLogin(player)\n"
            "\tplayer:registerEvent(\"PlayerDeath\")\n"
            "\treturn true\n"
            "end\n");
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

        const std::string configText = readText(runtimeRoot / "config.lua");
        require(configText.find("mapName = \"workflow_test\"") != std::string::npos,
            "packaged config.lua does not contain the target mapName");

        mark("OTC_EXTENDED_PROFILE");
        Tfs1098TargetConfig extendedProfile = reopenedProfile;
        extendedProfile.compatibilityProfile = Tfs1098CompatibilityProfile::OtcExtended;
        Tfs1098RuntimeProfile::save(projectRoot, extendedProfile);
        const auto reopenedExtendedProfile = Tfs1098RuntimeProfile::load(projectRoot);
        require(
            reopenedExtendedProfile.compatibilityProfile == Tfs1098CompatibilityProfile::OtcExtended,
            "OTC extended compatibility profile did not persist");

        const fs::path clientRoot = projectRoot / "build" / "client" / "otcv8";
        fs::create_directories(clientRoot);

        Tfs1098OtcExtendedAdapterRequest extendedRequest;
        extendedRequest.projectRoot = projectRoot;
        extendedRequest.runtimeDirectory = runtimeRoot;
        extendedRequest.clientDirectory = clientRoot;
        extendedRequest.requiredChannels = {
            SystemChannelId::parse("ui.inventory"),
            SystemChannelId::parse("system.quest:v1"),
        };

        mark("OTC_EXTENDED_APPLY");
        const auto extendedReport = Tfs1098OtcExtendedAdapter::apply(extendedRequest);
        require(
            extendedReport.success,
            extendedReport.errors.empty()
                ? "TFS1098 OTC extended adapter failed without error detail"
                : extendedReport.errors.front());
        require(extendedReport.bindings.size() == 2U, "OTC extended adapter did not bind both semantic channels");
        require(
            extendedReport.bindings[0].channel.value == "ui.inventory" &&
                extendedReport.bindings[0].wireCode == Tfs1098SystemChannelRegistry::MinFantasyOpcode,
            "ui.inventory was not assigned the first Fantasy-owned opcode");
        require(
            extendedReport.bindings[1].channel.value == "system.quest:v1" &&
                extendedReport.bindings[1].wireCode == Tfs1098SystemChannelRegistry::MinFantasyOpcode + 1U,
            "system.quest:v1 was not assigned the next Fantasy-owned opcode");

        const fs::path channelRegistry = Tfs1098SystemChannelRegistry::pathForProject(projectRoot);
        const fs::path serverBridge = runtimeRoot / "data" / "creaturescripts" / "scripts" / "extendedopcode.lua";
        const fs::path creatureScripts = runtimeRoot / "data" / "creaturescripts" / "creaturescripts.xml";
        const fs::path loginScript = runtimeRoot / "data" / "creaturescripts" / "scripts" / "login.lua";
        const fs::path clientBridge = clientRoot / "mods" / "fantasy_runtime_bridge" / "fantasy_runtime_bridge.lua";
        const fs::path clientModule = clientRoot / "mods" / "fantasy_runtime_bridge" / "fantasy_runtime_bridge.otmod";

        require(fs::is_regular_file(channelRegistry), "OTC extended semantic channel registry missing");
        require(fs::is_regular_file(serverBridge), "OTC extended TFS Lua bridge missing");
        require(fs::is_regular_file(clientBridge), "OTC extended OTClient Lua bridge missing");
        require(fs::is_regular_file(clientModule), "OTC extended OTClient module manifest missing");

        const std::string registryText = readText(channelRegistry);
        require(registryText.find("\"profile\": \"otc_extended\"") != std::string::npos,
            "semantic channel registry profile is not otc_extended");
        require(registryText.find("\"channel\": \"ui.inventory\", \"opcode\": 200") != std::string::npos,
            "semantic channel registry is missing ui.inventory -> 200");
        require(registryText.find("\"channel\": \"system.quest:v1\", \"opcode\": 201") != std::string::npos,
            "semantic channel registry is missing system.quest:v1 -> 201");

        const std::string serverBridgeText = readText(serverBridge);
        require(serverBridgeText.find("FANTASY/") != std::string::npos, "TFS bridge is missing Fantasy envelope prefix");
        require(serverBridgeText.find("[200] = 'ui.inventory'") != std::string::npos,
            "TFS bridge is missing ui.inventory mapping");
        require(serverBridgeText.find("[201] = 'system.quest:v1'") != std::string::npos,
            "TFS bridge is missing system.quest:v1 mapping");
        require(serverBridgeText.find("player:sendExtendedOpcode") != std::string::npos,
            "TFS bridge is missing outbound Extended Opcode support");

        const std::string clientBridgeText = readText(clientBridge);
        require(clientBridgeText.find("ProtocolGame.registerExtendedOpcode(200") != std::string::npos,
            "OTClient bridge is missing ui.inventory registration");
        require(clientBridgeText.find("ProtocolGame.registerExtendedOpcode(201") != std::string::npos,
            "OTClient bridge is missing system.quest:v1 registration");
        require(clientBridgeText.find("protocol:sendExtendedOpcode") != std::string::npos,
            "OTClient bridge is missing outbound Extended Opcode support");

        const std::string creatureScriptsText = readText(creatureScripts);
        require(creatureScriptsText.find("type=\"extendedopcode\"") != std::string::npos,
            "creaturescripts.xml is missing Extended Opcode event type");
        require(creatureScriptsText.find("name=\"ExtendedOpcode\"") != std::string::npos,
            "creaturescripts.xml is missing ExtendedOpcode event name");
        require(countOccurrences(creatureScriptsText, "name=\"ExtendedOpcode\"") == 1U,
            "creaturescripts.xml contains duplicate ExtendedOpcode events");

        const std::string loginScriptText = readText(loginScript);
        require(loginScriptText.find("player:registerEvent(\"ExtendedOpcode\")") != std::string::npos,
            "login.lua does not register the ExtendedOpcode creature event");
        require(countOccurrences(loginScriptText, "registerEvent(\"ExtendedOpcode\")") == 1U,
            "login.lua contains duplicate ExtendedOpcode registration");

        mark("OTC_EXTENDED_IDEMPOTENCE");
        const auto secondExtendedReport = Tfs1098OtcExtendedAdapter::apply(extendedRequest);
        require(secondExtendedReport.success, "second OTC extended adapter pass failed");
        require(secondExtendedReport.bindings == extendedReport.bindings,
            "OTC extended adapter changed stable channel bindings on second pass");
        require(countOccurrences(readText(creatureScripts), "name=\"ExtendedOpcode\"") == 1U,
            "second OTC extended adapter pass duplicated creaturescripts event");
        require(countOccurrences(readText(loginScript), "registerEvent(\"ExtendedOpcode\")") == 1U,
            "second OTC extended adapter pass duplicated login registration");

        mark("PASS");
        fs::remove_all(root, ignored);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "TFS1098_WORKFLOW FAIL: " << error.what() << std::endl;
        fs::remove_all(root, ignored);
        return 1;
    }
}
