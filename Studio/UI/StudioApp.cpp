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
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using fantasy::studio::ProjectInfo;
using fantasy::studio::ProjectManager;
using fantasy::studio::map::EditorOperations;
using fantasy::studio::map::MapDocument;
using fantasy::studio::map::TileLocator;
using fantasy::studio::map::loadFmap;
using fantasy::studio::map::saveFmap;

namespace {

enum class StudioPage {
    Home,
    Map,
    ItemsAssets,
    Monsters,
    Npcs,
    Spells,
    Quests,
    Systems,
    Server,
    Client,
};

enum class TopAction {
    None,
    Save,
    Undo,
    Redo,
};

struct EditorState {
    StudioPage page = StudioPage::Map;
    std::int16_t floor = 7;
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    std::optional<TileLocator> selected;
    std::array<char, 128> groundKey{};
    std::array<char, 128> objectKey{};
    std::string status = "Fantasy Studio ready";
};

struct WorkspaceLayout {
    ImVec2 contentPos{};
    ImVec2 contentSize{};
    float topbarHeight = 54.0f;
    float sidebarWidth = 176.0f;
    float gap = 8.0f;
};

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
    const int r = 70 + static_cast<int>((hash >> 0) & 0x6F);
    const int g = 70 + static_cast<int>((hash >> 8) & 0x6F);
    const int b = 70 + static_cast<int>((hash >> 16) & 0x6F);
    return IM_COL32(r, g, b, 255);
}

bool sameLocator(const TileLocator& a, const TileLocator& b) {
    return a == b;
}

void copyText(std::array<char, 128>& target, const std::string& value) {
    const std::size_t count = std::min<std::size_t>(target.size() - 1, value.size());
    std::copy_n(value.data(), count, target.data());
    target[count] = '\0';
}

WorkspaceLayout computeLayout() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    WorkspaceLayout layout;
    layout.contentPos = ImVec2(
        viewport->WorkPos.x + layout.sidebarWidth + layout.gap,
        viewport->WorkPos.y + layout.topbarHeight + layout.gap);
    layout.contentSize = ImVec2(
        std::max(320.0f, viewport->WorkSize.x - layout.sidebarWidth - layout.gap * 2.0f),
        std::max(240.0f, viewport->WorkSize.y - layout.topbarHeight - layout.gap * 2.0f));
    return layout;
}

TopAction drawTopbar(const ProjectInfo& project, const EditorState& state, const WorkspaceLayout& layout) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, layout.topbarHeight), ImGuiCond_Always);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    TopAction action = TopAction::None;
    ImGui::Begin("##fantasy-topbar", nullptr, flags);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.13f, 0.83f, 0.93f, 1.0f));
    ImGui::TextUnformatted("FANTASY");
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::TextUnformatted("STUDIO");
    ImGui::SameLine(0.0f, 16.0f);
    ImGui::TextDisabled("|  %s  |  %s", project.name.c_str(), pageLabel(state.page));

    const float controlsWidth = 490.0f;
    if (ImGui::GetContentRegionAvail().x > controlsWidth) {
        ImGui::SameLine(ImGui::GetWindowWidth() - controlsWidth);
    } else {
        ImGui::SameLine();
    }

    if (ImGui::Button("Save")) action = TopAction::Save;
    ImGui::SameLine();
    if (ImGui::Button("Undo")) action = TopAction::Undo;
    ImGui::SameLine();
    if (ImGui::Button("Redo")) action = TopAction::Redo;
    ImGui::SameLine();

    ImGui::BeginDisabled();
    ImGui::Button("Play");
    ImGui::SameLine();
    ImGui::Button("Stop");
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
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + layout.topbarHeight + layout.gap),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(layout.sidebarWidth, viewport->WorkSize.y - layout.topbarHeight - layout.gap),
        ImGuiCond_Always);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("##fantasy-sidebar", nullptr, flags);
    ImGui::TextDisabled("WORKSPACE");
    ImGui::Separator();

    const auto nav = [&state](StudioPage page, const char* label) {
        const bool selected = state.page == page;
        if (ImGui::Selectable(label, selected, ImGuiSelectableFlags_None, ImVec2(0.0f, 34.0f))) {
            state.page = page;
            state.status = std::string("Opened ") + label;
        }
    };

    nav(StudioPage::Home, "Home");
    nav(StudioPage::Map, "Map");
    nav(StudioPage::ItemsAssets, "Items & Assets");
    nav(StudioPage::Monsters, "Monsters");
    nav(StudioPage::Npcs, "NPCs");
    nav(StudioPage::Spells, "Spells");
    nav(StudioPage::Quests, "Quests");
    nav(StudioPage::Systems, "Systems");

    ImGui::Spacing();
    ImGui::TextDisabled("RUNTIME");
    ImGui::Separator();
    nav(StudioPage::Server, "Server");
    nav(StudioPage::Client, "Client");

    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY() + 8.0f, ImGui::GetWindowHeight() - 64.0f));
    ImGui::Separator();
    ImGui::TextDisabled("Fantasy Protocol v1");
    ImGui::TextDisabled("FMAP native");

    ImGui::End();
}

