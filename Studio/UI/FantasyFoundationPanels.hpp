#pragma once

#include "Foundation/FantasyAuthoringRepository.hpp"
#include "Foundation/FantasyProjectHealthScanner.hpp"
#include "Foundation/FantasySystemCatalog.hpp"
#include "Project/ProjectManager.hpp"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace fantasy::studio::ui::foundation_panels {

using namespace fantasy::studio::foundation;

struct PanelLayout {
    ImVec2 pos{};
    ImVec2 size{};
};

enum class WorkspaceKind {
    ItemsAssets,
    Creatures,
    Entities,
    Classes,
    Systems,
    ClientBuild,
};

namespace detail {

template <std::size_t N>
void copyField(std::array<char, N>& target, const std::string& value) {
    const auto count = std::min<std::size_t>(N - 1U, value.size());
    std::copy_n(value.data(), count, target.data());
    target[count] = '\0';
}

inline std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

inline std::vector<std::string> splitCsv(const char* text) {
    std::vector<std::string> result;
    if (text == nullptr) return result;
    std::stringstream stream(text);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim(token);
        if (!token.empty()) result.push_back(std::move(token));
    }
    return result;
}

inline std::string joinCsv(const std::vector<std::string>& values) {
    std::string result;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) result += ", ";
        result += values[i];
    }
    return result;
}

inline AttributeMap parseAttributes(const char* text) {
    AttributeMap result;
    if (text == nullptr) return result;
    std::stringstream stream(text);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim(token);
        if (token.empty()) continue;
        const auto equals = token.find('=');
        if (equals == std::string::npos) {
            throw std::invalid_argument("attribute entries must use id=value");
        }
        const auto id = trim(token.substr(0, equals));
        const auto value = trim(token.substr(equals + 1));
        requireIdentifier(id, "attribute id");
        result[id] = std::stoi(value);
    }
    return result;
}

inline std::string joinAttributes(const AttributeMap& values) {
    std::string result;
    for (const auto& [id, value] : values) {
        if (!result.empty()) result += ", ";
        result += id + "=" + std::to_string(value);
    }
    return result;
}

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
        case HealthSeverity::Info: return ImVec4(0.18f, 0.75f, 0.92f, 1.0f);
        case HealthSeverity::Warning: return ImVec4(0.95f, 0.67f, 0.18f, 1.0f);
        case HealthSeverity::Error: return ImVec4(0.94f, 0.27f, 0.27f, 1.0f);
    }
    return ImVec4(1, 1, 1, 1);
}

struct AuthoringUiState {
    int itemsTab = 0;
    int creaturesTab = 0;
    int systemsTab = 0;
    std::string selectedId;
    std::string status = "Foundation V2 authoring ready";

    std::array<char, 128> id{};
    std::array<char, 160> name{};
    std::array<char, 160> appearance{};
    std::array<char, 320> tags{};
    std::array<char, 320> attributes{};
    std::array<char, 320> secondary{};
    std::array<char, 320> tertiary{};
    std::array<char, 320> quaternary{};
    std::array<char, 160> visual{};
    std::array<char, 96> slot{};
    std::array<char, 96> sourceRole{};
    std::array<char, 320> sourcePath{};
    std::array<char, 80> sourceHash{};
    int valueA = 1;
    int valueB = 100;
    int valueC = 0;
    int valueD = 0;
    bool toggle = false;

    void clearForm() {
        id.fill('\0'); name.fill('\0'); appearance.fill('\0'); tags.fill('\0');
        attributes.fill('\0'); secondary.fill('\0'); tertiary.fill('\0'); quaternary.fill('\0');
        visual.fill('\0'); slot.fill('\0'); sourceRole.fill('\0'); sourcePath.fill('\0'); sourceHash.fill('\0');
        valueA = 1; valueB = 100; valueC = 0; valueD = 0; toggle = false;
    }
};

inline std::map<std::string, AuthoringUiState>& states() {
    static std::map<std::string, AuthoringUiState> value;
    return value;
}

inline AuthoringUiState& stateFor(const std::filesystem::path& root) {
    return states()[root.lexically_normal().generic_string()];
}

