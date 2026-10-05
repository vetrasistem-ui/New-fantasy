#include "MapEngine/EditorOperations.hpp"
#include "MapEngine/FmapCore.hpp"
#include "Project/ProjectManager.hpp"
#include "UI/StudioTheme.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using fantasy::studio::ProjectInfo;
using fantasy::studio::ProjectManager;
using fantasy::studio::map::EditorOperations;
using fantasy::studio::map::MapDocument;
using fantasy::studio::map::TileLocator;
using fantasy::studio::map::loadFmap;
using fantasy::studio::map::saveFmap;

namespace {

enum class StudioPage { Home, Map, ItemsAssets, Monsters, Npcs, Spells, Quests, Systems, Server, Client };
enum class TopAction { None, Save, Undo, Redo };
enum class MapTool { Select, Brush, Erase, Fill, Tile, Object, Spawn, Zone, Path, Event, Config };

enum class AssetTab { Items, Sprites, Textures, Sounds };

struct EditorState {
    StudioPage page = StudioPage::Home;
    MapTool mapTool = MapTool::Select;
    AssetTab assetTab = AssetTab::Items;
    std::int16_t floor = 7;
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    std::optional<TileLocator> selected;
    std::array<char, 128> groundKey{};
    std::array<char, 128> objectKey{};
    std::array<char, 128> assetSearch{};
    std::string assetFilter = "All";
    std::string status = "Fantasy Studio ready";
};

struct WorkspaceLayout {
    ImVec2 contentPos{};
    ImVec2 contentSize{};
    float topbarHeight = 58.0f;
    float sidebarWidth = 174.0f;
    float gap = 8.0f;
};

constexpr ImGuiWindowFlags kFixedWindow =
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse;

const char* pageLabel(StudioPage page) {
    switch (page) {
        case StudioPage::Home: return "Home";
        case StudioPage::Map: return "Map";
        case StudioPage::ItemsAssets: return "Items & Assets";
        case StudioPage::Monsters: return "Monsters";
        case StudioPage::Npcs: return "NPCs";
        case StudioPage::Spells: return "Spells";
        case StudioPage::Quests: return "Quests";
        case StudioPage::Systems: return "Systems";
        case StudioPage::Server: return "Server";
        case StudioPage::Client: return "Client";
    }
    return "Fantasy Studio";
}

ImU32 colorForKey(const std::string& key) {
    std::uint32_t hash = 2166136261u;
    for (const unsigned char ch : key) {
        hash ^= ch;
        hash *= 16777619u;
    }
    const int r = 55 + static_cast<int>((hash >> 0) & 0x78);
    const int g = 65 + static_cast<int>((hash >> 8) & 0x78);
    const int b = 70 + static_cast<int>((hash >> 16) & 0x78);
    return IM_COL32(r, g, b, 255);
}

void copyText(std::array<char, 128>& target, const std::string& value) {
    const std::size_t count = std::min<std::size_t>(target.size() - 1, value.size());
    std::copy_n(value.data(), count, target.data());
    target[count] = '\0';
}

bool sameLocator(const TileLocator& a, const TileLocator& b) { return a == b; }

WorkspaceLayout computeLayout() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    WorkspaceLayout layout;
    layout.contentPos = ImVec2(
        viewport->WorkPos.x + layout.sidebarWidth + layout.gap,
        viewport->WorkPos.y + layout.topbarHeight + layout.gap);
    layout.contentSize = ImVec2(
        std::max(420.0f, viewport->WorkSize.x - layout.sidebarWidth - layout.gap * 2.0f),
        std::max(300.0f, viewport->WorkSize.y - layout.topbarHeight - layout.gap * 2.0f));
    return layout;
}

void drawBrandMark(ImDrawList* draw, ImVec2 center, float radius) {
    const ImU32 cyan = IM_COL32(34, 211, 238, 255);
    const ImU32 blue = IM_COL32(14, 165, 233, 255);
    const ImU32 ice = IM_COL32(226, 232, 240, 245);
    const ImU32 gold = IM_COL32(245, 158, 11, 255);

    const ImVec2 top(center.x, center.y - radius);
    const ImVec2 right(center.x + radius * 0.72f, center.y);
    const ImVec2 bottom(center.x, center.y + radius);
    const ImVec2 left(center.x - radius * 0.72f, center.y);
    draw->AddQuadFilled(top, right, bottom, left, IM_COL32(2, 132, 199, 110));
    draw->AddQuad(top, right, bottom, left, cyan, 2.0f);
    draw->AddLine(ImVec2(center.x - radius * 0.38f, center.y - radius * 0.42f),
                  ImVec2(center.x + radius * 0.42f, center.y - radius * 0.42f), ice, 3.0f);
    draw->AddLine(ImVec2(center.x - radius * 0.28f, center.y - radius * 0.42f),
                  ImVec2(center.x - radius * 0.28f, center.y + radius * 0.48f), blue, 4.0f);
    draw->AddLine(ImVec2(center.x - radius * 0.28f, center.y),
                  ImVec2(center.x + radius * 0.22f, center.y), cyan, 3.0f);
    draw->AddCircleFilled(ImVec2(center.x, center.y - radius * 0.62f), radius * 0.13f, gold);
}

