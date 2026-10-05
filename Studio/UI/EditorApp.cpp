#include "MapEngine/EditorOperations.hpp"
#include "MapEngine/FmapCore.hpp"
#include "Project/ProjectManager.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
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
using fantasy::studio::ProjectManager;
using fantasy::studio::map::EditorOperations;
using fantasy::studio::map::MapDocument;
using fantasy::studio::map::TileLocator;
using fantasy::studio::map::loadFmap;
using fantasy::studio::map::saveFmap;

namespace {

struct EditorState {
    std::int16_t floor = 7;
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    std::optional<TileLocator> selected;
    std::array<char, 128> groundKey{};
    std::array<char, 128> objectKey{};
    std::string status = "Ready";
};

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

void drawMapViewport(MapDocument& document, EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("Map Viewport");

    ImGui::Text("Floor %d", static_cast<int>(state.floor));
    ImGui::SameLine();
    if (ImGui::Button("Floor -")) --state.floor;
    ImGui::SameLine();
    if (ImGui::Button("Floor +")) ++state.floor;
    ImGui::SameLine();
    ImGui::Text("Zoom %.2fx", state.zoom);

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 200.0f);
    canvasSize.y = std::max(canvasSize.y, 200.0f);
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("##map-canvas", canvasSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(canvasOrigin,
        ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y),
        IM_COL32(28, 30, 34, 255));

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        state.panX += delta.x;
        state.panY += delta.y;
    }

    if (hovered && ImGui::GetIO().MouseWheel != 0.0f) {
        const float oldZoom = state.zoom;
        state.zoom = std::clamp(state.zoom * (ImGui::GetIO().MouseWheel > 0.0f ? 1.1f : 0.9f), 0.25f, 4.0f);
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
                draw->AddRect(p0, p1,
                    selected ? IM_COL32(255, 220, 80, 255) : IM_COL32(70, 74, 82, 255),
                    0.0f, 0, selected ? 3.0f : 1.0f);

                if (!tile.objects.empty() && tilePixels >= 16.0f) {
                    draw->AddCircleFilled(
                        ImVec2(p0.x + tilePixels * 0.72f, p0.y + tilePixels * 0.28f),
                        std::max(2.0f, tilePixels * 0.10f),
                        IM_COL32(240, 240, 240, 220));
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

    draw->PushClipRect(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), true);
    const float gridStep = tilePixels;
    if (gridStep >= 8.0f) {
        const float left = canvasOrigin.x;
        const float top = canvasOrigin.y;
        const float right = canvasOrigin.x + canvasSize.x;
        const float bottom = canvasOrigin.y + canvasSize.y;
        const float offsetX = std::fmod(centerX - worldCenterX * gridStep, gridStep);
        const float offsetY = std::fmod(centerY - worldCenterY * gridStep, gridStep);
        for (float x = left + offsetX; x < right; x += gridStep)
            draw->AddLine(ImVec2(x, top), ImVec2(x, bottom), IM_COL32(255, 255, 255, 18));
        for (float y = top + offsetY; y < bottom; y += gridStep)
            draw->AddLine(ImVec2(left, y), ImVec2(right, y), IM_COL32(255, 255, 255, 18));
    }
    draw->PopClipRect();

    ImGui::End();
}