inline void beginWorkspace(const char* title, const PanelLayout& layout) {
    ImGui::SetNextWindowPos(layout.pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.size, ImGuiCond_Always);
    ImGui::Begin(title, nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar);
}

inline void drawHeader(const char* title, const char* subtitle, AuthoringUiState& state) {
    ImGui::TextUnformatted(title);
    ImGui::TextDisabled("%s", subtitle);
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.13f, 0.83f, 0.93f, 1.0f), "%s", state.status.c_str());
    ImGui::Separator();
}

inline bool tabButton(const char* label, int index, int& current) {
    const bool selected = current == index;
    if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f, 0.42f, 0.68f, 1.0f));
    const bool pressed = ImGui::Button(label, ImVec2(126.0f, 28.0f));
    if (selected) ImGui::PopStyleColor();
    if (pressed) current = index;
    return pressed;
}

inline void drawCollection(
    const char* title,
    const std::vector<std::string>& ids,
    AuthoringUiState& state,
    float width = 230.0f) {

    ImGui::BeginChild((std::string("##list-") + title).c_str(), ImVec2(width, 0.0f), ImGuiChildFlags_Borders);
    ImGui::Text("%s (%d)", title, static_cast<int>(ids.size()));
    ImGui::Separator();
    if (ImGui::Button("New", ImVec2(74, 26))) {
        state.selectedId.clear();
        state.clearForm();
        state.status = std::string("New ") + title + " entry";
    }
    ImGui::Separator();
    for (const auto& id : ids) {
        const bool selected = state.selectedId == id;
        if (ImGui::Selectable(id.c_str(), selected)) state.selectedId = id;
    }
    if (ids.empty()) ImGui::TextDisabled("No entries yet.");
    ImGui::EndChild();
    ImGui::SameLine();
}

inline void textField(const char* label, std::array<char, 128>& field) {
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(label, field.data(), field.size());
}

inline void textField(const char* label, std::array<char, 160>& field) {
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(label, field.data(), field.size());
}

inline void textField(const char* label, std::array<char, 320>& field) {
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(label, field.data(), field.size());
}

inline void textField(const char* label, std::array<char, 96>& field) {
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(label, field.data(), field.size());
}

inline void textField(const char* label, std::array<char, 80>& field) {
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(label, field.data(), field.size());
}

