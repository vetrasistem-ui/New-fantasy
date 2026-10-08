#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fantasy::studio::runtime {

enum class RuntimeKind {
    Tfs1098,
    FantasyNative,
};

struct RuntimeCapabilities {
    bool canPackageProject = false;
    bool canLaunch = false;
    bool canStop = false;
    bool canStreamLogs = false;
};

struct RuntimePackageRequest {
    std::filesystem::path projectRoot;
    std::filesystem::path outputDirectory;
};

struct RuntimePackageReport {
    bool success = false;
    std::vector<std::filesystem::path> generatedFiles;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class RuntimeBackend {
public:
    virtual ~RuntimeBackend() = default;

    [[nodiscard]] virtual RuntimeKind kind() const noexcept = 0;
    [[nodiscard]] virtual const char* id() const noexcept = 0;
    [[nodiscard]] virtual const char* displayName() const noexcept = 0;
    [[nodiscard]] virtual RuntimeCapabilities capabilities() const noexcept = 0;

    [[nodiscard]] virtual RuntimePackageReport packageProject(
        const RuntimePackageRequest& request) = 0;
};

} // namespace fantasy::studio::runtime
