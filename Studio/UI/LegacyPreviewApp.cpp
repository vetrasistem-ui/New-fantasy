#include "MapEngine/Import/LegacyWorkspaceSession.hpp"
#include "Rendering/LegacySpriteTextureCache.hpp"
#include "UI/LegacyMapCanvasRenderer.hpp"
#include "UI/StudioTheme.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using fantasy::studio::mapcore::LegacyMapProjectConfig;
using fantasy::studio::mapcore::LegacyWorkspaceSession;
using fantasy::studio::mapcore::Position;
using fantasy::studio::rendering::LegacySpriteTextureCache;
using fantasy::studio::ui::LegacyMapCanvasRenderer;
using fantasy::studio::ui::LegacyMapCanvasView;

namespace {

void usage() {
    std::cout
        << "Fantasy Legacy Preview\n"
        << "Usage:\n"
        << "  fantasy-legacy-preview --otbm <map.otbm> --otb <items.otb> --dat <Tibia.dat> --spr <Tibia.spr>\n"
        << "      [--house <map-house.xml>] [--spawn <map-spawn.xml>] [--profile <id>]\n";
}

LegacyMapProjectConfig parseConfig(int argc, char** argv) {
    LegacyMapProjectConfig config;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--otbm" && i + 1 < argc) config.otbmPath = argv[++i];
        else if (arg == "--otb" && i + 1 < argc) config.otbPath = argv[++i];
        else if (arg == "--dat" && i + 1 < argc) config.datPath = argv[++i];
        else if (arg == "--spr" && i + 1 < argc) config.sprPath = argv[++i];
        else if (arg == "--house" && i + 1 < argc) config.houseXmlPath = fs::path(argv[++i]);
        else if (arg == "--spawn" && i + 1 < argc) config.spawnXmlPath = fs::path(argv[++i]);
        else if (arg == "--profile" && i + 1 < argc) config.profileId = argv[++i];
        else throw std::runtime_error("Unknown or incomplete argument: " + arg);
    }
    return config;
}

void printFailure(const LegacyWorkspaceSession& session) {
    for (const auto& warning : session.report().warnings) std::cerr << "WARN " << warning << '\n';
    for (const auto& error : session.report().errors) std::cerr << "ERROR " << error << '\n';
}