void drawSectionTitle(const char* title, const char* subtitle = nullptr) {
    ImGui::PushFont(nullptr, 20.0f);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    if (subtitle != nullptr) ImGui::TextDisabled("%s", subtitle);
}

bool accentButton(const char* label, ImVec2 size = ImVec2(0.0f, 0.0f)) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f, 0.52f, 0.78f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.05f, 0.65f, 0.91f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.04f, 0.42f, 0.66f, 1.0f));
    const bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return clicked;
}

TopAction drawTopbar(const ProjectInfo& project, const EditorState& state, const WorkspaceLayout& layout) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, layout.topbarHeight), ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    TopAction action = TopAction::None;
    ImGui::Begin("##fantasy-topbar", nullptr, flags);

    const ImVec2 start = ImGui::GetCursorScreenPos();
    drawBrandMark(ImGui::GetWindowDrawList(), ImVec2(start.x + 18.0f, start.y + 17.0f), 15.0f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 42.0f);
    ImGui::PushFont(nullptr, 19.0f);
    ImGui::TextUnformatted("FANTASY");
    ImGui::SameLine(0.0f, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.13f, 0.83f, 0.93f, 1.0f));
    ImGui::TextUnformatted("STUDIO");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::SameLine(0.0f, 14.0f);
    ImGui::TextDisabled("%s  /  %s", project.name.c_str(), pageLabel(state.page));

    const float controlsWidth = 500.0f;
    if (ImGui::GetContentRegionAvail().x > controlsWidth) ImGui::SameLine(ImGui::GetWindowWidth() - controlsWidth);
    else ImGui::SameLine();

    if (ImGui::Button("Save")) action = TopAction::Save;
    ImGui::SameLine();
    if (ImGui::Button("Undo")) action = TopAction::Undo;
    ImGui::SameLine();
    if (ImGui::Button("Redo")) action = TopAction::Redo;
    ImGui::SameLine();

    ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06f, 0.72f, 0.51f, 0.8f));
    ImGui::Button("Play");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.94f, 0.27f, 0.27f, 0.8f));
    ImGui::Button("Stop");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Button("Build");
    ImGui::SameLine();
    ImGui::Button("Settings");
    ImGui::EndDisabled();

    ImGui::End();
    return action;
}

void drawSidebar(EditorState& state, const WorkspaceLayout& layout) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + layout.topbarHeight + layout.gap), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.sidebarWidth, viewport->WorkSize.y - layout.topbarHeight - layout.gap), ImGuiCond_Always);

    ImGui::Begin("##fantasy-sidebar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    const auto nav = [&state](StudioPage page, const char* badge, const char* label) {
        ImGui::PushID(label);
        const bool selected = state.page == page;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.02f, 0.52f, 0.78f, 0.82f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.05f, 0.65f, 0.91f, 0.9f));
        }
        const std::string line = std::string("[") + badge + "]  " + label;
        if (ImGui::Selectable(line.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(0.0f, 35.0f))) {
            state.page = page;
            state.status = std::string("Opened ") + label;
        }
        if (selected) ImGui::PopStyleColor(2);
        ImGui::PopID();
    };

    ImGui::TextDisabled("CREATE");
    ImGui::Separator();
    nav(StudioPage::Home, "H", "Home");
    nav(StudioPage::Map, "M", "Map");
    nav(StudioPage::ItemsAssets, "A", "Items & Assets");
    nav(StudioPage::Monsters, "X", "Monsters");
    nav(StudioPage::Npcs, "N", "NPCs");
    nav(StudioPage::Spells, "S", "Spells");
    nav(StudioPage::Quests, "Q", "Quests");
    nav(StudioPage::Systems, "Y", "Systems");

    ImGui::Spacing();
    ImGui::TextDisabled("RUNTIME");
    ImGui::Separator();
    nav(StudioPage::Server, "R", "Server");
    nav(StudioPage::Client, "C", "Client");

    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY() + 8.0f, ImGui::GetWindowHeight() - 82.0f));
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.13f, 0.83f, 0.93f, 1.0f), "Fantasy Protocol v1");
    ImGui::TextDisabled("FMAP native");
    ImGui::TextDisabled("Visual Foundation v1");
    ImGui::End();
}

struct WorldStats { std::size_t regions = 0; std::size_t chunks = 0; std::size_t tiles = 0; std::size_t objects = 0; };

WorldStats collectWorldStats(const MapDocument& document) {
    WorldStats stats;
    stats.regions = document.world().regions.size();
    for (const auto& region : document.world().regions) {
        stats.chunks += region.chunks.size();
        for (const auto& chunk : region.chunks) {
            stats.tiles += chunk.tiles.size();
            for (const auto& tile : chunk.tiles) stats.objects += tile.objects.size();
        }
    }
    return stats;
}

