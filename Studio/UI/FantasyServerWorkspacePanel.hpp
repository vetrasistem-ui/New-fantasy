#pragma once

#include "Foundation/FantasyServerWorkspaceController.hpp"
#include "Runtime/Tfs1098RuntimeProfile.hpp"

#include "imgui.h"

#include <array>
#include <filesystem>
#include <string>

namespace fantasy::studio::ui::server_workspace_panel {

using fantasy::studio::foundation::FantasyServerWorkspaceController;
using fantasy::studio::foundation::HealthSeverity;
using fantasy::studio::runtime::Tfs1098CompatibilityProfile;
using fantasy::studio::runtime::Tfs1098RuntimeProfile;
using fantasy::studio::runtime::Tfs1098TargetConfig;

struct DrawResult {
    bool profileChanged = false;
    bool prepared = false;
};

inline const char* severityLabel(HealthSeverity severity) {
    switch (severity) {
        case HealthSeverity::Info: return "INFO";
        case HealthSeverity::Warning: return "WARN";
        case HealthSeverity::Error: return "ERROR";
    }
    return "?";
}

inline ImVec4 severityColor(HealthSeverity severity) {
    switch (severity) {
        case HealthSeverity::Info: return ImVec4(0.13f, 0.83f, 0.93f, 1.0f);
        case HealthSeverity::Warning: return ImVec4(0.95f, 0.67f, 0.18f, 1.0f);
        case HealthSeverity::Error: return ImVec4(0.94f, 0.27f, 0.27f, 1.0f);
    }
    return ImVec4(1, 1, 1, 1);
}

template <std::size_t TemplateN, std::size_t ClientN>
DrawResult draw(
    const fantasy::studio::ProjectInfo& project,
    std::array<char, TemplateN>& runtimeTemplate,
    std::array<char, ClientN>& clientPackage,
    Tfs1098TargetConfig& profile,
    std::string& status) {

    DrawResult result;
    try {
        FantasyServerWorkspaceController controller(
            project,
            std::filesystem::path(runtimeTemplate.data()),
            std::filesystem::path(clientPackage.data()));

        const char* profiles[] = {"Vanilla", "OTC Extended"};
        int selected = profile.compatibilityProfile == Tfs1098CompatibilityProfile::OtcExtended ? 1 : 0;
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::Combo("Compatibility profile", &selected, profiles, 2)) {
            profile.compatibilityProfile = selected == 1
                ? Tfs1098CompatibilityProfile::OtcExtended
                : Tfs1098CompatibilityProfile::Vanilla;
            controller.setCompatibilityProfile(profile.compatibilityProfile);
            profile = Tfs1098RuntimeProfile::load(project.root);
            result.profileChanged = true;
            status = selected == 1 ? "OTC Extended profile selected" : "Vanilla profile selected";
        }

        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("OTCv8 client package", clientPackage.data(), clientPackage.size())) {
            controller.setClientPackageDirectory(std::filesystem::path(clientPackage.data()));
        }

        const auto& state = controller.state();
        const bool extended = state.profile.compatibilityProfile == Tfs1098CompatibilityProfile::OtcExtended;
        ImGui::TextDisabled("Mode: %s", extended ? "Fantasy semantic channels over Extended Opcode" : "Official vanilla TFS 1.4.2 / 10.98");

        ImGui::BeginChild("##server-v2-capabilities", ImVec2(0.0f, 112.0f), ImGuiChildFlags_Borders);
        ImGui::Text("Runtime capabilities");
        ImGui::Separator();
        ImGui::Text("Package %s  Launch %s  Stop %s  Logs %s",
            state.capabilities.canPackageProject ? "YES" : "NO",
            state.capabilities.canLaunch ? "YES" : "NO",
            state.capabilities.canStop ? "YES" : "NO",
            state.capabilities.canStreamLogs ? "YES" : "NO");
        ImGui::Text("System channels %s  Zones %s  Appearance extensions %s",
            state.capabilities.canUseSystemChannels ? "YES" : "NO",
            state.capabilities.canUseZones ? "YES" : "NO",
            state.capabilities.canUseAppearanceExtensions ? "YES" : "NO");
        ImGui::EndChild();

        ImGui::BeginChild("##server-v2-channels", ImVec2(0.0f, 104.0f), ImGuiChildFlags_Borders);
        ImGui::Text("Required semantic channels (%d)", static_cast<int>(state.requiredChannels.size()));
        ImGui::Separator();
        if (state.requiredChannels.empty()) ImGui::TextDisabled("No authored system channels required.");
        for (const auto& channel : state.requiredChannels) ImGui::BulletText("%s", channel.c_str());
        ImGui::EndChild();

        ImGui::BeginChild("##server-v2-health", ImVec2(0.0f, 138.0f), ImGuiChildFlags_Borders);
        ImGui::TextColored(
            state.health.ready() ? ImVec4(.06f,.72f,.51f,1) : ImVec4(.94f,.27f,.27f,1),
            "%s", state.health.ready() ? "PROJECT HEALTH READY" : "PROJECT HEALTH BLOCKED");
        ImGui::Separator();
        if (state.health.issues.empty()) ImGui::TextDisabled("No issues reported.");
        for (const auto& issue : state.health.issues) {
            ImGui::TextColored(severityColor(issue.severity), "[%s] %s", severityLabel(issue.severity), issue.code.c_str());
            ImGui::TextWrapped("%s", issue.message.c_str());
        }
        ImGui::EndChild();

        ImGui::BeginDisabled(!extended || !state.health.ready());
        if (ImGui::Button("Prepare Extended Runtime", ImVec2(188.0f, 30.0f))) {
            auto report = controller.prepareRuntime();
            if (!report.success) {
                throw std::runtime_error(report.errors.empty() ? "Runtime preparation failed" : report.errors.front());
            }
            status = controller.state().status;
            result.prepared = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (!extended) ImGui::TextDisabled("Vanilla requires no bridge preparation.");
        else if (!state.health.ready()) ImGui::TextDisabled("Resolve Project Health errors before preparing bridges.");
        else ImGui::TextDisabled("Generates/updates channel registry + TFS Lua + OTCv8 module.");
    } catch (const std::exception& error) {
        status = error.what();
        ImGui::TextColored(ImVec4(.94f,.27f,.27f,1), "%s", error.what());
    }
    return result;
}

} // namespace fantasy::studio::ui::server_workspace_panel
