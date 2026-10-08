// Keep the accepted V5 implementation as the single visual source. This entry
// renames its original main and reuses every shell helper in the same translation
// unit, adding only the canonical 10.98 Map workspace path.
#define main fantasyStudioFmapMain
#include "StudioAppV5.cpp"
#undef main

#include "MapEngine/Import/LegacyWorkspaceSession.hpp"
#include "Rendering/LegacySpriteTextureCache.hpp"
#include "UI/LegacyMapCanvasRenderer.hpp"

#include <memory>

namespace {

using LegacyConfig = fantasy::studio::mapcore::LegacyMapProjectConfig;
using LegacySession = fantasy::studio::mapcore::LegacyWorkspaceSession;
using LegacyPosition = fantasy::studio::mapcore::Position;
using LegacyTextureCache = fantasy::studio::rendering::LegacySpriteTextureCache;
using LegacyCanvasRenderer = fantasy::studio::ui::LegacyMapCanvasRenderer;
using LegacyCanvasView = fantasy::studio::ui::LegacyMapCanvasView;

struct LegacyLaunchOptions {
    fs::path projectPath;
    LegacyConfig config;
    bool enabled = false;
};

LegacyLaunchOptions parseLegacyLaunch(int argc, char** argv) {
    LegacyLaunchOptions options;
    options.projectPath = fs::current_path();

    bool projectAssigned = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        const auto takePath = [&](fs::path& target) {
            if (i + 1 >= argc) throw std::runtime_error("Missing value for " + argument);
            target = fs::path(argv[++i]);
            options.enabled = true;
        };

        if (argument == "--legacy-otbm") takePath(options.config.otbmPath);
        else if (argument == "--legacy-otb") takePath(options.config.otbPath);
        else if (argument == "--legacy-dat") takePath(options.config.datPath);
        else if (argument == "--legacy-spr") takePath(options.config.sprPath);
        else if (argument == "--legacy-house") {
            fs::path path;
            takePath(path);
            options.config.houseXmlPath = std::move(path);
        } else if (argument == "--legacy-spawn") {
            fs::path path;
            takePath(path);
            options.config.spawnXmlPath = std::move(path);
        } else if (argument == "--legacy-profile") {
            if (i + 1 >= argc) throw std::runtime_error("Missing value for --legacy-profile");
            options.config.profileId = argv[++i];
            options.enabled = true;
        } else if (!argument.starts_with("--") && !projectAssigned) {
            options.projectPath = fs::path(argument);
            projectAssigned = true;
        } else if (options.enabled) {
            throw std::runtime_error("Unknown legacy launch argument: " + argument);
        }
    }

    if (!options.enabled) return options;
    if (options.config.profileId.empty()) options.config.profileId = "legacy1098";
    if (options.config.otbmPath.empty() || options.config.otbPath.empty() ||
        options.config.datPath.empty() || options.config.sprPath.empty()) {
        throw std::runtime_error(
            "Legacy V5 workspace requires --legacy-otbm, --legacy-otb, --legacy-dat and --legacy-spr.");
    }
    return options;
}

struct LegacyV5Runtime {
    LegacySession session;
    std::unique_ptr<LegacyTextureCache> textures;
    LegacyCanvasRenderer canvas;
    std::optional<LegacyPosition> selected;
    LegacyPosition homeCenter{};
    float dragX = 0.0f;
    float dragY = 0.0f;
    std::string status = "Real 10.98 project loaded";

    void load(const LegacyConfig& config) {
        if (!session.open(config)) {
            std::string message = "Unable to load real 10.98 project";
            if (!session.report().errors.empty()) message += ": " + session.report().errors.front();
            throw std::runtime_error(message);
        }
        homeCenter = session.view().center;
    }

    void attachRenderer(SDL_Renderer* renderer) {
        textures = std::make_unique<LegacyTextureCache>(renderer, session.datPath(), session.sprPath());
    }

    void clearRenderer() noexcept {
        if (textures) textures->clear();
        textures.reset();
    }
};