void drawHeroBanner(const ProjectInfo& project, const MapDocument& document) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float height = 128.0f;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilledMultiColor(
        p, ImVec2(p.x + width, p.y + height),
        IM_COL32(4, 24, 43, 255), IM_COL32(2, 91, 138, 255),
        IM_COL32(4, 44, 73, 255), IM_COL32(8, 18, 32, 255));
    draw->AddRect(p, ImVec2(p.x + width, p.y + height), IM_COL32(14, 165, 233, 170), 6.0f, 0, 1.0f);

    for (int i = 0; i < 9; ++i) {
        const float x = p.x + width * (0.55f + 0.05f * static_cast<float>(i));
        const float peak = p.y + 34.0f + static_cast<float>((i % 3) * 17);
        draw->AddTriangleFilled(ImVec2(x - 52.0f, p.y + height), ImVec2(x, peak), ImVec2(x + 58.0f, p.y + height),
                                IM_COL32(9, 84 + i * 4, 105 + i * 5, 125));
    }

    drawBrandMark(draw, ImVec2(p.x + 55.0f, p.y + 63.0f), 40.0f);
    ImGui::SetCursorScreenPos(ImVec2(p.x + 112.0f, p.y + 29.0f));
    ImGui::PushFont(nullptr, 27.0f);
    ImGui::TextUnformatted("FANTASY");
    ImGui::SameLine(0.0f, 7.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.13f, 0.83f, 0.93f, 1.0f));
    ImGui::TextUnformatted("STUDIO");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(p.x + 114.0f, p.y + 67.0f));
    ImGui::Text("Crie, edite e publique seus jogos 2D");
    ImGui::SetCursorScreenPos(ImVec2(p.x + 114.0f, p.y + 89.0f));
    ImGui::TextDisabled("Projeto ativo: %s  |  Mundo: %s", project.name.c_str(), document.world().info.name.c_str());
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + height + 8.0f));
}

void drawActionCard(const char* id, const char* symbol, const char* title, const char* body, bool enabled, bool* clicked) {
    ImGui::BeginChild(id, ImVec2(0.0f, 112.0f), ImGuiChildFlags_Borders);
    ImGui::PushFont(nullptr, 25.0f);
    ImGui::TextColored(ImVec4(0.13f, 0.83f, 0.93f, 1.0f), "%s", symbol);
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::TextUnformatted(title);
    ImGui::TextDisabled("%s", body);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 35.0f);
    ImGui::BeginDisabled(!enabled);
    if (accentButton(title, ImVec2(-1.0f, 0.0f)) && clicked != nullptr) *clicked = true;
    ImGui::EndDisabled();
    ImGui::EndChild();
}

void drawHomePage(const ProjectInfo& project, const MapDocument& document, EditorState& state, const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.contentSize, ImGuiCond_Always);
    ImGui::Begin("Home", nullptr, kFixedWindow);
    drawHeroBanner(project, document);

    drawSectionTitle("Projetos", "Gerencie a base do jogo e abra o workspace principal.");
    const float gap = 8.0f;
    const float cardWidth = std::max(180.0f, (ImGui::GetContentRegionAvail().x - gap * 2.0f) / 3.0f);
    bool openMap = false;
    ImGui::BeginGroup(); ImGui::SetNextItemWidth(cardWidth);
    ImGui::BeginChild("home-card-new-wrap", ImVec2(cardWidth, 112.0f));
    bool unused = false; drawActionCard("home-new", "+", "Novo Projeto", "Criação guiada será conectada ao ProjectManager.", false, &unused); ImGui::EndChild();
    ImGui::EndGroup();
    ImGui::SameLine(0.0f, gap);
    ImGui::BeginGroup(); ImGui::BeginChild("home-card-open-wrap", ImVec2(cardWidth, 112.0f));
    drawActionCard("home-open", ">", "Abrir Projeto", "Seletor nativo será conectado sem mudar o core.", false, &unused); ImGui::EndChild(); ImGui::EndGroup();
    ImGui::SameLine(0.0f, gap);
    ImGui::BeginGroup(); ImGui::BeginChild("home-card-map-wrap", ImVec2(cardWidth, 112.0f));
    drawActionCard("home-map", "#", "Abrir Mapa", "Abre o Map Workspace real do FMAP atual.", true, &openMap); ImGui::EndChild(); ImGui::EndGroup();

    if (openMap) { state.page = StudioPage::Map; state.status = "Opened Map Workspace"; }

    ImGui::Spacing();
    const float available = ImGui::GetContentRegionAvail().x;
    const float leftWidth = std::max(320.0f, available * 0.58f);
    const float rightWidth = std::max(260.0f, available - leftWidth - gap);
    const float panelHeight = std::max(230.0f, ImGui::GetContentRegionAvail().y);

    ImGui::BeginChild("recent-projects", ImVec2(leftWidth, panelHeight), ImGuiChildFlags_Borders);
    drawSectionTitle("Projetos Recentes");
    bool hasRecent = false;
    try {
        const auto recent = ProjectManager::recentProjects(project.root);
        for (const auto& path : recent) {
            hasRecent = true;
            ImGui::PushID(path.string().c_str());
            ImGui::Selectable(path.filename().string().c_str(), false, ImGuiSelectableFlags_None, ImVec2(0.0f, 34.0f));
            ImGui::SameLine();
            ImGui::TextDisabled("%s", path.generic_string().c_str());
            ImGui::PopID();
        }
    } catch (...) {}
    if (!hasRecent) {
        ImGui::Text("%s", project.name.c_str());
        ImGui::TextDisabled("Projeto atual");
        ImGui::TextWrapped("%s", project.root.string().c_str());
    }
    ImGui::EndChild();

    ImGui::SameLine(0.0f, gap);
    ImGui::BeginChild("project-info", ImVec2(rightWidth, panelHeight), ImGuiChildFlags_Borders);
    drawSectionTitle("Informações do Projeto");
    const WorldStats stats = collectWorldStats(document);
    ImGui::TextDisabled("Nome"); ImGui::Text("%s", project.name.c_str());
    ImGui::TextDisabled("Mundo"); ImGui::Text("%s", document.world().info.name.c_str());
    ImGui::Separator();
    ImGui::Text("Regions   %d", static_cast<int>(stats.regions));
    ImGui::Text("Chunks    %d", static_cast<int>(stats.chunks));
    ImGui::Text("Tiles     %d", static_cast<int>(stats.tiles));
    ImGui::Text("Objects   %d", static_cast<int>(stats.objects));
    ImGui::Separator();
    ImGui::TextDisabled("Main map");
    ImGui::TextWrapped("%s", project.mainMapPath.string().c_str());
    if (accentButton("Abrir Map Workspace", ImVec2(-1.0f, 0.0f))) {
        state.page = StudioPage::Map;
        state.status = "Opened Map Workspace";
    }
    ImGui::EndChild();
    ImGui::End();
}