inline void drawItemEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.items();
    drawCollection("Items", ids, state);
    ImGui::BeginChild("##item-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);

    static std::string loaded;
    if (!state.selectedId.empty() && loaded != state.selectedId) {
        try {
            const auto value = repository.loadItem(state.selectedId);
            copyField(state.id, value.id); copyField(state.name, value.name);
            copyField(state.appearance, value.appearanceRef.value_or("")); copyField(state.tags, joinCsv(value.tags));
            state.toggle = value.equipment.has_value();
            if (value.equipment) {
                copyField(state.slot, value.equipment->slot);
                state.valueA = static_cast<int>(value.equipment->requirements.minimumLevel);
                copyField(state.attributes, joinAttributes(value.equipment->requirements.attributes));
                copyField(state.secondary, joinAttributes(value.equipment->bonuses.attributes));
            }
            loaded = state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }

    textField("ID", state.id); textField("Name", state.name); textField("Appearance ref", state.appearance);
    textField("Tags (comma separated)", state.tags);
    ImGui::Checkbox("Equipment", &state.toggle);
    if (state.toggle) {
        textField("Slot", state.slot);
        ImGui::InputInt("Minimum level", &state.valueA);
        textField("Requirements (attr=value)", state.attributes);
        textField("Bonuses (attr=value)", state.secondary);
    }
    if (ImGui::Button("Save item", ImVec2(112, 30))) {
        try {
            ItemDefinition value;
            value.id = state.id.data(); value.name = state.name.data(); value.tags = splitCsv(state.tags.data());
            if (state.appearance[0] != '\0') value.appearanceRef = std::string(state.appearance.data());
            if (state.toggle) {
                EquipmentDefinition equipment;
                equipment.id = value.id + ".equipment";
                equipment.slot = state.slot.data();
                equipment.requirements.minimumLevel = static_cast<std::uint32_t>(std::max(1, state.valueA));
                equipment.requirements.attributes = parseAttributes(state.attributes.data());
                equipment.bonuses.attributes = parseAttributes(state.secondary.data());
                value.equipment = std::move(equipment);
            }
            repository.save(value); state.selectedId = value.id; loaded.clear(); state.status = "Item saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeItem(state.selectedId); state.status = "Item removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); loaded.clear(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::EndChild();
}

inline void drawAppearanceEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.appearances();
    drawCollection("Appearances", ids, state);
    ImGui::BeginChild("##appearance-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    static std::string loaded;
    if (!state.selectedId.empty() && loaded != state.selectedId) {
        try {
            const auto value = repository.loadAppearance(state.selectedId);
            copyField(state.id, value.id); copyField(state.visual, value.baseVisual);
            copyField(state.secondary, value.shaderRef.value_or("")); loaded = state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    textField("ID", state.id); textField("Base visual", state.visual); textField("Shader ref", state.secondary);
    ImGui::TextDisabled("Attachments/effects remain domain data and are preserved when loaded; this compact editor creates the base appearance.");
    if (ImGui::Button("Save appearance", ImVec2(132, 30))) {
        try {
            AppearanceDefinition value;
            value.id = state.id.data(); value.baseVisual = state.visual.data();
            if (state.secondary[0] != '\0') value.shaderRef = std::string(state.secondary.data());
            repository.save(value); state.selectedId = value.id; loaded.clear(); state.status = "Appearance saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeAppearance(state.selectedId); state.status = "Appearance removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); loaded.clear(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::EndChild();
}

inline void drawCreatureEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.creatures();
    drawCollection("Creatures", ids, state);
    ImGui::BeginChild("##creature-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    static std::string loaded;
    if (!state.selectedId.empty() && loaded != state.selectedId) {
        try {
            const auto value = repository.loadCreature(state.selectedId);
            copyField(state.id, value.id); copyField(state.name, value.name); copyField(state.appearance, value.appearanceRef);
            copyField(state.tags, joinCsv(value.tags)); copyField(state.attributes, joinAttributes(value.attributes));
            state.valueA = static_cast<int>(value.level); state.valueB = static_cast<int>(value.movementSpeed); loaded = state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    textField("ID", state.id); textField("Name", state.name); textField("Appearance ref", state.appearance);
    ImGui::InputInt("Level", &state.valueA); ImGui::InputInt("Movement speed", &state.valueB);
    textField("Attributes (attr=value)", state.attributes); textField("Tags", state.tags);
    if (ImGui::Button("Save creature", ImVec2(122, 30))) {
        try {
            CreatureDefinition value;
            value.id = state.id.data(); value.name = state.name.data(); value.appearanceRef = state.appearance.data();
            value.level = static_cast<std::uint32_t>(std::max(1, state.valueA));
            value.movementSpeed = static_cast<std::uint32_t>(std::max(1, state.valueB));
            value.attributes = parseAttributes(state.attributes.data()); value.tags = splitCsv(state.tags.data());
            repository.save(value); state.selectedId = value.id; loaded.clear(); state.status = "Creature saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeCreature(state.selectedId); state.status = "Creature removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); loaded.clear(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::EndChild();
}

inline void drawClassEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.classes();
    drawCollection("Classes", ids, state);
    ImGui::BeginChild("##class-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    static std::string loaded;
    if (!state.selectedId.empty() && loaded != state.selectedId) {
        try {
            const auto value = repository.loadClass(state.selectedId);
            copyField(state.id, value.id); copyField(state.name, value.name);
            copyField(state.secondary, joinCsv(value.passiveSystems)); copyField(state.tertiary, joinCsv(value.abilitySystems)); loaded = state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    textField("ID", state.id); textField("Name", state.name);
    textField("Passive systems", state.secondary); textField("Ability systems", state.tertiary);
    if (ImGui::Button("Save class", ImVec2(108, 30))) {
        try {
            ClassDefinition value;
            value.id = state.id.data(); value.name = state.name.data();
            value.passiveSystems = splitCsv(state.secondary.data()); value.abilitySystems = splitCsv(state.tertiary.data());
            repository.save(value); state.selectedId = value.id; loaded.clear(); state.status = "Class saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeClass(state.selectedId); state.status = "Class removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); loaded.clear(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::EndChild();
}

inline void drawEntityEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.entities();
    drawCollection("Entities", ids, state);
    ImGui::BeginChild("##entity-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    static std::string loaded;
    if (!state.selectedId.empty() && loaded != state.selectedId) {
        try {
            const auto value = repository.loadEntity(state.selectedId);
            copyField(state.id, value.id); copyField(state.name, value.name); copyField(state.appearance, value.appearanceRef.value_or(""));
            copyField(state.tags, joinCsv(value.tags)); loaded = state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    textField("ID", state.id); textField("Name", state.name); textField("Appearance ref", state.appearance); textField("Tags", state.tags);
    ImGui::TextDisabled("Components are persisted through the domain model; advanced component rows are validated during Codex visual acceptance.");
    if (ImGui::Button("Save entity", ImVec2(112, 30))) {
        try {
            EntityArchetype value; value.id = state.id.data(); value.name = state.name.data(); value.tags = splitCsv(state.tags.data());
            if (state.appearance[0] != '\0') value.appearanceRef = std::string(state.appearance.data());
            repository.save(value); state.selectedId = value.id; loaded.clear(); state.status = "Entity saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeEntity(state.selectedId); state.status = "Entity removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); loaded.clear(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::EndChild();
}

inline void drawZoneEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.zones();
    drawCollection("Zones", ids, state);
    ImGui::BeginChild("##zone-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    static std::string loaded;
    if (!state.selectedId.empty() && loaded != state.selectedId) {
        try {
            const auto value = repository.loadZone(state.selectedId);
            copyField(state.id, value.id); copyField(state.name, value.name); copyField(state.tags, joinCsv(value.tags));
            if (!value.rectangles.empty()) {
                const auto& rect = value.rectangles.front();
                state.valueA = rect.x; state.valueB = rect.y; state.valueC = rect.z; state.valueD = static_cast<int>(rect.width);
                copyField(state.secondary, std::to_string(rect.height));
            }
            loaded = state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    textField("ID", state.id); textField("Name", state.name); textField("Tags", state.tags);
    ImGui::InputInt("X", &state.valueA); ImGui::InputInt("Y", &state.valueB); ImGui::InputInt("Z", &state.valueC);
    ImGui::InputInt("Width", &state.valueD); textField("Height", state.secondary);
    if (ImGui::Button("Save zone", ImVec2(108, 30))) {
        try {
            ZoneDefinition value; value.id = state.id.data(); value.name = state.name.data(); value.tags = splitCsv(state.tags.data());
            const int height = std::max(1, std::stoi(state.secondary[0] == '\0' ? "1" : state.secondary.data()));
            value.rectangles.push_back({state.valueA, state.valueB, static_cast<std::int16_t>(state.valueC),
                static_cast<std::uint32_t>(std::max(1, state.valueD)), static_cast<std::uint32_t>(height)});
            repository.save(value); state.selectedId = value.id; loaded.clear(); state.status = "Zone saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeZone(state.selectedId); state.status = "Zone removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); loaded.clear(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::EndChild();
}

inline void drawSystemEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.systems();
    drawCollection("Systems", ids, state);
    ImGui::BeginChild("##system-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    static std::string loaded;
    if (!state.selectedId.empty() && loaded != state.selectedId) {
        try {
            const auto value = repository.loadSystem(state.selectedId);
            copyField(state.id, value.id); state.valueA = static_cast<int>(value.version);
            copyField(state.secondary, joinCsv(value.dependencies)); copyField(state.tertiary, joinCsv(value.requiredChannels));
            loaded = state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    textField("ID", state.id); ImGui::InputInt("Version", &state.valueA);
    textField("Dependencies", state.secondary); textField("Required semantic channels", state.tertiary);
    ImGui::TextWrapped("Compact System Lab authoring creates a valid Login -> PersistValue skeleton. Trigger/Condition/Action graph details remain in the domain contract and can be expanded visually without changing persistence.");
    if (ImGui::Button("Save system", ImVec2(116, 30))) {
        try {
            SystemDefinition value; value.id = state.id.data(); value.version = static_cast<std::uint32_t>(std::max(1, state.valueA));
            value.dependencies = splitCsv(state.secondary.data()); value.requiredChannels = splitCsv(state.tertiary.data());
            value.triggers.push_back({"trigger.login", TriggerKind::Login, {}});
            value.actions.push_back({"action.persist", ActionKind::PersistValue, {{"key.id", "system.state"}}});
            repository.save(value); state.selectedId = value.id; loaded.clear(); state.status = "System saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeSystem(state.selectedId); state.status = "System removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); loaded.clear(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::Separator();
    try {
        FantasySystemCatalog catalog;
        for (const auto& id : repository.systems()) catalog.add(repository.loadSystem(id));
        const auto ordered = catalog.orderedSystemIds();
        ImGui::Text("Dependency order (%d)", static_cast<int>(ordered.size()));
        for (const auto& id : ordered) ImGui::BulletText("%s", id.c_str());
    } catch (const std::exception& error) {
        ImGui::TextColored(ImVec4(0.94f, .27f, .27f, 1), "%s", error.what());
    }
    ImGui::EndChild();
}

inline void drawModernAssetEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.modernAssets();
    drawCollection("Modern assets", ids, state);
    ImGui::BeginChild("##modern-asset-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    static std::string loaded;
    if (!state.selectedId.empty() && loaded != state.selectedId) {
        try {
            const auto value = repository.loadModernAsset(state.selectedId);
            copyField(state.id, value.id); state.valueA = value.width; state.valueB = value.height; copyField(state.tags, joinCsv(value.tags));
            if (!value.layers.empty() && !value.layers.front().frames.empty()) {
                copyField(state.visual, value.layers.front().frames.front().visualRef);
                state.valueC = static_cast<int>(value.layers.front().frames.front().durationMs);
            }
            loaded = state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    textField("ID", state.id); ImGui::InputInt("Width (tiles)", &state.valueA); ImGui::InputInt("Height (tiles)", &state.valueB);
    textField("Tags", state.tags); textField("Visual ref", state.visual); ImGui::InputInt("Frame duration ms", &state.valueC);
    if (ImGui::Button("Save modern asset", ImVec2(146, 30))) {
        try {
            ModernAssetDefinition value; value.id = state.id.data(); value.width = static_cast<std::uint16_t>(std::max(1, state.valueA));
            value.height = static_cast<std::uint16_t>(std::max(1, state.valueB)); value.tags = splitCsv(state.tags.data());
            ModernAssetLayer layer; layer.id = "base"; layer.frames.push_back({state.visual.data(), static_cast<std::uint32_t>(std::max(1, state.valueC))});
            value.layers.push_back(std::move(layer)); repository.saveModernAsset(value);
            state.selectedId = value.id; loaded.clear(); state.status = "Modern asset saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeModernAsset(state.selectedId); state.status = "Modern asset removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); loaded.clear(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::EndChild();
}

inline void drawAssetProfileEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.assetProfiles();
    drawCollection("Asset profiles", ids, state);
    ImGui::BeginChild("##asset-profile-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    textField("ID", state.id); ImGui::InputInt("Version", &state.valueA);
    textField("Source role", state.sourceRole); textField("Project-relative source path", state.sourcePath); textField("SHA-256 (optional)", state.sourceHash);
    if (ImGui::Button("Save profile", ImVec2(114, 30))) {
        try {
            AssetProfile value; value.id = state.id.data(); value.version = static_cast<std::uint32_t>(std::max(1, state.valueA));
            value.sources.push_back({state.sourceRole.data(), std::filesystem::path(state.sourcePath.data()), state.sourceHash.data()});
            repository.save(value); state.selectedId = value.id; state.status = "Asset profile saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::Separator();
    if (!state.selectedId.empty()) {
        try {
            const auto value = repository.loadAssetProfile(state.selectedId);
            ImGui::Text("Selected: %s · v%u", value.id.c_str(), value.version);
            for (const auto& source : value.sources) ImGui::BulletText("%s -> %s", source.role.c_str(), source.path.generic_string().c_str());
        } catch (const std::exception& error) { ImGui::TextColored(ImVec4(.94f,.27f,.27f,1), "%s", error.what()); }
    }
    ImGui::EndChild();
}

inline void drawBrushEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.brushes();
    drawCollection("Brushes", ids, state);
    ImGui::BeginChild("##brush-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    textField("ID", state.id); textField("Asset ref", state.visual); textField("Required tags", state.tags); ImGui::InputInt("Variant weight", &state.valueA);
    if (ImGui::Button("Save brush", ImVec2(108, 30))) {
        try {
            BrushDefinition value; value.id = state.id.data(); value.requiredTags = splitCsv(state.tags.data());
            value.variants.push_back({state.visual.data(), static_cast<std::uint32_t>(std::max(1, state.valueA))});
            repository.saveBrush(value); state.selectedId = value.id; state.status = "Brush saved: " + value.id;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::SameLine();
    if (!state.selectedId.empty() && ImGui::Button("Delete", ImVec2(82, 30))) {
        try { repository.removeBrush(state.selectedId); state.status = "Brush removed: " + state.selectedId; state.selectedId.clear(); state.clearForm(); }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::EndChild();
}

inline void drawMigrationEditor(FantasyAuthoringRepository& repository, AuthoringUiState& state) {
    const auto ids = repository.migrations();
    drawCollection("Migrations", ids, state);
    ImGui::BeginChild("##migration-editor", ImVec2(0, 0), ImGuiChildFlags_Borders);
    textField("Migration ID", state.id); textField("Source profile", state.secondary); textField("Target profile", state.tertiary);
    ImGui::InputInt("Legacy ID", &state.valueA); ImGui::InputInt("Target ID (0 for AddAsNew)", &state.valueB);
    const char* modes[] = {"AddAsNew", "ReplaceObject", "ReplaceVisualOnly"};
    state.valueC = std::clamp(state.valueC, 0, 2); ImGui::Combo("Mode", &state.valueC, modes, 3);
    if (ImGui::Button("Save migration", ImVec2(128, 30))) {
        try {
            AssetMigrationPlan value; value.sourceProfile = state.secondary.data(); value.targetProfile = state.tertiary.data();
            value.entries.push_back({static_cast<std::uint32_t>(std::max(1, state.valueA)), static_cast<std::uint32_t>(std::max(0, state.valueB)), static_cast<AssetMigrationMode>(state.valueC)});
            repository.saveMigration(state.id.data(), value); state.selectedId = state.id.data(); state.status = "Migration saved: " + state.selectedId;
        } catch (const std::exception& error) { state.status = error.what(); }
    }
    ImGui::Separator();
    if (!state.selectedId.empty()) {
        try {
            const auto value = repository.loadMigration(state.selectedId);
            ImGui::Text("%s -> %s · %d entries", value.sourceProfile.c_str(), value.targetProfile.c_str(), static_cast<int>(value.entries.size()));
        } catch (const std::exception& error) { ImGui::TextColored(ImVec4(.94f,.27f,.27f,1), "%s", error.what()); }
    }
    ImGui::EndChild();
}

inline void drawHealth(
    const ProjectInfo& project,
    const std::filesystem::path& runtimeTemplate,
    const std::filesystem::path& clientPackage,
    AuthoringUiState& state) {

    ImGui::BeginChild("##health", ImVec2(0, 0), ImGuiChildFlags_Borders);
    try {
        const auto report = FantasyProjectHealthScanner::scan({project, runtimeTemplate, clientPackage});
        ImGui::TextColored(report.ready() ? ImVec4(.06f,.72f,.51f,1) : ImVec4(.94f,.27f,.27f,1),
            "%s", report.ready() ? "PROJECT READY" : "BLOCKING ISSUES");
        ImGui::Separator();
        for (const auto& issue : report.issues) {
            ImGui::TextColored(severityColor(issue.severity), "[%s] %s", severityLabel(issue.severity), issue.code.c_str());
            ImGui::TextWrapped("%s", issue.message.c_str());
        }
        if (report.issues.empty()) ImGui::TextDisabled("No health issues reported.");
        state.status = report.ready() ? "Project health ready" : "Project health has blocking issues";
    } catch (const std::exception& error) {
        state.status = error.what(); ImGui::TextColored(ImVec4(.94f,.27f,.27f,1), "%s", error.what());
    }
    ImGui::EndChild();
}

} // namespace detail

inline void drawAuthoringWorkspace(
    WorkspaceKind kind,
    const ProjectInfo& project,
    const PanelLayout& layout,
    std::string& globalStatus,
    const std::filesystem::path& runtimeTemplate = {},
    const std::filesystem::path& clientPackage = {}) {

    auto& state = detail::stateFor(project.root);
    FantasyAuthoringRepository repository(project.root);
    detail::beginWorkspace("Foundation V2##authoring", layout);

    switch (kind) {
        case WorkspaceKind::ItemsAssets: {
            detail::drawHeader("Items & Assets — Foundation V2", "Repository-backed authoring; no widget writes raw JSON.", state);
            detail::tabButton("Items", 0, state.itemsTab); ImGui::SameLine();
            detail::tabButton("Appearances", 1, state.itemsTab); ImGui::SameLine();
            detail::tabButton("Modern Assets", 2, state.itemsTab); ImGui::SameLine();
            detail::tabButton("Profiles", 3, state.itemsTab); ImGui::SameLine();
            detail::tabButton("Brushes", 4, state.itemsTab); ImGui::SameLine();
            detail::tabButton("Migrations", 5, state.itemsTab);
            ImGui::Separator();
            if (state.itemsTab == 0) detail::drawItemEditor(repository, state);
            else if (state.itemsTab == 1) detail::drawAppearanceEditor(repository, state);
            else if (state.itemsTab == 2) detail::drawModernAssetEditor(repository, state);
            else if (state.itemsTab == 3) detail::drawAssetProfileEditor(repository, state);
            else if (state.itemsTab == 4) detail::drawBrushEditor(repository, state);
            else detail::drawMigrationEditor(repository, state);
            break;
        }
        case WorkspaceKind::Creatures: {
            detail::drawHeader("Creature / Monster Editor", "Creature and Class definitions persisted through Foundation V2.", state);
            detail::tabButton("Creatures", 0, state.creaturesTab); ImGui::SameLine(); detail::tabButton("Classes", 1, state.creaturesTab);
            ImGui::Separator();
            if (state.creaturesTab == 0) detail::drawCreatureEditor(repository, state); else detail::drawClassEditor(repository, state);
            break;
        }
        case WorkspaceKind::Entities: {
            detail::drawHeader("Entity / NPC Editor", "Neutral entity archetypes; runtime adapters remain outside authoring.", state);
            detail::drawEntityEditor(repository, state); break;
        }
        case WorkspaceKind::Classes: {
            detail::drawHeader("Class / Progression Editor", "Classes reference passive and ability systems; base character attributes stay shared.", state);
            detail::drawClassEditor(repository, state); break;
        }
        case WorkspaceKind::Systems: {
            detail::drawHeader("System Lab — Foundation", "Systems, Zones and Project Health use validated domain contracts.", state);
            detail::tabButton("Systems", 0, state.systemsTab); ImGui::SameLine();
            detail::tabButton("Zones", 1, state.systemsTab); ImGui::SameLine();
            detail::tabButton("Health", 2, state.systemsTab);
            ImGui::Separator();
            if (state.systemsTab == 0) detail::drawSystemEditor(repository, state);
            else if (state.systemsTab == 1) detail::drawZoneEditor(repository, state);
            else detail::drawHealth(project, runtimeTemplate, clientPackage, state);
            break;
        }
        case WorkspaceKind::ClientBuild: {
            detail::drawHeader("Client / Build Workspace", "Headless build contracts are ready; this surface exposes project health before local packaging.", state);
            detail::drawHealth(project, runtimeTemplate, clientPackage, state); break;
        }
    }

    globalStatus = state.status;
    ImGui::End();
}

} // namespace fantasy::studio::ui::foundation_panels