void drawLegacyMapToolbar(LegacyV5Runtime& runtime, EditorState& state) {
    auto& view = runtime.session.view();
    iconButton("Select", Icon::Select, ImVec2(29, 28), true);
    ImGui::SameLine(0, 5);
    ImGui::BeginDisabled();
    iconButton("Brush", Icon::Brush, ImVec2(29, 28), true);
    ImGui::SameLine(0, 5);
    iconButton("Erase", Icon::Erase, ImVec2(29, 28), true);
    ImGui::EndDisabled();
    ImGui::SameLine(0, 10);
    ImGui::TextDisabled("real 10.98 / read gate");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(78.0f);
    int floor = static_cast<int>(view.floor);
    if (ImGui::InputInt("Z", &floor, 1, 1)) {
        view.floor = static_cast<std::int16_t>(std::clamp(floor, 0, 15));
        state.floor = view.floor;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &view.showGrid);
    ImGui::SameLine();
    if (ImGui::SmallButton("Center")) {
        view.center = runtime.homeCenter;
        view.floor = runtime.homeCenter.z;
        state.floor = view.floor;
    }
}

void drawLegacyMapViewport(LegacyV5Runtime& runtime, EditorState& state) {
    ImGui::Begin("Map Editor##v5", nullptr, kFixedWindow | ImGuiWindowFlags_NoTitleBar);
    drawLegacyMapToolbar(runtime, state);
    ImGui::Separator();

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(1.0f, canvasSize.x);
    canvasSize.y = std::max(1.0f, canvasSize.y - 22.0f);
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(
        "##legacy-v5-canvas",
        canvasSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    auto& viewState = runtime.session.view();

    if (hovered && ImGui::GetIO().MouseWheel != 0.0f) {
        viewState.tilePixels = std::clamp(
            viewState.tilePixels * (ImGui::GetIO().MouseWheel > 0.0f ? 1.15f : 0.87f),
            8.0f,
            96.0f);
    }

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        runtime.dragX += ImGui::GetIO().MouseDelta.x;
        runtime.dragY += ImGui::GetIO().MouseDelta.y;
        while (runtime.dragX >= viewState.tilePixels) {
            --viewState.center.x;
            runtime.dragX -= viewState.tilePixels;
        }
        while (runtime.dragX <= -viewState.tilePixels) {
            ++viewState.center.x;
            runtime.dragX += viewState.tilePixels;
        }
        while (runtime.dragY >= viewState.tilePixels) {
            --viewState.center.y;
            runtime.dragY -= viewState.tilePixels;
        }
        while (runtime.dragY <= -viewState.tilePixels) {
            ++viewState.center.y;
            runtime.dragY += viewState.tilePixels;
        }
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const ImVec2 center(canvasOrigin.x + canvasSize.x * 0.5f, canvasOrigin.y + canvasSize.y * 0.5f);
        const auto x = viewState.center.x + static_cast<std::int32_t>(
            std::floor((mouse.x - center.x) / viewState.tilePixels));
        const auto y = viewState.center.y + static_cast<std::int32_t>(
            std::floor((mouse.y - center.y) / viewState.tilePixels));
        const LegacyPosition position{x, y, viewState.floor};
        runtime.selected = runtime.session.document().map().contains(position)
            ? std::optional<LegacyPosition>(position)
            : std::nullopt;
        runtime.status = runtime.selected.has_value() ? "Canonical tile selected" : "Empty coordinate";
        state.status = runtime.status;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(
        canvasOrigin,
        ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y),
        IM_COL32(5, 13, 25, 255));

    LegacyCanvasView view;
    view.centerTileX = viewState.center.x;
    view.centerTileY = viewState.center.y;
    view.floor = viewState.floor;
    view.tilePixels = viewState.tilePixels;
    view.showGrid = viewState.showGrid;

    const auto stats = runtime.canvas.draw(
        draw,
        canvasOrigin,
        canvasSize,
        runtime.session.document(),
        *runtime.textures,
        view,
        runtime.selected);

    ImGui::SetCursorScreenPos(ImVec2(canvasOrigin.x, canvasOrigin.y + canvasSize.y + 3.0f));
    ImGui::TextDisabled(
        "Center %d,%d,%d  ·  %.0fpx  ·  visible %zu  ·  ground %zu  ·  items %zu  ·  missing %zu",
        viewState.center.x,
        viewState.center.y,
        static_cast<int>(viewState.floor),
        viewState.tilePixels,
        stats.visitedTiles,
        stats.renderedGrounds,
        stats.renderedItems,
        stats.missingTextures);
    ImGui::End();
}