void drawMapToolbar(EditorState& state) {
    struct Tool { MapTool id; const char* shortName; const char* label; };
    static constexpr Tool tools[] = {
        {MapTool::Select,"S","Select"}, {MapTool::Brush,"B","Brush"}, {MapTool::Erase,"E","Erase"},
        {MapTool::Fill,"F","Fill"}, {MapTool::Tile,"T","Tile"}, {MapTool::Object,"O","Object"},
        {MapTool::Spawn,"P","Spawn"}, {MapTool::Zone,"Z","Zone"}, {MapTool::Path,"R","Path"},
        {MapTool::Event,"V","Event"}, {MapTool::Config,"C","Config"}
    };
    for (std::size_t i = 0; i < std::size(tools); ++i) {
        const bool active = state.mapTool == tools[i].id;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f, 0.52f, 0.78f, 1.0f));
        ImGui::PushID(static_cast<int>(i));
        const std::string caption = std::string(tools[i].shortName) + "##" + tools[i].label;
        if (ImGui::Button(caption.c_str(), ImVec2(32.0f, 28.0f))) {
            state.mapTool = tools[i].id;
            state.status = std::string("Tool selected: ") + tools[i].label;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tools[i].label);
        ImGui::PopID();
        if (active) ImGui::PopStyleColor();
        if (i + 1 < std::size(tools)) ImGui::SameLine();
    }
}