void drawInspector(MapDocument& document, EditorState& state, const fs::path& mapPath) {
    ImGui::Begin("Inspector / Brushes");

    if (state.selected.has_value()) {
        const auto& selected = *state.selected;
        ImGui::Text("Region: %s", selected.regionId.c_str());
        ImGui::Text("Chunk: %d, %d", selected.chunkX, selected.chunkY);
        ImGui::Text("Tile: %d, %d, floor %d", selected.tileX, selected.tileY, static_cast<int>(selected.floor));

        ImGui::SeparatorText("Ground brush");
        ImGui::InputText("Ground key", state.groundKey.data(), state.groundKey.size());
        if (ImGui::Button("Paint selected")) {
            try {
                EditorOperations::paintGround(document, selected, state.groundKey.data());
                state.status = "Paint ground — PASS";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Fill connected")) {
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

        ImGui::SeparatorText("Object brush");
        ImGui::InputText("Object key", state.objectKey.data(), state.objectKey.size());
        if (ImGui::Button("Add object")) {
            try {
                EditorOperations::addObject(document, selected, state.objectKey.data());
                state.status = "Add object — PASS";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove object")) {
            try {
                EditorOperations::removeObject(document, selected, state.objectKey.data());
                state.status = "Remove object — PASS";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Erase tile objects")) {
            try {
                const std::size_t removed = EditorOperations::eraseObjects(document, selected);
                state.status = "Erase tile objects — " + std::to_string(removed) + " object(s)";
            } catch (const std::exception& error) {
                state.status = error.what();
            }
        }
    } else {
        ImGui::TextDisabled("Select a tile in the viewport.");
    }

    ImGui::SeparatorText("History");
    if (!document.canUndo()) ImGui::BeginDisabled();
    if (ImGui::Button("Undo")) {
        try { document.undo(); state.status = "Undo — PASS"; }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    if (!document.canUndo()) ImGui::EndDisabled();
    ImGui::SameLine();
    if (!document.canRedo()) ImGui::BeginDisabled();
    if (ImGui::Button("Redo")) {
        try { document.redo(); state.status = "Redo — PASS"; }
        catch (const std::exception& error) { state.status = error.what(); }
    }
    if (!document.canRedo()) ImGui::EndDisabled();

    ImGui::SeparatorText("File");
    if (ImGui::Button("Save FMAP")) {
        try {
            saveFmap(document.world(), mapPath);
            state.status = "FMAP saved";
        } catch (const std::exception& error) {
            state.status = error.what();
        }
    }

    ImGui::TextWrapped("Status: %s", state.status.c_str());
    ImGui::End();
}

void drawWorldPanel(const MapDocument& document, EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("World");
    ImGui::Text("%s", world.info.name.c_str());
    ImGui::Text("ID: %s", world.info.id.c_str());
    ImGui::Text("Tile size: %d", world.info.tileSize);
    ImGui::Text("Regions: %d", static_cast<int>(world.regions.size()));
    ImGui::Text("Development spawn: %d, %d, %d",
        world.developmentSpawn.x, world.developmentSpawn.y, static_cast<int>(world.developmentSpawn.z));

    if (ImGui::Button("Center on spawn")) {
        state.panX = 0.0f;
        state.panY = 0.0f;
        state.floor = world.developmentSpawn.z;
        state.status = "Centered on development spawn";
    }

    ImGui::SeparatorText("Regions");
    for (const auto& region : world.regions) {
        ImGui::BulletText("%s — origin %d,%d,%d — %dx%d",
            region.id.c_str(), region.origin.x, region.origin.y, static_cast<int>(region.origin.z),
            region.size.width, region.size.height);
    }
    ImGui::End();
}

void drawMiniMap(const MapDocument& document, EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("Minimap");
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.x = std::max(size.x, 180.0f);
    size.y = std::max(size.y, 140.0f);
    ImGui::InvisibleButton("##minimap", size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(20, 22, 26, 255));

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
        const float scale = std::min((size.x - 16.0f) / worldW, (size.y - 16.0f) / worldH);
        const ImVec2 base(origin.x + 8.0f, origin.y + 8.0f);
        for (const auto& region : world.regions) {
            const ImVec2 p0(base.x + (region.origin.x - minX) * scale,
                            base.y + (region.origin.y - minY) * scale);
            const ImVec2 p1(p0.x + region.size.width * scale,
                            p0.y + region.size.height * scale);
            draw->AddRectFilled(p0, p1, IM_COL32(70, 105, 80, 210));
            draw->AddRect(p0, p1, IM_COL32(180, 220, 190, 255));
        }
        const ImVec2 spawn(base.x + (world.developmentSpawn.x - minX) * scale,
                           base.y + (world.developmentSpawn.y - minY) * scale);
        draw->AddCircleFilled(spawn, 4.0f, IM_COL32(255, 220, 80, 255));
    }

    ImGui::Text("Floor %d", static_cast<int>(state.floor));
    ImGui::End();
}

int runEditor(const fs::path& projectInput) {
    const auto project = ProjectManager::openProject(projectInput);
    MapDocument document(loadFmap(project.mainMapPath));
    EditorState state;
    state.floor = document.world().developmentSpawn.z;
    if (!document.world().regions.empty() && !document.world().regions.front().chunks.empty() &&
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
        "Fantasy Studio — Map Editor",
        static_cast<int>(1440 * mainScale),
        static_cast<int>(900 * mainScale),
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr) {
        const std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow failed: " + error);
    }

    SDL_GPUDevice* gpu = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL |
        SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB,
        true, nullptr);
    if (gpu == nullptr) {
        const std::string error = SDL_GetError();
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("SDL_CreateGPUDevice failed: " + error);
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu, window)) {
        const std::string error = SDL_GetError();
        SDL_DestroyGPUDevice(gpu);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("SDL_ClaimWindowForGPUDevice failed: " + error);
    }
    SDL_SetGPUSwapchainParameters(gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(mainScale);
    ImGui::GetStyle().FontScaleDpi = mainScale;

    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo initInfo{};
    initInfo.Device = gpu;
    initInfo.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu, window);
    initInfo.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    initInfo.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    initInfo.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&initInfo);

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

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    try { saveFmap(document.world(), project.mainMapPath); state.status = "FMAP saved"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
                if (ImGui::MenuItem("Exit")) done = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, document.canUndo())) {
                    try { document.undo(); state.status = "Undo — PASS"; } catch (const std::exception& error) { state.status = error.what(); }
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, document.canRedo())) {
                    try { document.redo(); state.status = "Redo — PASS"; } catch (const std::exception& error) { state.status = error.what(); }
                }
                ImGui::EndMenu();
            }
            ImGui::TextDisabled("  %s", project.name.c_str());
            ImGui::EndMainMenuBar();
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            try { saveFmap(document.world(), project.mainMapPath); state.status = "FMAP saved"; }
            catch (const std::exception& error) { state.status = error.what(); }
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false) && document.canUndo()) {
            try { document.undo(); state.status = "Undo — PASS"; } catch (const std::exception& error) { state.status = error.what(); }
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false) && document.canRedo()) {
            try { document.redo(); state.status = "Redo — PASS"; } catch (const std::exception& error) { state.status = error.what(); }
        }

        drawWorldPanel(document, state);
        drawMapViewport(document, state);
        drawInspector(document, state, project.mainMapPath);
        drawMiniMap(document, state);

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        const bool minimized = drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f;
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gpu);
        if (commandBuffer == nullptr) {
            state.status = std::string("SDL_AcquireGPUCommandBuffer failed: ") + SDL_GetError();
            continue;
        }

        SDL_GPUTexture* swapchainTexture = nullptr;
        SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, nullptr, nullptr);
        if (swapchainTexture != nullptr && !minimized) {
            ImGui_ImplSDLGPU3_PrepareDrawData(drawData, commandBuffer);
            SDL_GPUColorTargetInfo target{};
            target.texture = swapchainTexture;
            target.clear_color = SDL_FColor{0.08f, 0.09f, 0.11f, 1.0f};
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.store_op = SDL_GPU_STOREOP_STORE;
            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &target, 1, nullptr);
            ImGui_ImplSDLGPU3_RenderDrawData(drawData, commandBuffer, renderPass);
            SDL_EndGPURenderPass(renderPass);
        }
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_WaitForGPUIdle(gpu);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();
    SDL_ReleaseWindowFromGPUDevice(gpu, window);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const fs::path project = argc > 1 ? fs::path(argv[1]) : fs::current_path();
        return runEditor(project);
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Studio GUI error: " << error.what() << '\n';
        return 1;
    }
}