void drawMapViewport(MapDocument& document, EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("Map Workspace", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Floor %d", static_cast<int>(state.floor));
    ImGui::SameLine();
    if (ImGui::Button("Floor -")) --state.floor;
    ImGui::SameLine();
    if (ImGui::Button("Floor +")) ++state.floor;
    ImGui::SameLine();
    if (ImGui::Button("Center")) {
        state.panX = 0.0f;
        state.panY = 0.0f;
        state.floor = world.developmentSpawn.z;
        state.status = "Centered on development spawn";
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Zoom %.2fx", state.zoom);

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 200.0f);
    canvasSize.y = std::max(canvasSize.y, 200.0f);
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("##map-canvas", canvasSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(
        canvasOrigin,
        ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y),
        IM_COL32(17, 24, 39, 255));

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        state.panX += delta.x;
        state.panY += delta.y;
    }

    if (hovered && ImGui::GetIO().MouseWheel != 0.0f) {
        const float oldZoom = state.zoom;
        state.zoom = std::clamp(
            state.zoom * (ImGui::GetIO().MouseWheel > 0.0f ? 1.1f : 0.9f),
            0.25f,
            4.0f);
        if (std::abs(state.zoom - oldZoom) > 0.0001f) {
            state.status = "Viewport zoom updated";
        }
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
                const float x0 = centerX + (globalX - worldCenterX) * tilePixels;
                const float y0 = centerY + (globalY - worldCenterY) * tilePixels;
                const ImVec2 p0(x0, y0);
                const ImVec2 p1(x0 + tilePixels, y0 + tilePixels);

                if (p1.x < canvasOrigin.x || p1.y < canvasOrigin.y ||
                    p0.x > canvasOrigin.x + canvasSize.x || p0.y > canvasOrigin.y + canvasSize.y) {
                    continue;
                }

                const TileLocator locator{region.id, chunk.x, chunk.y, chunk.floor, tile.x, tile.y};
                const bool selected = state.selected.has_value() && sameLocator(*state.selected, locator);
                draw->AddRectFilled(p0, p1, colorForKey(tile.ground));
                draw->AddRect(
                    p0,
                    p1,
                    selected ? IM_COL32(34, 211, 238, 255) : IM_COL32(30, 41, 59, 255),
                    0.0f,
                    0,
                    selected ? 3.0f : 1.0f);

                if (!tile.objects.empty() && tilePixels >= 16.0f) {
                    draw->AddCircleFilled(
                        ImVec2(p0.x + tilePixels * 0.72f, p0.y + tilePixels * 0.28f),
                        std::max(2.0f, tilePixels * 0.10f),
                        IM_COL32(226, 232, 240, 230));
                }

                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                    mouse.x >= p0.x && mouse.x < p1.x && mouse.y >= p0.y && mouse.y < p1.y) {
                    clicked = locator;
                    copyText(state.groundKey, tile.ground);
                    if (!tile.objects.empty()) copyText(state.objectKey, tile.objects.front());
                }
            }
        }
    }

    if (clicked.has_value()) {
        state.selected = clicked;
        state.status = "Tile selected";
    }

    draw->PushClipRect(
        canvasOrigin,
        ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y),
        true);
    if (tilePixels >= 8.0f) {
        const float left = canvasOrigin.x;
        const float top = canvasOrigin.y;
        const float right = canvasOrigin.x + canvasSize.x;
        const float bottom = canvasOrigin.y + canvasSize.y;
        const float offsetX = std::fmod(centerX - worldCenterX * tilePixels, tilePixels);
        const float offsetY = std::fmod(centerY - worldCenterY * tilePixels, tilePixels);
        for (float x = left + offsetX; x < right; x += tilePixels) {
            draw->AddLine(ImVec2(x, top), ImVec2(x, bottom), IM_COL32(226, 232, 240, 18));
        }
        for (float y = top + offsetY; y < bottom; y += tilePixels) {
            draw->AddLine(ImVec2(left, y), ImVec2(right, y), IM_COL32(226, 232, 240, 18));
        }
    }
    draw->PopClipRect();

    ImGui::End();
}