void drawMapViewport(MapDocument& document, EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("Map Editor", nullptr, kFixedWindow);
    drawMapToolbar(state);
    ImGui::Separator();

    ImGui::Text("Floor %d", static_cast<int>(state.floor));
    ImGui::SameLine(); if (ImGui::SmallButton("-")) --state.floor;
    ImGui::SameLine(); if (ImGui::SmallButton("+")) ++state.floor;
    ImGui::SameLine();
    if (ImGui::SmallButton("Center")) {
        state.panX = 0.0f; state.panY = 0.0f; state.floor = world.developmentSpawn.z;
        state.status = "Centered on development spawn";
    }
    ImGui::SameLine(); ImGui::TextDisabled("Zoom %.2fx", state.zoom);

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 220.0f);
    canvasSize.y = std::max(canvasSize.y, 200.0f);
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##map-canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), IM_COL32(5, 14, 27, 255));

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta; state.panX += delta.x; state.panY += delta.y;
    }
    if (hovered && ImGui::GetIO().MouseWheel != 0.0f) {
        state.zoom = std::clamp(state.zoom * (ImGui::GetIO().MouseWheel > 0.0f ? 1.1f : 0.9f), 0.25f, 4.0f);
    }

    const float tilePixels = static_cast<float>(world.info.tileSize) * state.zoom;
    const float centerX = canvasOrigin.x + canvasSize.x * 0.5f + state.panX;
    const float centerY = canvasOrigin.y + canvasSize.y * 0.5f + state.panY;
    const float worldCenterX = static_cast<float>(world.developmentSpawn.x);
    const float worldCenterY = static_cast<float>(world.developmentSpawn.y);
    std::optional<TileLocator> clicked;
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    for (const auto& region : world.regions) {
        for (const auto& chunk : region.chunks) {
            if (chunk.floor != state.floor) continue;
            for (const auto& tile : chunk.tiles) {
                const float globalX = static_cast<float>(region.origin.x + chunk.x + tile.x);
                const float globalY = static_cast<float>(region.origin.y + chunk.y + tile.y);
                const ImVec2 p0(centerX + (globalX - worldCenterX) * tilePixels, centerY + (globalY - worldCenterY) * tilePixels);
                const ImVec2 p1(p0.x + tilePixels, p0.y + tilePixels);
                if (p1.x < canvasOrigin.x || p1.y < canvasOrigin.y || p0.x > canvasOrigin.x + canvasSize.x || p0.y > canvasOrigin.y + canvasSize.y) continue;
                const TileLocator locator{region.id, chunk.x, chunk.y, chunk.floor, tile.x, tile.y};
                const bool selected = state.selected.has_value() && sameLocator(*state.selected, locator);
                draw->AddRectFilled(p0, p1, colorForKey(tile.ground));
                draw->AddRect(p0, p1, selected ? IM_COL32(34,211,238,255) : IM_COL32(30,41,59,255), 0.0f, 0, selected ? 3.0f : 1.0f);
                if (!tile.objects.empty() && tilePixels >= 16.0f) {
                    draw->AddCircleFilled(ImVec2(p0.x + tilePixels * 0.72f, p0.y + tilePixels * 0.28f), std::max(2.0f, tilePixels * 0.10f), IM_COL32(226,232,240,230));
                }
                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mouse.x >= p0.x && mouse.x < p1.x && mouse.y >= p0.y && mouse.y < p1.y) {
                    clicked = locator; copyText(state.groundKey, tile.ground); if (!tile.objects.empty()) copyText(state.objectKey, tile.objects.front());
                }
            }
        }
    }
    if (clicked.has_value()) { state.selected = clicked; state.status = "Tile selected"; }

    draw->PushClipRect(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), true);
    if (tilePixels >= 8.0f) {
        const float offsetX = std::fmod(centerX - worldCenterX * tilePixels, tilePixels);
        const float offsetY = std::fmod(centerY - worldCenterY * tilePixels, tilePixels);
        for (float x = canvasOrigin.x + offsetX; x < canvasOrigin.x + canvasSize.x; x += tilePixels)
            draw->AddLine(ImVec2(x, canvasOrigin.y), ImVec2(x, canvasOrigin.y + canvasSize.y), IM_COL32(226,232,240,16));
        for (float y = canvasOrigin.y + offsetY; y < canvasOrigin.y + canvasSize.y; y += tilePixels)
            draw->AddLine(ImVec2(canvasOrigin.x, y), ImVec2(canvasOrigin.x + canvasSize.x, y), IM_COL32(226,232,240,16));
    }
    draw->PopClipRect();
    ImGui::End();
}

