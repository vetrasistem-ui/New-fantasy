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
using fantasy::studio::ui::LegacyMapCanvasStats;
using fantasy::studio::ui::LegacyMapCanvasView;

namespace {

struct PreviewOptions {
    LegacyMapProjectConfig config;
    std::optional<fs::path> screenshotPath;
};

void usage() {
    std::cout
        << "Fantasy Legacy Preview\n"
        << "Usage:\n"
        << "  fantasy-legacy-preview --otbm <map.otbm> --otb <items.otb> --dat <Tibia.dat> --spr <Tibia.spr>\n"
        << "      [--house <map-house.xml>] [--spawn <map-spawn.xml>] [--profile <id>]\n"
        << "      [--screenshot <output.bmp>]\n\n"
        << "When --screenshot is supplied the preview renders two frames, writes a BMP,\n"
        << "prints render statistics and exits. This is intended for real-pack visual gates.\n";
}

PreviewOptions parseOptions(int argc, char** argv) {
    PreviewOptions options;
    auto& config = options.config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto takePath = [&](fs::path& target) {
            if (i + 1 >= argc) throw std::runtime_error("Missing value for " + arg);
            target = fs::path(argv[++i]);
        };

        if (arg == "--otbm") takePath(config.otbmPath);
        else if (arg == "--otb") takePath(config.otbPath);
        else if (arg == "--dat") takePath(config.datPath);
        else if (arg == "--spr") takePath(config.sprPath);
        else if (arg == "--house") {
            fs::path path;
            takePath(path);
            config.houseXmlPath = std::move(path);
        } else if (arg == "--spawn") {
            fs::path path;
            takePath(path);
            config.spawnXmlPath = std::move(path);
        } else if (arg == "--profile") {
            if (i + 1 >= argc) throw std::runtime_error("Missing value for --profile");
            config.profileId = argv[++i];
        } else if (arg == "--screenshot") {
            fs::path path;
            takePath(path);
            options.screenshotPath = std::move(path);
        } else {
            throw std::runtime_error("Unknown or incomplete argument: " + arg);
        }
    }

    if (config.otbmPath.empty() || config.otbPath.empty() || config.datPath.empty() || config.sprPath.empty()) {
        throw std::runtime_error("--otbm, --otb, --dat and --spr are required");
    }
    if (config.profileId.empty()) config.profileId = "legacy1098";
    return options;
}

void printFailure(const LegacyWorkspaceSession& session) {
    for (const auto& warning : session.report().warnings) std::cerr << "WARN " << warning << '\n';
    for (const auto& error : session.report().errors) std::cerr << "ERROR " << error << '\n';
}

void saveScreenshot(SDL_Renderer* renderer, const fs::path& output) {
    if (output.has_parent_path()) fs::create_directories(output.parent_path());

    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    if (surface == nullptr) {
        throw std::runtime_error(std::string("SDL_RenderReadPixels failed: ") + SDL_GetError());
    }

    const std::string outputUtf8 = output.string();
    const bool saved = SDL_SaveBMP(surface, outputUtf8.c_str());
    SDL_DestroySurface(surface);
    if (!saved) {
        throw std::runtime_error(std::string("SDL_SaveBMP failed: ") + SDL_GetError());
    }
}

int runPreview(const PreviewOptions& options) {
    const auto& config = options.config;
    LegacyWorkspaceSession session;
    if (!session.open(config)) {
        printFailure(session);
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (options.screenshotPath.has_value()) windowFlags |= SDL_WINDOW_HIDDEN;

    SDL_Window* window = SDL_CreateWindow(
        "Fantasy Studio - Legacy 10.98 Preview",
        1440,
        900,
        windowFlags);
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
    (void)SDL_SetRenderVSync(renderer, options.screenshotPath.has_value() ? 0 : 1);

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
    int renderedFrames = 0;
    LegacyMapCanvasStats lastStats;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) done = true;
        }

        if (!options.screenshotPath.has_value() && (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) {
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
        lastStats = mapRenderer.draw(
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
                lastStats.visitedTiles, lastStats.renderedGrounds, lastStats.renderedItems,
                lastStats.missingTextures, textures.cachedTextureCount());
        } else {
            ImGui::TextDisabled(
                "Center %d,%d,%d | visited=%zu ground=%zu items=%zu missing=%zu cached=%zu",
                viewState.center.x, viewState.center.y, static_cast<int>(viewState.floor),
                lastStats.visitedTiles, lastStats.renderedGrounds, lastStats.renderedItems,
                lastStats.missingTextures, textures.cachedTextureCount());
        }

        ImGui::End();
        ImGui::Render();

        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 5, 13, 25, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        ++renderedFrames;
        if (options.screenshotPath.has_value() && renderedFrames >= 2) {
            saveScreenshot(renderer, *options.screenshotPath);
            std::cout
                << "SCREENSHOT " << options.screenshotPath->string() << '\n'
                << "CENTER " << viewState.center.x << ' ' << viewState.center.y << ' ' << viewState.floor << '\n'
                << "RENDER visited=" << lastStats.visitedTiles
                << " ground=" << lastStats.renderedGrounds
                << " items=" << lastStats.renderedItems
                << " missing=" << lastStats.missingTextures
                << " cached=" << textures.cachedTextureCount() << '\n';
            done = true;
        }

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
        if (argc == 1) {
            usage();
            return 2;
        }
        return runPreview(parseOptions(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "Fantasy legacy preview error: " << error.what() << '\n';
        usage();
        return 1;
    }
}