void drawInspector(MapDocument& document, EditorState& state, const fs::path& mapPath) {
    const auto& world = document.world();
    ImGui::Begin("Inspector", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::TextDisabled("WORLD");
    ImGui::Text("%s", world.info.name.c_str());
    ImGui::TextDisabled("%d region(s) | %d px tiles", static_cast<int>(world.regions.size()), world.info.tileSize);
    ImGui::Separator();

    if (state.selected.has_value()) {
        const auto& selected = *state.selected;
        ImGui::Text("Tile %d, %d, %d", selected.tileX, selected.tileY, static_cast<int>(selected.floor));
        ImGui::TextDisabled("Region %s | Chunk %d,%d", selected.regionId.c_str(), selected.chunkX, selected.chunkY);

        ImGui::SeparatorText("Ground");
        ImGui::InputText("Ground key", state.groundKey.data(), state.groundKey.size());
        if (ImGui::Button("Paint")) {
            try {
                EditorOperations::paintGround(document, selected, state.groundKey.data());
                state.status = "Paint ground — PASS";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Fill")) {
            try {
                const std::size_t changed = EditorOperations::fillConnectedGround(
                    document,
                    selected,
                    state.groundKey.data());
                state.status = "Fill connected — " + std::to_string(changed) + " tile(s)";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }

        ImGui::SeparatorText("Object");
        ImGui::InputText("Object key", state.objectKey.data(), state.objectKey.size());
        if (ImGui::Button("Add")) {
            try {
                EditorOperations::addObject(document, selected, state.objectKey.data());
                state.status = "Add object — PASS";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove")) {
            try {
                EditorOperations::removeObject(document, selected, state.objectKey.data());
                state.status = "Remove object — PASS";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Erase")) {
            try {
                const std::size_t removed = EditorOperations::eraseObjects(document, selected);
                state.status = "Erase tile objects — " + std::to_string(removed) + " object(s)";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }
    } else {
        ImGui::TextDisabled("Select a tile in the map workspace.");
    }

    ImGui::SeparatorText("History");
    ImGui::BeginDisabled(!document.canUndo());
    if (ImGui::Button("Undo")) {
        try {
            document.undo();
            state.status = "Undo — PASS";
        } catch (const std::exception& error) {
            state.status = error.what();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!document.canRedo());
    if (ImGui::Button("Redo")) {
        try {
            document.redo();
            state.status = "Redo — PASS";
        } catch (const std::exception& error) {
            state.status = error.what();
        }
    }
    ImGui::EndDisabled();

    ImGui::SeparatorText("File");
    if (ImGui::Button("Save FMAP")) {
        try {
            saveFmap(document.world(), mapPath);
            state.status = "FMAP saved";
        } catch (const std::exception& error) {
            state.status = error.what();
        }
    }

    ImGui::End();
}

void drawMiniMap(const MapDocument& document, const EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("Minimap", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.x = std::max(size.x, 160.0f);
    size.y = std::max(size.y, 120.0f);
    ImGui::InvisibleButton("##minimap", size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(17, 24, 39, 255));

    if (!world.regions.empty()) {
        std::int32_t minX = world.regions.front().origin.x;
        std::int32_t minY = world.regions.front().origin.y;
        std::int32_t maxX = minX + world.regions.front().size.width;
        std::int32_t maxY = minY + world.regions.front().size.height;
        for (const auto& region : world.regions) {
            minX = std::min(minX, region.origin.x);
            minY = std::min(minY, region.origin.y);
            maxX = std::max(maxX, region.origin.x + region.size.width);
            maxY = std::max(maxY, region.origin.y + region.size.height);
        }

        const float worldW = static_cast<float>(std::max(1, maxX - minX));
        const float worldH = static_cast<float>(std::max(1, maxY - minY));
        const float scale = std::min((size.x - 16.0f) / worldW, (size.y - 32.0f) / worldH);
        const ImVec2 base(origin.x + 8.0f, origin.y + 8.0f);

        for (const auto& region : world.regions) {
            const ImVec2 p0(
                base.x + (region.origin.x - minX) * scale,
                base.y + (region.origin.y - minY) * scale);
            const ImVec2 p1(
                p0.x + region.size.width * scale,
                p0.y + region.size.height * scale);
            draw->AddRectFilled(p0, p1, IM_COL32(30, 70, 83, 230));
            draw->AddRect(p0, p1, IM_COL32(34, 211, 238, 255));
        }

        const ImVec2 spawn(
            base.x + (world.developmentSpawn.x - minX) * scale,
            base.y + (world.developmentSpawn.y - minY) * scale);
        draw->AddCircleFilled(spawn, 4.0f, IM_COL32(245, 158, 11, 255));
    }

    draw->AddText(
        ImVec2(origin.x + 8.0f, origin.y + size.y - 20.0f),
        IM_COL32(148, 163, 184, 255),
        (std::string("Floor ") + std::to_string(state.floor)).c_str());

    ImGui::End();
}

void drawConsole(const ProjectInfo& project, const EditorState& state) {
    ImGui::Begin("Console", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::TextColored(ImVec4(0.13f, 0.83f, 0.93f, 1.0f), "[Fantasy Studio]");
    ImGui::SameLine();
    ImGui::TextWrapped("%s", state.status.c_str());
    ImGui::TextDisabled("Project: %s", project.root.string().c_str());
    ImGui::TextDisabled("FMAP: %s", project.mainMapPath.string().c_str());
    ImGui::End();
}

void drawHomeCard(const char* id, const char* title, const char* body, bool enabled, bool* clicked = nullptr) {
    ImGui::BeginChild(id, ImVec2(230.0f, 132.0f), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(title);
    ImGui::Spacing();
    ImGui::TextDisabled("%s", body);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 42.0f);
    ImGui::BeginDisabled(!enabled);
    if (ImGui::Button(title, ImVec2(-1.0f, 0.0f)) && clicked != nullptr) {
        *clicked = true;
    }
    ImGui::EndDisabled();
    ImGui::EndChild();
}

void drawHomePage(const ProjectInfo& project, EditorState& state, const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.contentSize, ImGuiCond_Always);
    ImGui::Begin("Home", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.13f, 0.83f, 0.93f, 1.0f));
    ImGui::TextUnformatted("FANTASY STUDIO");
    ImGui::PopStyleColor();
    ImGui::Text("Crie, edite e publique seus jogos 2D");
    ImGui::TextDisabled("Visual Foundation v1 — native SDL_Renderer3 baseline");
    ImGui::Separator();

    bool openMap = false;
    drawHomeCard("home-new", "Novo projeto", "Criação guiada será ligada ao ProjectManager em uma etapa própria.", false);
    ImGui::SameLine();
    drawHomeCard("home-open", "Abrir projeto", "O shell está pronto; o seletor de arquivos nativo ainda não foi conectado.", false);
    ImGui::SameLine();
    drawHomeCard("home-import", "Importar projeto", "Reservado para importadores, incluindo o fluxo OTBM → FMAP.", false);
    ImGui::SameLine();
    drawHomeCard("home-map", "Abrir mapa", "Abre o Map Workspace real deste projeto.", true, &openMap);

    if (openMap) {
        state.page = StudioPage::Map;
        state.status = "Opened Map Workspace";
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Projeto atual");
    ImGui::Text("%s", project.name.c_str());
    ImGui::TextDisabled("Root");
    ImGui::TextWrapped("%s", project.root.string().c_str());
    ImGui::TextDisabled("Main map");
    ImGui::TextWrapped("%s", project.mainMapPath.string().c_str());

    ImGui::End();
}

void drawPlaceholderPage(StudioPage page, const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.contentSize, ImGuiCond_Always);
    ImGui::Begin(pageLabel(page), nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("%s", pageLabel(page));
    ImGui::Separator();
    ImGui::TextWrapped(
        "A base visual deste módulo já está reservada no Fantasy Studio. "
        "A funcionalidade de domínio será implementada em sua fase própria; "
        "este shell não simula dados nem altera o core existente.");

    switch (page) {
        case StudioPage::ItemsAssets:
            ImGui::TextDisabled("Próximo uso: F05.5 Legacy Asset Bridge / F07 / F13.");
            break;
        case StudioPage::Monsters:
            ImGui::TextDisabled("Planejado: F08 Creature / Monster Core.");
            break;
        case StudioPage::Npcs:
            ImGui::TextDisabled("Planejado: F10 NPC / Dialogue.");
            break;
        case StudioPage::Spells:
            ImGui::TextDisabled("Planejado: F09 Combat / Spell / Skill.");
            break;
        case StudioPage::Quests:
            ImGui::TextDisabled("Planejado: F11 Quest System.");
            break;
        case StudioPage::Systems:
            ImGui::TextDisabled("Planejado: F15 System Lab.");
            break;
        case StudioPage::Server:
            ImGui::TextDisabled("O Server nativo existe; controles visuais de execução entram em fase posterior.");
            break;
        case StudioPage::Client:
            ImGui::TextDisabled("O Client nativo existe; integração visual completa entra nas fases de assets/client.");
            break;
        default:
            break;
    }

    ImGui::End();
}

void drawMapPage(
    MapDocument& document,
    EditorState& state,
    const ProjectInfo& project,
    const WorkspaceLayout& layout) {

    const float rightWidth = std::clamp(layout.contentSize.x * 0.25f, 260.0f, 360.0f);
    const float consoleHeight = std::clamp(layout.contentSize.y * 0.22f, 118.0f, 170.0f);
    const float leftWidth = std::max(260.0f, layout.contentSize.x - rightWidth - layout.gap);
    const float upperHeight = std::max(220.0f, layout.contentSize.y - consoleHeight - layout.gap);
    const float inspectorHeight = std::max(220.0f, upperHeight * 0.67f);
    const float miniHeight = std::max(120.0f, upperHeight - inspectorHeight - layout.gap);

    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftWidth, upperHeight), ImGuiCond_Always);
    drawMapViewport(document, state);

    ImGui::SetNextWindowPos(
        ImVec2(layout.contentPos.x + leftWidth + layout.gap, layout.contentPos.y),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightWidth, inspectorHeight), ImGuiCond_Always);
    drawInspector(document, state, project.mainMapPath);

    ImGui::SetNextWindowPos(
        ImVec2(layout.contentPos.x + leftWidth + layout.gap, layout.contentPos.y + inspectorHeight + layout.gap),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightWidth, miniHeight), ImGuiCond_Always);
    drawMiniMap(document, state);

    ImGui::SetNextWindowPos(
        ImVec2(layout.contentPos.x, layout.contentPos.y + upperHeight + layout.gap),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftWidth, consoleHeight), ImGuiCond_Always);
    drawConsole(project, state);
}

void executeTopAction(
    TopAction action,
    MapDocument& document,
    EditorState& state,
    const fs::path& mapPath) {

    try {
        switch (action) {
            case TopAction::Save:
                saveFmap(document.world(), mapPath);
                state.status = "FMAP saved";
                break;
            case TopAction::Undo:
                if (document.canUndo()) {
                    document.undo();
                    state.status = "Undo — PASS";
                } else {
                    state.status = "Nothing to undo";
                }
                break;
            case TopAction::Redo:
                if (document.canRedo()) {
                    document.redo();
                    state.status = "Redo — PASS";
                } else {
                    state.status = "Nothing to redo";
                }
                break;
            case TopAction::None:
                break;
        }
    } catch (const std::exception& error) {
        state.status = error.what();
    }
}

int runStudio(const fs::path& projectInput) {
    const auto project = ProjectManager::openProject(projectInput);
    MapDocument document(loadFmap(project.mainMapPath));
    EditorState state;
    state.floor = document.world().developmentSpawn.z;

    if (!document.world().regions.empty() &&
        !document.world().regions.front().chunks.empty() &&
        !document.world().regions.front().chunks.front().tiles.empty()) {
        copyText(state.groundKey, document.world().regions.front().chunks.front().tiles.front().ground);
    } else {
        copyText(state.groundKey, "terrain.grass.basic");
    }
    copyText(state.objectKey, "nature.tree.oak.small");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (mainScale <= 0.0f) mainScale = 1.0f;

    SDL_Window* window = SDL_CreateWindow(
        "Fantasy Studio",
        static_cast<int>(1440 * mainScale),
        static_cast<int>(900 * mainScale),
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr) {
        const std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow failed: " + error);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        const std::string error = SDL_GetError();
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("SDL_CreateRenderer failed: " + error);
    }
    (void)SDL_SetRenderVSync(renderer, 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    fantasy::studio::ui::applyFantasyStudioTheme(mainScale);
    ImGui::GetStyle().FontScaleDpi = mainScale;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window)) {
                done = true;
            }
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        const WorkspaceLayout layout = computeLayout();
        const TopAction topAction = drawTopbar(project, state, layout);
        drawSidebar(state, layout);
        executeTopAction(topAction, document, state, project.mainMapPath);

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            executeTopAction(TopAction::Save, document, state, project.mainMapPath);
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            executeTopAction(TopAction::Undo, document, state, project.mainMapPath);
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
            executeTopAction(TopAction::Redo, document, state, project.mainMapPath);
        }

        switch (state.page) {
            case StudioPage::Home:
                drawHomePage(project, state, layout);
                break;
            case StudioPage::Map:
                drawMapPage(document, state, project, layout);
                break;
            default:
                drawPlaceholderPage(state.page, layout);
                break;
        }

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0.031f, 0.071f, 0.125f, 1.0f);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const fs::path project = argc > 1 ? fs::path(argv[1]) : fs::current_path();
        return runStudio(project);
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Studio GUI error: " << error.what() << '\n';
        return 1;
    }
}