void drawInspector(MapDocument& document, EditorState& state, const fs::path& mapPath) {
    const auto& world = document.world();
    ImGui::Begin("Inspector", nullptr, kFixedWindow);
    ImGui::TextDisabled("INSPECTOR");
    if (ImGui::BeginTabBar("inspector-tabs")) {
        if (ImGui::BeginTabItem("Tile")) {
            ImGui::Text("%s", world.info.name.c_str());
            ImGui::TextDisabled("%d region(s) | %d px", static_cast<int>(world.regions.size()), world.info.tileSize);
            ImGui::Separator();
            if (state.selected.has_value()) {
                const auto& selected = *state.selected;
                ImGui::Text("Tile %d, %d, %d", selected.tileX, selected.tileY, static_cast<int>(selected.floor));
                ImGui::TextDisabled("Region %s | Chunk %d,%d", selected.regionId.c_str(), selected.chunkX, selected.chunkY);
                ImGui::SeparatorText("Ground");
                ImGui::InputText("##ground-key", state.groundKey.data(), state.groundKey.size());
                if (accentButton("Paint")) {
                    try { EditorOperations::paintGround(document, selected, state.groundKey.data()); state.status = "Paint ground — PASS"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
                ImGui::SameLine();
                if (ImGui::Button("Fill")) {
                    try { const std::size_t changed = EditorOperations::fillConnectedGround(document, selected, state.groundKey.data()); state.status = "Fill connected — " + std::to_string(changed) + " tile(s)"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
                ImGui::SeparatorText("Object");
                ImGui::InputText("##object-key", state.objectKey.data(), state.objectKey.size());
                if (accentButton("Add")) {
                    try { EditorOperations::addObject(document, selected, state.objectKey.data()); state.status = "Add object — PASS"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove")) {
                    try { EditorOperations::removeObject(document, selected, state.objectKey.data()); state.status = "Remove object — PASS"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
                ImGui::SameLine();
                if (ImGui::Button("Erase")) {
                    try { const std::size_t removed = EditorOperations::eraseObjects(document, selected); state.status = "Erase tile objects — " + std::to_string(removed) + " object(s)"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
            } else ImGui::TextDisabled("Selecione um tile no mapa.");
            ImGui::SeparatorText("History");
            ImGui::BeginDisabled(!document.canUndo()); if (ImGui::Button("Undo")) { try { document.undo(); state.status = "Undo — PASS"; } catch (const std::exception& error) { state.status = error.what(); } } ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(!document.canRedo()); if (ImGui::Button("Redo")) { try { document.redo(); state.status = "Redo — PASS"; } catch (const std::exception& error) { state.status = error.what(); } } ImGui::EndDisabled();
            if (accentButton("Save FMAP", ImVec2(-1.0f, 0.0f))) { try { saveFmap(document.world(), mapPath); state.status = "FMAP saved"; } catch (const std::exception& error) { state.status = error.what(); } }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Item")) { ImGui::TextDisabled("Item properties enter with F07/F13."); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Object")) { ImGui::TextDisabled("Object inspector shell ready."); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void drawMiniMap(const MapDocument& document, const EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("Minimap", nullptr, kFixedWindow);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.x = std::max(size.x, 160.0f); size.y = std::max(size.y, 100.0f);
    ImGui::InvisibleButton("##minimap", size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(5,14,27,255));
    if (!world.regions.empty()) {
        std::int32_t minX = world.regions.front().origin.x, minY = world.regions.front().origin.y;
        std::int32_t maxX = minX + world.regions.front().size.width, maxY = minY + world.regions.front().size.height;
        for (const auto& region : world.regions) { minX = std::min(minX, region.origin.x); minY = std::min(minY, region.origin.y); maxX = std::max(maxX, region.origin.x + region.size.width); maxY = std::max(maxY, region.origin.y + region.size.height); }
        const float worldW = static_cast<float>(std::max(1, maxX - minX)); const float worldH = static_cast<float>(std::max(1, maxY - minY));
        const float scale = std::min((size.x - 16.0f) / worldW, (size.y - 28.0f) / worldH); const ImVec2 base(origin.x + 8.0f, origin.y + 8.0f);
        for (const auto& region : world.regions) { const ImVec2 p0(base.x + (region.origin.x - minX) * scale, base.y + (region.origin.y - minY) * scale); const ImVec2 p1(p0.x + region.size.width * scale, p0.y + region.size.height * scale); draw->AddRectFilled(p0,p1,IM_COL32(30,70,83,230)); draw->AddRect(p0,p1,IM_COL32(34,211,238,255)); }
        const ImVec2 spawn(base.x + (world.developmentSpawn.x - minX) * scale, base.y + (world.developmentSpawn.y - minY) * scale); draw->AddCircleFilled(spawn,4.0f,IM_COL32(245,158,11,255));
    }
    draw->AddText(ImVec2(origin.x + 8.0f, origin.y + size.y - 18.0f), IM_COL32(148,163,184,255), (std::string("Floor ") + std::to_string(state.floor)).c_str());
    ImGui::End();
}

void drawConsole(const ProjectInfo& project, const EditorState& state) {
    ImGui::Begin("Console", nullptr, kFixedWindow);
    ImGui::TextColored(ImVec4(0.13f,0.83f,0.93f,1.0f), "[Fantasy]"); ImGui::SameLine(); ImGui::TextWrapped("%s", state.status.c_str());
    ImGui::TextDisabled("Project: %s", project.name.c_str());
    ImGui::TextDisabled("FMAP: %s", project.mainMapPath.string().c_str());
    ImGui::End();
}

void drawMapPage(MapDocument& document, EditorState& state, const ProjectInfo& project, const WorkspaceLayout& layout) {
    const float rightWidth = std::clamp(layout.contentSize.x * 0.25f, 270.0f, 365.0f);
    const float consoleHeight = std::clamp(layout.contentSize.y * 0.20f, 112.0f, 158.0f);
    const float leftWidth = std::max(300.0f, layout.contentSize.x - rightWidth - layout.gap);
    const float upperHeight = std::max(260.0f, layout.contentSize.y - consoleHeight - layout.gap);
    const float inspectorHeight = std::max(255.0f, upperHeight * 0.68f);
    const float miniHeight = std::max(105.0f, upperHeight - inspectorHeight - layout.gap);

    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always); ImGui::SetNextWindowSize(ImVec2(leftWidth, upperHeight), ImGuiCond_Always); drawMapViewport(document, state);
    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x + leftWidth + layout.gap, layout.contentPos.y), ImGuiCond_Always); ImGui::SetNextWindowSize(ImVec2(rightWidth, inspectorHeight), ImGuiCond_Always); drawInspector(document, state, project.mainMapPath);
    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x + leftWidth + layout.gap, layout.contentPos.y + inspectorHeight + layout.gap), ImGuiCond_Always); ImGui::SetNextWindowSize(ImVec2(rightWidth, miniHeight), ImGuiCond_Always); drawMiniMap(document, state);
    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x, layout.contentPos.y + upperHeight + layout.gap), ImGuiCond_Always); ImGui::SetNextWindowSize(ImVec2(leftWidth, consoleHeight), ImGuiCond_Always); drawConsole(project, state);
}

std::vector<std::string> collectSemanticAssets(const MapDocument& document, bool objects) {
    std::set<std::string> keys;
    for (const auto& region : document.world().regions) for (const auto& chunk : region.chunks) for (const auto& tile : chunk.tiles) {
        if (!objects) { if (!tile.ground.empty()) keys.insert(tile.ground); }
        else for (const auto& object : tile.objects) if (!object.empty()) keys.insert(object);
    }
    return std::vector<std::string>(keys.begin(), keys.end());
}

void drawAssetGrid(const std::vector<std::string>& keys, const char* search) {
    const float cell = 102.0f;
    const float avail = ImGui::GetContentRegionAvail().x;
    const int columns = std::max(1, static_cast<int>(avail / cell));
    int column = 0;
    const std::string query = search == nullptr ? "" : search;
    for (const auto& key : keys) {
        if (!query.empty() && key.find(query) == std::string::npos) continue;
        ImGui::PushID(key.c_str());
        ImGui::BeginChild("asset-card", ImVec2(94.0f, 94.0f), ImGuiChildFlags_Borders);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("asset-preview", ImVec2(72.0f, 54.0f));
        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 72.0f, p.y + 54.0f), colorForKey(key), 4.0f);
        ImGui::TextWrapped("%s", key.c_str());
        ImGui::EndChild();
        ImGui::PopID();
        ++column; if (column < columns) ImGui::SameLine(); else column = 0;
    }
    if (keys.empty()) ImGui::TextDisabled("No semantic assets in the current FMAP yet.");
}

void drawItemsAssetsPage(const MapDocument& document, EditorState& state, const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always); ImGui::SetNextWindowSize(layout.contentSize, ImGuiCond_Always);
    ImGui::Begin("Items & Assets", nullptr, kFixedWindow);
    drawSectionTitle("Items & Assets", "Catálogo visual conectado aos semantic keys reais do FMAP; sprites 10.98 entram na F05.5.");

    const auto tab = [&state](AssetTab id, const char* label) { const bool selected = state.assetTab == id; if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f,0.52f,0.78f,1.0f)); if (ImGui::Button(label)) state.assetTab = id; if (selected) ImGui::PopStyleColor(); };
    tab(AssetTab::Items, "Items"); ImGui::SameLine(); tab(AssetTab::Sprites, "Sprites"); ImGui::SameLine(); tab(AssetTab::Textures, "Textures"); ImGui::SameLine(); tab(AssetTab::Sounds, "Sounds");
    ImGui::Separator();

    const float leftWidth = 210.0f;
    ImGui::BeginChild("asset-tree", ImVec2(leftWidth, 0.0f), ImGuiChildFlags_Borders);
    ImGui::TextDisabled("LIBRARY");
    if (ImGui::Selectable("All", state.assetFilter == "All")) state.assetFilter = "All";
    if (ImGui::Selectable("Grounds", state.assetFilter == "Grounds")) state.assetFilter = "Grounds";
    if (ImGui::Selectable("Objects", state.assetFilter == "Objects")) state.assetFilter = "Objects";
    ImGui::Separator();
    ImGui::TextDisabled("F05.5 SOURCE");
    ImGui::Text("PokeFans / 10.98");
    ImGui::TextDisabled("DAT / SPR / OTB");
    ImGui::TextDisabled("OTBM -> FMAP pending");
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("asset-grid", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputTextWithHint("##asset-search", "Buscar semantic key...", state.assetSearch.data(), state.assetSearch.size());
    ImGui::Separator();
    if (state.assetTab != AssetTab::Items) {
        ImGui::TextDisabled("Este catálogo visual será preenchido pelo Asset Registry da F05.5/F13.");
    } else {
        const bool showGrounds = state.assetFilter == "All" || state.assetFilter == "Grounds";
        const bool showObjects = state.assetFilter == "All" || state.assetFilter == "Objects";
        if (showGrounds) { ImGui::TextDisabled("GROUNDS"); drawAssetGrid(collectSemanticAssets(document, false), state.assetSearch.data()); }
        if (showObjects) { ImGui::Spacing(); ImGui::TextDisabled("OBJECTS"); drawAssetGrid(collectSemanticAssets(document, true), state.assetSearch.data()); }
    }
    ImGui::EndChild();
    ImGui::End();
}

