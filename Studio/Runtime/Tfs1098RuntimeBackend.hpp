#pragma once

#include "RuntimeBackend.hpp"

#include <cstring>
#include <memory>

namespace fantasy::studio::runtime {

class Tfs1098RuntimeBackend final : public RuntimeBackend {
public:
    Tfs1098RuntimeBackend();
    ~Tfs1098RuntimeBackend() override;

    Tfs1098RuntimeBackend(const Tfs1098RuntimeBackend&) = delete;
    Tfs1098RuntimeBackend& operator=(const Tfs1098RuntimeBackend&) = delete;

    [[nodiscard]] RuntimeKind kind() const noexcept override { return RuntimeKind::Tfs1098; }
    [[nodiscard]] const char* id() const noexcept override { return "tfs1098"; }
    [[nodiscard]] const char* displayName() const noexcept override { return "TFS 1.4.2 / 10.98"; }
    [[nodiscard]] RuntimeCapabilities capabilities() const noexcept override;

    [[nodiscard]] RuntimePackageReport packageProject(
        const RuntimePackageRequest& request) override;
    [[nodiscard]] RuntimeLaunchReport launch(
        const RuntimeLaunchRequest& request) override;
    [[nodiscard]] RuntimeStopReport stop() override;
    [[nodiscard]] RuntimeStatus status() const override;
    [[nodiscard]] RuntimeLogChunk readLogs(std::uint64_t cursor) const override;

private:
    struct ProcessState;

    [[nodiscard]] static bool validMapName(const std::string& value) noexcept;
    [[nodiscard]] static bool patchMapName(
        const std::filesystem::path& configPath,
        const std::string& mapName,
        std::string& error);

    std::filesystem::path preparedDirectory_;
    mutable std::filesystem::path logFile_;
    mutable RuntimeState state_ = RuntimeState::NotPrepared;
    mutable std::uint64_t processId_ = 0;
    std::unique_ptr<ProcessState> process_;
};

} // namespace fantasy::studio::runtime
