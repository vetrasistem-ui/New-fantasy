#include "Core/DevelopmentClient.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fp = fantasy::protocol;
namespace fc = fantasy::client;

namespace {

std::uint16_t parsePort(const std::string& value) {
    const int parsed = std::stoi(value);
    if (parsed < 1 || parsed > 65535) throw std::runtime_error("port must be between 1 and 65535");
    return static_cast<std::uint16_t>(parsed);
}

ImU32 colorForKey(const std::string& key) {
    std::uint32_t hash = 2166136261u;
    for (const unsigned char ch : key) {
        hash ^= ch;
        hash *= 16777619u;
    }
    const int r = 65 + static_cast<int>((hash >> 0) & 0x6F);
    const int g = 65 + static_cast<int>((hash >> 8) & 0x6F);
    const int b = 65 + static_cast<int>((hash >> 16) & 0x6F);
    return IM_COL32(r, g, b, 255);
}

struct Bounds {
    std::int32_t minX = 0;
    std::int32_t minY = 0;
    std::int32_t maxX = 0;
    std::int32_t maxY = 0;
    bool initialized = false;
};

Bounds worldBounds(const fc::DevelopmentClient& client) {
    Bounds bounds;
    for (const auto& received : client.chunks()) {
        for (const auto& tile : received.chunk.tiles) {
            const auto global = received.globalPosition(tile);
            if (!bounds.initialized) {
                bounds.minX = bounds.maxX = global.x;
                bounds.minY = bounds.maxY = global.y;
                bounds.initialized = true;
            } else {
                bounds.minX = std::min(bounds.minX, global.x);
                bounds.minY = std::min(bounds.minY, global.y);
                bounds.maxX = std::max(bounds.maxX, global.x);
                bounds.maxY = std::max(bounds.maxY, global.y);
            }
        }
    }
    return bounds;
}

void drawWorld(const fc::DevelopmentClient& client, const std::string& status) {
    ImGui::Begin("Fantasy World");
    ImGui::Text("Fantasy Protocol v%d", static_cast<int>(fp::kProtocolVersion));
    ImGui::Text("Entity: %llu", static_cast<unsigned long long>(client.entityId().value_or(0)));
    ImGui::Text("Position: %d, %d, %d",
        client.position().x, client.position().y, static_cast<int>(client.position().z));
    ImGui::Text("Chunks: %d", static_cast<int>(client.chunks().size()));
    ImGui::TextWrapped("Movement: arrow keys | Exit: close window");
    ImGui::TextWrapped("Status: %s", status.c_str());
    ImGui::Separator();

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 320.0f);
    canvasSize.y = std::max(canvasSize.y, 320.0f);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##world-canvas", canvasSize);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), IM_COL32(24, 26, 30, 255));

    const Bounds bounds = worldBounds(client);
    if (!bounds.initialized) {
        draw->AddText(ImVec2(origin.x + 12.0f, origin.y + 12.0f), IM_COL32_WHITE, "No MapChunk data");
        ImGui::End();
        return;
    }

    const float columns = static_cast<float>(bounds.maxX - bounds.minX + 1);
    const float rows = static_cast<float>(bounds.maxY - bounds.minY + 1);
    const float availableW = std::max(1.0f, canvasSize.x - 32.0f);
    const float availableH = std::max(1.0f, canvasSize.y - 32.0f);
    const float tilePixels = std::max(8.0f, std::min(availableW / columns, availableH / rows));
    const float mapW = columns * tilePixels;
    const float mapH = rows * tilePixels;
    const float baseX = origin.x + (canvasSize.x - mapW) * 0.5f;
    const float baseY = origin.y + (canvasSize.y - mapH) * 0.5f;

    draw->PushClipRect(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), true);
    for (const auto& received : client.chunks()) {
        for (const auto& tile : received.chunk.tiles) {
            const auto global = received.globalPosition(tile);
            if (global.z != client.position().z) continue;

            const float x0 = baseX + static_cast<float>(global.x - bounds.minX) * tilePixels;
            const float y0 = baseY + static_cast<float>(global.y - bounds.minY) * tilePixels;
            const ImVec2 p0(x0, y0);
            const ImVec2 p1(x0 + tilePixels, y0 + tilePixels);
            draw->AddRectFilled(p0, p1, colorForKey(tile.ground));
            draw->AddRect(p0, p1, IM_COL32(25, 28, 32, 180));

            if (!tile.objects.empty()) {
                draw->AddCircleFilled(
                    ImVec2(x0 + tilePixels * 0.72f, y0 + tilePixels * 0.28f),
                    std::max(2.0f, tilePixels * 0.10f),
                    IM_COL32(235, 235, 235, 230));
            }
        }
    }

    const float playerX = baseX + static_cast<float>(client.position().x - bounds.minX) * tilePixels + tilePixels * 0.5f;
    const float playerY = baseY + static_cast<float>(client.position().y - bounds.minY) * tilePixels + tilePixels * 0.5f;
    draw->AddCircleFilled(
        ImVec2(playerX, playerY),
        std::max(4.0f, tilePixels * 0.25f),
        IM_COL32(255, 214, 70, 255));
    draw->AddCircle(
        ImVec2(playerX, playerY),
        std::max(4.0f, tilePixels * 0.25f),
        IM_COL32(30, 30, 30, 255),
        0,
        2.0f);
    draw->PopClipRect();

    ImGui::End();
}

void applyMovement(fc::DevelopmentClient& client, fp::MoveDirection direction, std::string& status) {
    try {
        const auto moved = client.move(direction);
        status = "Server position: " + std::to_string(moved.x) + "," +
            std::to_string(moved.y) + "," + std::to_string(moved.z);
    } catch (const std::exception& error) {
        status = error.what();
    }
}

int runVisualClient(const std::string& host, std::uint16_t port, const std::string& character) {
    auto client = fc::DevelopmentClient::connectIpv4(host, port, 1);
    client.handshake();
    client.login(character);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (mainScale <= 0.0f) mainScale = 1.0f;

    SDL_Window* window = SDL_CreateWindow(
        "Fantasy Client — Native Play",
        static_cast<int>(1050 * mainScale),
        static_cast<int>(760 * mainScale),
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
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(mainScale);
    ImGui::GetStyle().FontScaleDpi = mainScale;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool done = false;
    std::string status = "Connected to Fantasy Server";
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

        if (!io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) applyMovement(client, fp::MoveDirection::North, status);
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) applyMovement(client, fp::MoveDirection::East, status);
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false)) applyMovement(client, fp::MoveDirection::South, status);
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) applyMovement(client, fp::MoveDirection::West, status);
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) done = true;
        }

        drawWorld(client, status);

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0.06f, 0.07f, 0.09f, 1.0f);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    try {
        client.disconnect("visual client exit");
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Client disconnect warning: " << error.what() << '\n';
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
        if (argc < 4 || std::string(argv[1]) != "--connect") {
            std::cout << "Fantasy Client GUI 0.1.0\n";
            std::cout << "usage: fantasy-client-gui --connect <ipv4|localhost> <port> [character]\n";
            return argc == 1 ? 0 : 1;
        }

        const std::string host = argv[2];
        const std::uint16_t port = parsePort(argv[3]);
        const std::string character = argc >= 5 ? argv[4] : "Development Hero";
        return runVisualClient(host, port, character);
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Client GUI error: " << error.what() << '\n';
        return 1;
    }
}