void drawModuleShell(StudioPage page, const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always); ImGui::SetNextWindowSize(layout.contentSize, ImGuiCond_Always);
    ImGui::Begin(pageLabel(page), nullptr, kFixedWindow);
    drawSectionTitle(pageLabel(page), "Workspace preparado para o módulo oficial do Fantasy Studio.");
    ImGui::Separator();
    ImGui::BeginChild("module-summary", ImVec2(0.0f, 122.0f), ImGuiChildFlags_Borders);
    ImGui::TextColored(ImVec4(0.13f,0.83f,0.93f,1.0f), "VISUAL FOUNDATION READY");
    ImGui::TextWrapped("A navegação, hierarquia de painéis e linguagem visual já são definitivas. A lógica de domínio entra na fase correspondente sem criar um segundo core.");
    ImGui::ProgressBar(0.15f, ImVec2(-1.0f, 0.0f), "UI shell");
    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::BeginChild("module-workspace", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    switch (page) {
        case StudioPage::Monsters: ImGui::Text("F08 — Creature / Monster Core"); break;
        case StudioPage::Npcs: ImGui::Text("F10 — NPC / Dialogue"); break;
        case StudioPage::Spells: ImGui::Text("F09 — Combat / Spell / Skill"); break;
        case StudioPage::Quests: ImGui::Text("F11 — Quest System"); break;
        case StudioPage::Systems: ImGui::Text("F15 — System Lab"); break;
        case StudioPage::Server: ImGui::Text("Fantasy Server native runtime"); break;
        case StudioPage::Client: ImGui::Text("Fantasy Client native runtime"); break;
        default: break;
    }
    ImGui::TextDisabled("Sem dados simulados. Ferramentas reais serão ligadas ao mesmo domínio compartilhado.");
    ImGui::EndChild();
    ImGui::End();
}

