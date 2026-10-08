#include "Runtime/RuntimeBackend.hpp"

#include <cassert>
#include <filesystem>

using namespace fantasy::studio::runtime;

namespace {

class FakeBackend final : public RuntimeBackend {
public:
    [[nodiscard]] RuntimeKind kind() const noexcept override { return RuntimeKind::Tfs1098; }
    [[nodiscard]] const char* id() const noexcept override { return "test-tfs1098"; }
    [[nodiscard]] const char* displayName() const noexcept override { return "Test TFS 1.4.2 / 10.98"; }
    [[nodiscard]] RuntimeCapabilities capabilities() const noexcept override {
        return RuntimeCapabilities{true, false, false, false};
    }

    [[nodiscard]] RuntimePackageReport packageProject(const RuntimePackageRequest& request) override {
        RuntimePackageReport report;
        if (request.projectRoot.empty() || request.outputDirectory.empty()) {
            report.errors.emplace_back("projectRoot and outputDirectory are required");
            return report;
        }
        report.success = true;
        report.generatedFiles.push_back(request.outputDirectory / "world.otbm");
        return report;
    }
};

} // namespace

int main() {
    FakeBackend backend;
    assert(backend.kind() == RuntimeKind::Tfs1098);
    assert(backend.capabilities().canPackageProject);

    const RuntimePackageRequest request{
        std::filesystem::path{"FantasyProject"},
        std::filesystem::path{"Build/TFS1098"},
    };
    const RuntimePackageReport report = backend.packageProject(request);
    assert(report.success);
    assert(report.generatedFiles.size() == 1);
    assert(report.generatedFiles.front().filename() == "world.otbm");

    return 0;
}