void drawLegacyInspector(LegacyV5Runtime& runtime) {
    panel("Inspector##v5", "Inspector");
    if (runtime.selected.has_value()) {
        const auto position = *runtime.selected;
        const auto* tile = runtime.session.document().map().findTile(position);
        ImGui::Text("Tile %d, %d, %d", position.x, position.y, static_cast<int>(position.z));
        ImGui::Separator();
        if (tile) {
            if (tile->ground.has_value()) {
                ImGui::Text("Ground server: %u", tile->ground->serverId);
                ImGui::TextDisabled("client: %u", tile->ground->clientId);
            } else {
                ImGui::TextDisabled("No ground");
            }
            ImGui::Text("Stack items: %zu", tile->items.size());
            ImGui::Text("House: %u", tile->houseId);
            ImGui::Text("Flags: 0x%08X", tile->flags);
            if (!tile->items.empty()) {
                ImGui::SeparatorText("Top item");
                const auto& item = tile->items.back();
                ImGui::Text("server %u", item.serverId);
                ImGui::TextDisabled("client %u · subtype %u", item.clientId, item.countOrSubtype);
            }
        }
    } else {
        ImGui::TextDisabled("Selecione um tile no mapa real.");
    }

    ImGui::Separator();
    drawStatusPill("READ GATE PASS", kSuccess);
    ImGui::TextWrapped("A escrita OTBM permanece bloqueada até concluir os testes reais de edição.");
    ImGui::End();
}

void drawLegacyMiniMap(LegacyV5Runtime& runtime) {
    panel("Minimap##v5", "Map status");
    const auto& document = runtime.session.document();
    const auto& view = runtime.session.view();
    ImGui::Text("%ux%u", document.metadata().width, document.metadata().height);
    ImGui::Text("%,zu tiles", document.map().tileCount());
    ImGui::TextDisabled("%zu houses · %zu spawn areas", document.map().houses().size(), document.map().spawnAreas().size());
    ImGui::Separator();
    ImGui::Text("Center %d,%d,%d", view.center.x, view.center.y, static_cast<int>(view.floor));
    ImGui::TextDisabled("Minimap index comes after real edit gate.");
    ImGui::End();
}

void drawLegacyConsole(const ProjectInfo& project, LegacyV5Runtime& runtime) {
    panel("Console##v5", "Console");
    ImGui::TextColored(kCyan, "[10.98] %s", runtime.status.c_str());
    ImGui::TextDisabled(
        "%s · DAT 0x%X · SPR 0x%X · warnings %zu · errors %zu",
        project.name.c_str(),
        runtime.session.report().datSignature,
        runtime.session.report().sprSignature,
        runtime.session.report().warnings.size(),
        runtime.session.report().errors.size());
    ImGui::End();
}

void drawLegacyMapPage(
    LegacyV5Runtime& runtime,
    EditorState& state,
    const ProjectInfo& project,
    const WorkspaceLayout& layout) {

    const float right = std::clamp(layout.contentSize.x * .255f, 280.0f, 310.0f);
    const float console = 82.0f;
    const float left = layout.contentSize.x - right - layout.gap;
    const float upper = layout.contentSize.y - console - layout.gap;
    const float mini = std::clamp(upper * .23f, 122.0f, 155.0f);
    const float inspector = upper - mini - layout.gap;

    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(left, upper), ImGuiCond_Always);
    drawLegacyMapViewport(runtime, state);

    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x + left + layout.gap, layout.contentPos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(right, inspector), ImGuiCond_Always);
    drawLegacyInspector(runtime);

    ImGui::SetNextWindowPos(
        ImVec2(layout.contentPos.x + left + layout.gap, layout.contentPos.y + inspector + layout.gap),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(right, mini), ImGuiCond_Always);
    drawLegacyMiniMap(runtime);

    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x, layout.contentPos.y + upper + layout.gap), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.contentSize.x, console), ImGuiCond_Always);
    drawLegacyConsole(project, runtime);
}