void executeTopAction(TopAction action, MapDocument& document, EditorState& state, const fs::path& mapPath) {
    try {
        switch (action) {
            case TopAction::Save: saveFmap(document.world(), mapPath); state.status = "FMAP saved"; break;
            case TopAction::Undo: if (document.canUndo()) { document.undo(); state.status = "Undo — PASS"; } else state.status = "Nothing to undo"; break;
            case TopAction::Redo: if (document.canRedo()) { document.redo(); state.status = "Redo — PASS"; } else state.status = "Nothing to redo"; break;
            case TopAction::None: break;
        }
    } catch (const std::exception& error) { state.status = error.what(); }
}

int runStudio(const fs::path& projectInput) {
    const auto project = ProjectManager::openProject(projectInput);
    MapDocument document(loadFmap(project.mainMapPath));
    EditorState state; state.floor = document.world().developmentSpawn.z;
    if (!document.world().regions.empty() && !document.world().regions.front().chunks.empty() && !document.world().regions.front().chunks.front().tiles.empty()) copyText(state.groundKey, document.world().regions.front().chunks.front().tiles.front().ground);
    else copyText(state.groundKey, "terrain.grass.basic");
    copyText(state.objectKey, "nature.tree.oak.small");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay()); if (mainScale <= 0.0f) mainScale = 1.0f;
    SDL_Window* window = SDL_CreateWindow("Fantasy Studio", static_cast<int>(1440 * mainScale), static_cast<int>(900 * mainScale), SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr) { const std::string error = SDL_GetError(); SDL_Quit(); throw std::runtime_error("SDL_CreateWindow failed: " + error); }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) { const std::string error = SDL_GetError(); SDL_DestroyWindow(window); SDL_Quit(); throw std::runtime_error("SDL_CreateRenderer failed: " + error); }
    (void)SDL_SetRenderVSync(renderer, 1);

    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    fantasy::studio::ui::applyFantasyStudioTheme(mainScale); ImGui::GetStyle().FontScaleDpi = mainScale;
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer); ImGui_ImplSDLRenderer3_Init(renderer);

    bool done = false;
    while (!done) {
        SDL_Event event; while (SDL_PollEvent(&event)) { ImGui_ImplSDL3_ProcessEvent(&event); if (event.type == SDL_EVENT_QUIT) done = true; if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) done = true; }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) { SDL_Delay(10); continue; }
        ImGui_ImplSDLRenderer3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();

        const WorkspaceLayout layout = computeLayout();
        const TopAction topAction = drawTopbar(project, state, layout); drawSidebar(state, layout); executeTopAction(topAction, document, state, project.mainMapPath);
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) executeTopAction(TopAction::Save, document, state, project.mainMapPath);
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) executeTopAction(TopAction::Undo, document, state, project.mainMapPath);
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) executeTopAction(TopAction::Redo, document, state, project.mainMapPath);

        switch (state.page) {
            case StudioPage::Home: drawHomePage(project, document, state, layout); break;
            case StudioPage::Map: drawMapPage(document, state, project, layout); break;
            case StudioPage::ItemsAssets: drawItemsAssetsPage(document, state, layout); break;
            default: drawModuleShell(state.page, layout); break;
        }

        ImGui::Render(); SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y); SDL_SetRenderDrawColorFloat(renderer, 0.031f, 0.071f, 0.125f, 1.0f); SDL_RenderClear(renderer); ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer); SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown(); ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext(); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}

} // namespace

int main(int argc, char** argv) {
    try { const fs::path project = argc > 1 ? fs::path(argv[1]) : fs::current_path(); return runStudio(project); }
    catch (const std::exception& error) { std::cerr << "Fantasy Studio GUI error: " << error.what() << '\n'; return 1; }
}
