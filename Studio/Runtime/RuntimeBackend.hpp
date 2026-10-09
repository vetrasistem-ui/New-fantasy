#pragma once

#include "RuntimeMessaging.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fantasy::studio::runtime {

enum class RuntimeKind {
    Tfs1098,
    FantasyNative,
};

enum class RuntimeState {
    NotPrepared,
    Stopped,
    Starting,
    Running,
    Failed,
};

struct RuntimeCapabilities {
    bool canPackageProject = false;
    bool canLaunch = false;
    bool canStop = false;
    bool canStreamLogs = false;

    // Neutral Fantasy-owned system/runtime capabilities. These deliberately do
    // not expose protocol opcodes or TFS implementation details.
    bool canUseSystemChannels = false;
    bool canUseSemanticTags = false;
    bool canUseZones = false;
    bool canUseAppearanceExtensions = false;
};

struct RuntimePackageRequest {
    std::filesystem::path projectRoot;
    std::filesystem::path outputDirectory;

    // Optional external-runtime staging inputs. Generic backends may ignore them.
    std::filesystem::path runtimeTemplateDirectory;
    std::filesystem::path exportedMapPath;
    std::filesystem::path itemsOtbPath;
    std::filesystem::path houseXmlPath;
    std::filesystem::path spawnXmlPath;
    std::string mapName = "fantasy";
};

struct RuntimePackageReport {
    bool success = false;
    std::vector<std::filesystem::path> generatedFiles;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

struct RuntimeLaunchRequest {
    std::filesystem::path runtimeDirectory;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path logFile;
};

struct RuntimeLaunchReport {
    bool success = false;
    std::uint64_t processId = 0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

struct RuntimeStopReport {
    bool success = false;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

struct RuntimeStatus {
    RuntimeState state = RuntimeState::NotPrepared;
    std::uint64_t processId = 0;
    std::string message;
};

struct RuntimeLogChunk {
    std::uint64_t nextCursor = 0;
    std::string text;
    std::vector<std::string> errors;
};

class RuntimeBackend {
public:
    virtual ~RuntimeBackend() = default;

    [[nodiscard]] virtual RuntimeKind kind() const noexcept = 0;
    [[nodiscard]] virtual const char* id() const noexcept = 0;
    [[nodiscard]] virtual const char* displayName() const noexcept = 0;
    [[nodiscard]] virtual RuntimeCapabilities capabilities() const noexcept = 0;

    [[nodiscard]] virtual std::vector<RuntimeChannelBinding> systemChannelBindings() const {
        return {};
    }

    [[nodiscard]] virtual RuntimePackageReport packageProject(
        const RuntimePackageRequest& request) = 0;

    [[nodiscard]] virtual RuntimeLaunchReport launch(const RuntimeLaunchRequest&) {
        RuntimeLaunchReport report;
        report.errors.emplace_back("runtime launch is not supported by this backend");
        return report;
    }

    [[nodiscard]] virtual RuntimeStopReport stop() {
        RuntimeStopReport report;
        report.errors.emplace_back("runtime stop is not supported by this backend");
        return report;
    }

    [[nodiscard]] virtual RuntimeStatus status() const {
        return RuntimeStatus{RuntimeState::NotPrepared, 0, "runtime status is not supported by this backend"};
    }

    [[nodiscard]] virtual RuntimeLogChunk readLogs(std::uint64_t cursor) const {
        RuntimeLogChunk chunk;
        chunk.nextCursor = cursor;
        chunk.errors.emplace_back("runtime log streaming is not supported by this backend");
        return chunk;
    }
};

} // namespace fantasy::studio::runtime