int runPreview(const LegacyMapProjectConfig& config) {
    LegacyWorkspaceSession session;
    if (!session.open(config)) {
        printFailure(session);
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    SDL_Window* window = SDL_CreateWindow(
        "Fantasy Studio - Legacy 10.98 Preview",
        1440,
        900,
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
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    fantasy::studio::ui::applyFantasyStudioTheme(1.0f);
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    LegacySpriteTextureCache textures(renderer, config.datPath, config.sprPath);
    LegacyMapCanvasRenderer mapRenderer;
    std::optional<Position> selected;
    float dragX = 0.0f;
    float dragY = 0.0f;
    bool done = false;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) done = true;
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        auto& viewState = session.view();
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize, ImGuiCond_Always);
        ImGui::Begin("Legacy Map Preview", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        ImGui::TextColored(ImVec4(0.13f, 0.83f, 0.93f, 1.0f), "Fantasy Studio / Legacy Project Preview");
        ImGui::SameLine();
        ImGui::TextDisabled("profile=%s", config.profileId.c_str());
        ImGui::TextDisabled(
            "%s | %ux%u | %zu tiles | %zu houses | %zu spawn areas | DAT 0x%X | SPR 0x%X",
            session.document().metadata().name.c_str(),
            session.document().metadata().width,
            session.document().metadata().height,
            session.document().map().tileCount(),
            session.document().map().houses().size(),
            session.document().map().spawnAreas().size(),
            session.report().datSignature,
            session.report().sprSignature);

        ImGui::Separator();
        ImGui::Text("Floor %d", static_cast<int>(viewState.floor));
        ImGui::SameLine();
        if (ImGui::SmallButton("Floor -")) --viewState.floor;
        ImGui::SameLine();
        if (ImGui::SmallButton("Floor +")) ++viewState.floor;
        ImGui::SameLine();
        ImGui::Text("Zoom %.0f px", viewState.tilePixels);
        ImGui::SameLine();
        ImGui::Checkbox("Grid", &viewState.showGrid);
        ImGui::SameLine();
        if (ImGui::SmallButton("Center")) {
            session.resetView();
            selected.reset();
            dragX = 0.0f;
            dragY = 0.0f;
        }

        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        canvasSize.x = std::max(1.0f, canvasSize.x);
        canvasSize.y = std::max(1.0f, canvasSize.y - 24.0f);
        const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(
            "##legacy-map-canvas",
            canvasSize,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
        const bool hovered = ImGui::IsItemHovered();

        if (hovered && ImGui::GetIO().MouseWheel != 0.0f) {
            viewState.tilePixels = std::clamp(
                viewState.tilePixels * (ImGui::GetIO().MouseWheel > 0.0f ? 1.15f : 0.87f),
                8.0f,
                96.0f);
        }

        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            dragX += ImGui::GetIO().MouseDelta.x;
            dragY += ImGui::GetIO().MouseDelta.y;
            while (dragX >= viewState.tilePixels) { --viewState.center.x; dragX -= viewState.tilePixels; }
            while (dragX <= -viewState.tilePixels) { ++viewState.center.x; dragX += viewState.tilePixels; }
            while (dragY >= viewState.tilePixels) { --viewState.center.y; dragY -= viewState.tilePixels; }
            while (dragY <= -viewState.tilePixels) { ++viewState.center.y; dragY += viewState.tilePixels; }
        }

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const ImVec2 center(canvasOrigin.x + canvasSize.x * 0.5f, canvasOrigin.y + canvasSize.y * 0.5f);
            const auto x = viewState.center.x + static_cast<std::int32_t>(std::floor((mouse.x - center.x) / viewState.tilePixels));
            const auto y = viewState.center.y + static_cast<std::int32_t>(std::floor((mouse.y - center.y) / viewState.tilePixels));
            Position position{x, y, viewState.floor};
            selected = session.document().map().contains(position) ? std::optional<Position>(position) : std::nullopt;
        }

        LegacyMapCanvasView canvasView;
        canvasView.centerTileX = viewState.center.x;
        canvasView.centerTileY = viewState.center.y;
        canvasView.floor = viewState.floor;
        canvasView.tilePixels = viewState.tilePixels;
        canvasView.showGrid = viewState.showGrid;

        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(
            canvasOrigin,
            ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y),
            IM_COL32(5, 13, 25, 255));
        const auto stats = mapRenderer.draw(
            draw,
            canvasOrigin,
            canvasSize,
            session.document(),
            textures,
            canvasView,
            selected);

        ImGui::SetCursorScreenPos(ImVec2(canvasOrigin.x, canvasOrigin.y + canvasSize.y + 4.0f));
        if (selected.has_value()) {
            ImGui::TextDisabled(
                "Selected %d,%d,%d | visited=%zu ground=%zu items=%zu missing=%zu cached=%zu",
                selected->x, selected->y, static_cast<int>(selected->z),
                stats.visitedTiles, stats.renderedGrounds, stats.renderedItems,
                stats.missingTextures, textures.cachedTextureCount());
        } else {
            ImGui::TextDisabled(
                "Center %d,%d,%d | visited=%zu ground=%zu items=%zu missing=%zu cached=%zu",
                viewState.center.x, viewState.center.y, static_cast<int>(viewState.floor),
                stats.visitedTiles, stats.renderedGrounds, stats.renderedItems,
                stats.missingTextures, textures.cachedTextureCount());
        }

        ImGui::End();
        ImGui::Render();

        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 5, 13, 25, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    textures.clear();
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
        if (argc < 9) {
            usage();
            return 2;
        }
        return runPreview(parseConfig(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "Fantasy legacy preview error: " << error.what() << '\n';
        return 1;
    }
}