int runStudioLegacyV5(const fs::path& projectInput, const LegacyConfig& config) {
    const auto project = ProjectManager::openProject(projectInput);
    MapDocument fmapDocument(loadFmap(project.mainMapPath));
    EditorState state;
    state.page = StudioPage::Map;

    LegacyV5Runtime legacy;
    legacy.load(config);
    state.floor = legacy.session.view().floor;
    state.status = "Real 10.98 canonical map — read gate PASS";

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
    float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (mainScale <= 0.0f) mainScale = 1.0f;

    SDL_Rect usable{};
    int windowWidth = static_cast<int>(1440 * mainScale);
    int windowHeight = static_cast<int>(900 * mainScale);
    if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable)) {
        const float fit = std::min({
            1.0f,
            static_cast<float>(std::max(1, usable.w - 32)) / windowWidth,
            static_cast<float>(std::max(1, usable.h - 64)) / windowHeight});
        windowWidth = static_cast<int>(windowWidth * fit);
        windowHeight = static_cast<int>(windowHeight * fit);
    }

    SDL_Window* window = SDL_CreateWindow(
        "Fantasy Studio — 10.98",
        windowWidth,
        windowHeight,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        const std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow failed: " + error);
    }
    (void)SDL_SetWindowMinimumSize(
        window,
        std::min(windowWidth, static_cast<int>(960 * mainScale)),
        std::min(windowHeight, static_cast<int>(640 * mainScale)));
    styleNativeTitleBar(window);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        const std::string error = SDL_GetError();
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("SDL_CreateRenderer failed: " + error);
    }
    (void)SDL_SetRenderVSync(renderer, 1);
    legacy.attachRenderer(renderer);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    PresentationArt art(renderer);
    presentation = &art;
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    loadStudioFonts();
    fantasy::studio::ui::applyFantasyStudioTheme(mainScale);
    ImGui::GetStyle().FontSizeBase = 15.0f;
    ImGui::GetStyle().FontScaleDpi = mainScale;
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

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

        const WorkspaceLayout layout = computeLayout(state.page);
        const TopAction action = drawTopbar(project, fmapDocument, state, layout);
        drawSidebar(state, layout);

        if (state.page == StudioPage::Map && action != TopAction::None) {
            if (action == TopAction::Save) {
                legacy.status = "OTBM Writer remains frozen until the real edit gate";
                state.status = legacy.status;
            } else {
                legacy.status = "Canonical Undo/Redo becomes active with the real edit gate";
                state.status = legacy.status;
            }
        } else {
            executeTopAction(action, fmapDocument, state, project.mainMapPath);
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            if (state.page == StudioPage::Map) {
                legacy.status = "OTBM Writer remains frozen until the real edit gate";
                state.status = legacy.status;
            } else {
                executeTopAction(TopAction::Save, fmapDocument, state, project.mainMapPath);
            }
        }

        switch (state.page) {
            case StudioPage::Home:
                drawHomePage(project, fmapDocument, state, layout);
                break;
            case StudioPage::Map:
                drawLegacyMapPage(legacy, state, project, layout);
                break;
            case StudioPage::ItemsAssets:
                drawItemsAssetsPage(fmapDocument, state, layout);
                break;
            default:
                drawModuleShell(state.page, layout);
                break;
        }

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0.031f, 0.071f, 0.125f, 1.0f);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    legacy.clearRenderer();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    art.emblem.reset();
    art.emblemSmall.reset();
    art.hero.reset();
    presentation = nullptr;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const LegacyLaunchOptions options = parseLegacyLaunch(argc, argv);
        if (!options.enabled) return fantasyStudioFmapMain(argc, argv);
        return runStudioLegacyV5(options.projectPath, options.config);
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Studio V5 entry error: " << error.what() << '\n';
        return 1;
    }
}
