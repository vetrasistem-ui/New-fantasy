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
#include <iterator>
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
enum class Icon { Home, Map, Assets, Monster, Npc, Spell, Quest, Settings, Server, Client,
                  New, Folder, Import, Save, Undo, Redo, Play, Stop, Build, Select, Brush, Erase, Fill };

ImFont* uiFont = nullptr;
ImFont* headingFont = nullptr;
ImFont* brandFont = nullptr;

void loadStudioFonts() {
    ImGuiIO& io = ImGui::GetIO();
    const char* windowsRoot = SDL_getenv("WINDIR");
    if (windowsRoot == nullptr) windowsRoot = SDL_getenv("windir");
    if (windowsRoot == nullptr) windowsRoot = SDL_getenv("SystemRoot");
    const fs::path systemRoot = fs::path(SDL_GetBasePath()).root_path();
    const fs::path windowsFonts = windowsRoot != nullptr ? fs::path(windowsRoot) / "Fonts" : fs::path();
    const auto load = [&io](const fs::path& path, float size) -> ImFont* {
        std::error_code error;
        return fs::is_regular_file(path, error) ? io.Fonts->AddFontFromFileTTF(path.string().c_str(), size) : nullptr;
    };
    uiFont = load(windowsFonts / "segoeui.ttf", 15.0f);
    if (uiFont == nullptr) uiFont = load(windowsFonts / "arial.ttf", 15.0f);
    if (uiFont == nullptr) uiFont = load(systemRoot / "usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 15.0f);
    if (uiFont == nullptr) uiFont = load(systemRoot / "System/Library/Fonts/Supplemental/Arial.ttf", 15.0f);
    if (uiFont == nullptr) throw std::runtime_error("A neutral system UI font is required (Segoe UI, Arial or DejaVu Sans).");
    headingFont = load(windowsFonts / "seguisb.ttf", 18.0f);
    if (headingFont == nullptr) headingFont = uiFont;
    brandFont = load(windowsFonts / "georgiab.ttf", 36.0f);
    if (brandFont == nullptr) brandFont = headingFont;
    io.FontDefault = uiFont;
}

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
    std::string selectedAsset;
    std::string status = "Fantasy Studio ready";
};

struct WorkspaceLayout {
    ImVec2 contentPos{};
    ImVec2 contentSize{};
    float topbarHeight = 58.0f;
    float sidebarWidth = 184.0f;
    float gap = 8.0f;
};

struct WorldStats {
    std::size_t regions = 0;
    std::size_t chunks = 0;
    std::size_t tiles = 0;
    std::size_t objects = 0;
};

constexpr ImGuiWindowFlags kFixedWindow =
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse;

const ImVec4 kCyan(0.13f, 0.83f, 0.93f, 1.0f);
const ImVec4 kBlue(0.05f, 0.65f, 0.91f, 1.0f);
const ImVec4 kSuccess(0.06f, 0.72f, 0.51f, 1.0f);
const ImVec4 kError(0.94f, 0.27f, 0.27f, 1.0f);

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

void copyText(std::array<char, 128>& target, const std::string& value) {
    const std::size_t count = std::min<std::size_t>(target.size() - 1, value.size());
    std::copy_n(value.data(), count, target.data());
    target[count] = '\0';
}

ImU32 colorForKey(const std::string& key) {
    std::uint32_t hash = 2166136261u;
    for (const unsigned char ch : key) {
        hash ^= ch;
        hash *= 16777619u;
    }
    const int r = 48 + static_cast<int>((hash >> 0) & 0x70);
    const int g = 58 + static_cast<int>((hash >> 8) & 0x70);
    const int b = 68 + static_cast<int>((hash >> 16) & 0x70);
    return IM_COL32(r, g, b, 255);
}

bool sameLocator(const TileLocator& a, const TileLocator& b) { return a == b; }

WorkspaceLayout computeLayout() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    WorkspaceLayout layout;
    layout.sidebarWidth = viewport->WorkSize.x < 1150.0f ? 164.0f : 184.0f;
    layout.topbarHeight = 58.0f;
    layout.contentPos = ImVec2(
        viewport->WorkPos.x + layout.sidebarWidth + layout.gap,
        viewport->WorkPos.y + layout.topbarHeight + layout.gap);
    layout.contentSize = ImVec2(
        std::max(1.0f, viewport->WorkSize.x - layout.sidebarWidth - layout.gap * 2.0f),
        std::max(1.0f, viewport->WorkSize.y - layout.topbarHeight - layout.gap * 2.0f));
    return layout;
}

void drawBrandMark(ImDrawList* draw, ImVec2 center, float radius) {
    const auto p = [center, radius](float x, float y) { return ImVec2(center.x + x * radius, center.y + y * radius); };
    const ImU32 cyan = IM_COL32(34, 211, 238, 255), ice = IM_COL32(214, 236, 253, 255);
    draw->AddCircleFilled(center, radius * 0.78f, IM_COL32(14, 165, 233, 9), 48);
    // An original faceted F/sword mark: vector branding, never a styleboard texture.
    draw->AddQuadFilled(p(-0.30f,-0.54f), p(0.15f,-0.54f), p(0.15f,0.56f), p(-0.12f,0.92f), IM_COL32(4, 75, 135, 255));
    draw->AddTriangleFilled(p(-0.30f,-0.54f), p(-0.12f,0.92f), p(-0.12f,-0.40f), IM_COL32(14, 165, 233, 255));
    draw->AddTriangleFilled(p(-0.12f,-0.40f), p(-0.12f,0.92f), p(0.15f,0.56f), IM_COL32(9, 111, 187, 255));
    draw->AddLine(p(-0.30f,-0.54f), p(-0.12f,0.92f), cyan, 1.5f);
    draw->AddLine(p(-0.12f,0.92f), p(0.15f,0.56f), ice, 1.2f);
    draw->AddQuadFilled(p(-0.33f,-0.50f), p(0.52f,-0.50f), p(0.39f,-0.28f), p(-0.33f,-0.28f), IM_COL32(18, 131, 202, 255));
    draw->AddLine(p(-0.33f,-0.50f), p(0.52f,-0.50f), ice, 2.0f);
    draw->AddQuadFilled(p(-0.30f,-0.05f), p(0.39f,-0.05f), p(0.26f,0.13f), p(-0.30f,0.13f), IM_COL32(34, 211, 238, 255));
    draw->AddLine(p(-0.30f,-0.05f), p(0.39f,-0.05f), ice, 1.2f);
    draw->AddQuadFilled(p(-0.68f,-0.72f), p(0.65f,-0.72f), p(0.46f,-0.54f), p(-0.48f,-0.54f), IM_COL32(91, 169, 220, 255));
    draw->AddTriangleFilled(p(-0.68f,-0.72f), p(-0.48f,-0.54f), p(-0.71f,-0.30f), IM_COL32(16, 110, 176, 255));
    draw->AddTriangleFilled(p(0.65f,-0.72f), p(0.70f,-0.40f), p(0.46f,-0.54f), IM_COL32(16, 110, 176, 255));
    draw->AddLine(p(-0.68f,-0.72f), p(0.65f,-0.72f), ice, 1.5f);
    draw->AddQuadFilled(p(-0.02f,-0.98f), p(0.12f,-0.84f), p(-0.02f,-0.71f), p(-0.16f,-0.84f), cyan);
    draw->AddLine(p(-0.02f,-0.98f), p(-0.02f,-0.71f), ice, 1.0f);
    draw->AddQuadFilled(p(-0.02f,-0.77f), p(0.13f,-0.62f), p(-0.02f,-0.46f), p(-0.18f,-0.62f), IM_COL32(245, 181, 78, 255));
    draw->AddTriangleFilled(p(-0.02f,-0.77f), p(-0.02f,-0.46f), p(-0.18f,-0.62f), IM_COL32(255, 224, 159, 255));
}

void drawIcon(ImDrawList* draw, Icon icon, ImVec2 center, float radius, ImU32 color) {
    const auto p = [center, radius](float x, float y) { return ImVec2(center.x + x * radius, center.y + y * radius); };
    const auto line = [&](float x1, float y1, float x2, float y2) { draw->AddLine(p(x1,y1),p(x2,y2),color,1.6f); };
    const auto box = [&](float x1, float y1, float x2, float y2) { draw->AddRect(p(x1,y1),p(x2,y2),color,1.2f,0,1.5f); };
    switch (icon) {
        case Icon::Home:
            line(-.85f,-.1f,0,-.85f); line(0,-.85f,.85f,-.1f); box(-.60f,-.1f,.60f,.75f); box(-.16f,.2f,.16f,.75f); break;
        case Icon::Map: box(-.8f,-.7f,-.12f,.7f); box(.12f,-.7f,.8f,.7f); line(-.48f,-.7f,-.48f,.7f); line(.45f,-.7f,.45f,.7f); break;
        case Icon::Assets:
            draw->AddQuad(p(0,-.85f),p(.8f,-.38f),p(0,.1f),p(-.8f,-.38f),color,1.5f);
            line(-.8f,-.38f,-.8f,.4f); line(.8f,-.38f,.8f,.4f); line(-.8f,.4f,0,.85f); line(.8f,.4f,0,.85f); line(0,.1f,0,.85f); break;
        case Icon::Monster:
            draw->AddCircle(p(0,-.12f),radius*.72f,color,16,1.5f); box(-.4f,.45f,.4f,.8f);
            draw->AddCircleFilled(p(-.26f,-.13f),radius*.13f,color); draw->AddCircleFilled(p(.26f,-.13f),radius*.13f,color); break;
        case Icon::Npc: draw->AddCircle(p(0,-.4f),radius*.35f,color,16,1.5f); draw->AddBezierCubic(p(-.8f,.8f),p(-.8f,0),p(.8f,0),p(.8f,.8f),color,1.5f); line(-.8f,.8f,.8f,.8f); break;
        case Icon::Spell: draw->AddQuadFilled(p(0,-.9f),p(.22f,-.22f),p(.9f,0),p(.22f,.22f),color); draw->AddQuadFilled(p(0,.9f),p(-.22f,.22f),p(-.9f,0),p(-.22f,-.22f),color); break;
        case Icon::Quest: box(-.6f,-.8f,.6f,.8f); line(-.3f,-.4f,.3f,-.4f); line(-.3f,0,.3f,0); line(-.3f,.4f,.12f,.4f); break;
        case Icon::Server: for(int i=0;i<3;++i){const float y=-.7f+i*.55f;box(-.8f,y,.8f,y+.35f);draw->AddCircleFilled(p(-.52f,y+.17f),radius*.07f,color);} break;
        case Icon::Client: box(-.85f,-.65f,.85f,.4f); line(0,.4f,0,.8f); line(-.4f,.8f,.4f,.8f); break;
        case Icon::New: line(-.8f,0,.8f,0); line(0,-.8f,0,.8f); break;
        case Icon::Folder: box(-.85f,-.4f,.85f,.7f); line(-.85f,-.4f,-.85f,-.72f); line(-.85f,-.72f,-.1f,-.72f); line(-.1f,-.72f,.1f,-.4f); break;
        case Icon::Import: box(-.75f,-.8f,.15f,.8f); line(-.3f,0,.85f,0); line(.45f,-.4f,.85f,0); line(.45f,.4f,.85f,0); break;
        case Icon::Save: box(-.75f,-.8f,.75f,.8f); box(-.35f,-.8f,.35f,-.16f); box(-.4f,.2f,.4f,.8f); break;
        case Icon::Undo: case Icon::Redo: {
            const float sign = icon==Icon::Undo ? 1.0f : -1.0f;
            draw->AddBezierCubic(p(-.65f*sign,-.25f),p(.8f*sign,-1.2f),p(1.1f*sign,.85f),p(-.35f*sign,.7f),color,1.6f);
            line(-.65f*sign,-.25f,-.15f*sign,-.3f); line(-.65f*sign,-.25f,-.55f*sign,-.8f); break; }
        case Icon::Play: draw->AddTriangleFilled(p(-.55f,-.75f),p(.75f,0),p(-.55f,.75f),color); break;
        case Icon::Stop: draw->AddRectFilled(p(-.6f,-.6f),p(.6f,.6f),color,1.0f); break;
        case Icon::Build: box(-.7f,-.7f,.7f,.7f); box(-.26f,-.26f,.26f,.26f); break;
        case Icon::Select: draw->AddTriangle(p(-.55f,-.8f),p(.75f,.25f),p(-.32f,.5f),color,1.5f); line(-.12f,.38f,.22f,.85f); break;
        case Icon::Brush: line(-.65f,.65f,.65f,-.65f); line(-.4f,.8f,.8f,-.4f); line(.65f,-.65f,.8f,-.4f); line(-.65f,.65f,-.4f,.8f); break;
        case Icon::Erase: draw->AddQuad(p(-.8f,.2f),p(.2f,-.75f),p(.8f,-.1f),p(-.2f,.8f),color,1.5f); line(-.45f,-.14f,.22f,.48f); break;
        case Icon::Fill: draw->AddQuad(p(-.8f,0),p(0,-.8f),p(.65f,-.12f),p(-.15f,.65f),color,1.5f); line(-.8f,0,.55f,0); draw->AddCircleFilled(p(.65f,.6f),radius*.18f,color); break;
        case Icon::Settings: draw->AddCircle(center,radius*.5f,color,16,1.5f); draw->AddCircle(center,radius*.16f,color,12,1.5f); for(int i=0;i<8;++i){const float a=i*3.14159265f/4;line(std::cos(a)*.5f,std::sin(a)*.5f,std::cos(a)*.85f,std::sin(a)*.85f);} break;
    }
}

bool iconButton(const char* label, Icon icon, ImVec2 size) {
    const bool clicked = ImGui::Button((std::string("##command-") + label).c_str(), size);
    const ImVec2 min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
    drawIcon(ImGui::GetWindowDrawList(), icon, ImVec2(min.x+15.0f,(min.y+max.y)*.5f),8.0f,color);
    const ImVec2 text = ImGui::CalcTextSize(label);
    ImGui::GetWindowDrawList()->AddText(ImVec2(min.x+29.0f,(min.y+max.y-text.y)*.5f),color,label);
    return clicked;
}

void drawSectionTitle(const char* title, const char* subtitle = nullptr) {
    ImGui::PushFont(headingFont, 19.0f);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    if (subtitle != nullptr) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", subtitle);
        ImGui::PopStyleColor();
    }
}

bool accentButton(const char* label, ImVec2 size = ImVec2(0.0f, 0.0f)) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f, 0.52f, 0.78f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.05f, 0.65f, 0.91f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.04f, 0.42f, 0.66f, 1.0f));
    const bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return clicked;
}

void drawStatusPill(const char* text, const ImVec4& color) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    const ImVec2 size(textSize.x + 16.0f, textSize.y + 8.0f);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.13f)), 3.0f);
    draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.55f)), 3.0f);
    draw->AddText(ImVec2(p.x + 8.0f, p.y + 4.0f), ImGui::ColorConvertFloat4ToU32(color), text);
    ImGui::Dummy(size);
}

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

TopAction drawTopbar(const ProjectInfo& project, const MapDocument& document, const EditorState& state, const WorkspaceLayout& layout) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, layout.topbarHeight), ImGuiCond_Always);
    ImGui::Begin("##fantasy-topbar-v4", nullptr, kFixedWindow | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    draw->AddRectFilledMultiColor(origin, ImVec2(origin.x+viewport->WorkSize.x,origin.y+layout.topbarHeight),
        IM_COL32(7,18,32,255),IM_COL32(14,28,44,255),IM_COL32(10,22,37,255),IM_COL32(7,18,32,255));
    draw->AddLine(ImVec2(origin.x,origin.y+layout.topbarHeight-1),ImVec2(origin.x+viewport->WorkSize.x,origin.y+layout.topbarHeight-1),IM_COL32(44,61,81,255));
    drawBrandMark(draw, ImVec2(origin.x+27,origin.y+29), 21);
    ImGui::SetCursorPos(ImVec2(54,17));
    ImGui::PushFont(brandFont,19);
    ImGui::TextUnformatted("FANTASY");
    ImGui::PopFont();
    ImGui::SameLine(0,6);
    ImGui::PushFont(headingFont,14);
    ImGui::TextColored(kCyan,"STUDIO");
    ImGui::PopFont();
    const float controlsX = viewport->WorkSize.x-605;
    const ImVec2 contextStart(ImGui::GetItemRectMax().x+18,origin.y+21);
    draw->PushClipRect(contextStart,ImVec2(origin.x+controlsX-12,origin.y+layout.topbarHeight),true);
    const std::string context=project.name+"  /  "+pageLabel(state.page);
    draw->AddText(uiFont,13,contextStart,IM_COL32(148,163,184,255),context.c_str());
    draw->PopClipRect();
    ImGui::SetCursorPos(ImVec2(controlsX,14));
    TopAction action=TopAction::None;
    if(iconButton("Save",Icon::Save,ImVec2(76,30))) action=TopAction::Save;
    ImGui::SameLine();
    ImGui::BeginDisabled(!document.canUndo());
    if(iconButton("Undo",Icon::Undo,ImVec2(76,30))) action=TopAction::Undo;
    ImGui::EndDisabled(); ImGui::SameLine();
    ImGui::BeginDisabled(!document.canRedo());
    if(iconButton("Redo",Icon::Redo,ImVec2(76,30))) action=TopAction::Redo;
    ImGui::EndDisabled(); ImGui::SameLine(0,12);
    ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Text,kSuccess);
    iconButton("Play",Icon::Play,ImVec2(72,30));
    ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text,kError);
    iconButton("Stop",Icon::Stop,ImVec2(72,30));
    ImGui::PopStyleColor(); ImGui::SameLine();
    iconButton("Build",Icon::Build,ImVec2(78,30)); ImGui::SameLine();
    iconButton("Settings",Icon::Settings,ImVec2(98,30));
    ImGui::EndDisabled();
    ImGui::End();
    return action;
}

void drawSidebar(EditorState& state, const WorkspaceLayout& layout) {
    const ImGuiViewport* viewport=ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x,viewport->WorkPos.y+layout.topbarHeight+layout.gap),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.sidebarWidth,viewport->WorkSize.y-layout.topbarHeight-layout.gap),ImGuiCond_Always);
    ImGui::Begin("##fantasy-sidebar-v4",nullptr,kFixedWindow|ImGuiWindowFlags_NoTitleBar);
    const auto group=[](const char* label) {
        ImGui::PushFont(headingFont,11); ImGui::TextDisabled("%s",label); ImGui::PopFont();
        ImGui::Separator();
    };
    const auto nav=[&state](StudioPage page,Icon icon,const char* label) {
        ImGui::PushID(label);
        const bool selected=state.page==page;
        if(ImGui::Selectable("##nav",selected,ImGuiSelectableFlags_None,ImVec2(0,34))) {
            state.page=page; state.status=std::string("Opened ")+label;
        }
        const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        ImDrawList* draw=ImGui::GetWindowDrawList();
        if(selected) draw->AddRectFilled(a,ImVec2(a.x+3,b.y),IM_COL32(34,211,238,255));
        drawIcon(draw,icon,ImVec2(a.x+17,(a.y+b.y)*.5f),9,
                 selected?IM_COL32(34,211,238,255):IM_COL32(148,163,184,255));
        draw->AddText(ImVec2(a.x+36,(a.y+b.y-ImGui::GetFontSize())*.5f),
                      selected?IM_COL32(226,232,240,255):IM_COL32(178,192,207,255),label);
        ImGui::PopID();
    };
    group("CREATE");
    nav(StudioPage::Home,Icon::Home,"Home");
    nav(StudioPage::Map,Icon::Map,"Map");
    nav(StudioPage::ItemsAssets,Icon::Assets,"Items & Assets");
    nav(StudioPage::Monsters,Icon::Monster,"Monsters");
    nav(StudioPage::Npcs,Icon::Npc,"NPCs");
    nav(StudioPage::Spells,Icon::Spell,"Spells");
    nav(StudioPage::Quests,Icon::Quest,"Quests");
    nav(StudioPage::Systems,Icon::Settings,"Systems");
    ImGui::Spacing(); group("RUNTIME");
    nav(StudioPage::Server,Icon::Server,"Server");
    nav(StudioPage::Client,Icon::Client,"Client");
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY()+8,ImGui::GetWindowHeight()-82));
    ImGui::Separator();
    ImGui::PushFont(uiFont,12);
    drawStatusPill("FMAP NATIVE",kCyan);
    ImGui::TextDisabled("Fantasy Protocol v1");
    ImGui::TextDisabled("SDL_Renderer3");
    ImGui::PopFont();
    ImGui::End();
}

void drawHeroBanner(const ProjectInfo& project,const MapDocument& document) {
    const ImVec2 p=ImGui::GetCursorScreenPos();
    const float width=ImGui::GetContentRegionAvail().x;
    const float height=ImGui::GetWindowHeight()<720?150.0f:182.0f;
    ImDrawList* draw=ImGui::GetWindowDrawList();
    draw->PushClipRect(p,ImVec2(p.x+width,p.y+height),true);
    draw->AddRectFilledMultiColor(p,ImVec2(p.x+width,p.y+height),
        IM_COL32(4,13,24,255),IM_COL32(13,62,90,255),IM_COL32(7,31,52,255),IM_COL32(6,16,29,255));
    const ImVec2 focus(p.x+width*.86f,p.y+height*.52f);
    for(int i=5;i>0;--i) draw->AddCircleFilled(focus,34.0f+i*17,IM_COL32(14,165,233,5),64);
    for(int ring=0;ring<3;++ring) {
        const float radius=54.0f+ring*35;
        draw->AddNgon(focus,radius,IM_COL32(79,167,207,34-ring*7),6,1);
        for(int i=0;i<6;++i) {
            const float angle=i*3.14159265f/3;
            const ImVec2 v(focus.x+std::cos(angle)*radius,focus.y+std::sin(angle)*radius);
            draw->AddCircleFilled(v,ring==1?2.0f:1.0f,IM_COL32(119,208,235,90));
        }
    }
    for(int i=0;i<7;++i) {
        const float x=p.x+width*.58f+i*width*.07f;
        draw->AddLine(ImVec2(x,p.y+height),ImVec2(x+height*.8f,p.y),IM_COL32(65,142,181,16),1);
    }
    draw->AddQuadFilled(ImVec2(focus.x-58,focus.y),ImVec2(focus.x,focus.y-68),
        ImVec2(focus.x+45,focus.y),ImVec2(focus.x,focus.y+68),IM_COL32(17,115,170,38));
    draw->AddLine(ImVec2(focus.x,focus.y-68),ImVec2(focus.x,focus.y+68),IM_COL32(34,211,238,85),1);
    draw->PopClipRect();
    draw->AddRect(p,ImVec2(p.x+width,p.y+height),IM_COL32(47,88,113,180),3.0f,0,1.0f);
    drawBrandMark(draw,ImVec2(p.x+62,p.y+height*.5f),52);
    ImGui::SetCursorScreenPos(ImVec2(p.x+132,p.y+25));
    ImGui::PushFont(brandFont,height<160?33.0f:40.0f);
    ImGui::TextUnformatted("FANTASY"); ImGui::PopFont();
    ImGui::SameLine(0,10);
    ImGui::PushFont(headingFont,19); ImGui::TextColored(kCyan,"S T U D I O"); ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(p.x+134,p.y+height*.53f));
    ImGui::PushFont(uiFont,17); ImGui::TextUnformatted("Crie, edite e publique seus jogos 2D."); ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(p.x+134,p.y+height-37));
    ImGui::TextDisabled("%s  /  %s",project.name.c_str(),document.world().info.name.c_str());
    ImGui::SetCursorScreenPos(ImVec2(p.x,p.y+height+12));
}

void drawActionCard(const char* id,Icon icon,const char* title,const char* body,const char* stateLabel) {
    ImGui::BeginChild(id,ImVec2(0,124),ImGuiChildFlags_Borders);
    const ImVec2 p=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
    ImDrawList* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilledMultiColor(ImVec2(p.x+1,p.y+1),ImVec2(p.x+size.x-1,p.y+size.y-1),
        IM_COL32(16,36,56,255),IM_COL32(15,32,50,255),IM_COL32(10,23,39,255),IM_COL32(10,24,40,255));
    drawIcon(draw,icon,ImVec2(p.x+size.x*.5f,p.y+27),13,IM_COL32(34,211,238,255));
    ImGui::PushFont(headingFont,17);
    const ImVec2 text=ImGui::CalcTextSize(title);
    ImGui::SetCursorPos(ImVec2((size.x-text.x)*.5f,49)); ImGui::TextUnformatted(title); ImGui::PopFont();
    ImGui::PushFont(uiFont,13);
    ImGui::SetCursorPos(ImVec2(14,77));
    ImGui::PushTextWrapPos(size.x-14);
    ImGui::TextDisabled("%s",body);
    ImGui::PopTextWrapPos(); ImGui::PopFont();
    draw->AddText(uiFont,10,ImVec2(p.x+size.x-58,p.y+9),IM_COL32(124,144,164,255),stateLabel);
    ImGui::EndChild();
}

void drawWorldPreview(const MapDocument& document,ImVec2 size,const char* id,std::int16_t floor) {
    const ImVec2 p=ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);
    ImGui::PushID(id);
    ImDrawList* draw=ImGui::GetWindowDrawList();
    draw->PushClipRect(p,ImVec2(p.x+size.x,p.y+size.y),true);
    draw->AddRectFilled(p,ImVec2(p.x+size.x,p.y+size.y),IM_COL32(6,16,29,255),2);
    const auto& world=document.world();
    if(!world.regions.empty()) {
        int minX=world.regions.front().origin.x,minY=world.regions.front().origin.y,maxX=minX+1,maxY=minY+1;
        for(const auto& region:world.regions) {
            minX=std::min(minX,region.origin.x); minY=std::min(minY,region.origin.y);
            maxX=std::max(maxX,region.origin.x+region.size.width); maxY=std::max(maxY,region.origin.y+region.size.height);
        }
        const float scale=std::max(0.1f,std::min((size.x-8)/(maxX-minX),(size.y-8)/(maxY-minY)));
        const ImVec2 base(p.x+(size.x-(maxX-minX)*scale)*.5f,p.y+(size.y-(maxY-minY)*scale)*.5f);
        for(const auto& region:world.regions) for(const auto& chunk:region.chunks) {
            if(chunk.floor!=floor) continue;
            for(const auto& tile:chunk.tiles) {
                const ImVec2 a(base.x+(region.origin.x+chunk.x+tile.x-minX)*scale,base.y+(region.origin.y+chunk.y+tile.y-minY)*scale);
                draw->AddRectFilled(a,ImVec2(a.x+scale,a.y+scale),colorForKey(tile.ground));
                draw->AddRect(a,ImVec2(a.x+scale,a.y+scale),IM_COL32(8,18,32,90));
                if(!tile.objects.empty()&&scale>5) draw->AddCircleFilled(ImVec2(a.x+scale*.7f,a.y+scale*.3f),std::max(1.0f,scale*.09f),IM_COL32(226,232,240,230));
            }
        }
        if(world.developmentSpawn.z==floor) draw->AddCircleFilled(ImVec2(base.x+(world.developmentSpawn.x-minX+.5f)*scale,base.y+(world.developmentSpawn.y-minY+.5f)*scale),std::max(1.5f,scale*.14f),IM_COL32(245,158,11,255));
    }
    draw->PopClipRect(); ImGui::PopID();
}

void drawRecentProjectRow(const fs::path& path,bool current,const MapDocument& document) {
    ImGui::PushID(path.string().c_str());
    ImGui::BeginChild("recent-row",ImVec2(0,55),ImGuiChildFlags_Borders);
    if(current) drawWorldPreview(document,ImVec2(50,35),"recent-preview",document.world().developmentSpawn.z);
    else { const ImVec2 p=ImGui::GetCursorScreenPos(); ImGui::Dummy(ImVec2(50,35)); drawIcon(ImGui::GetWindowDrawList(),Icon::Folder,ImVec2(p.x+25,p.y+17),12,IM_COL32(148,163,184,255)); }
    ImGui::SameLine(); ImGui::BeginGroup();
    ImGui::PushFont(headingFont,15); ImGui::TextUnformatted(path.filename().string().c_str()); ImGui::PopFont();
    if(current) ImGui::TextDisabled("Projeto atual · FMAP nativo");
    else ImGui::TextDisabled("Projeto recente");
    ImGui::EndGroup(); ImGui::EndChild(); ImGui::PopID();
}

void drawMetric(const char* label, std::size_t value) {
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 72.0f);
    ImGui::TextColored(kCyan, "%d", static_cast<int>(value));
    ImGui::Separator();
}

void drawHomePage(const ProjectInfo& project,const MapDocument& document,EditorState& state,const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos,ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.contentSize,ImGuiCond_Always);
    ImGui::Begin("Home##v4",nullptr,kFixedWindow);
    drawHeroBanner(project,document);
    drawSectionTitle("Projetos");
    const float gap=8,cardWidth=(ImGui::GetContentRegionAvail().x-gap*2)/3;
    ImGui::BeginChild("home-new-wrap",ImVec2(cardWidth,124));
    drawActionCard("home-new",Icon::New,"Novo Projeto","Criação pela CLI nesta fase.","CLI");
    ImGui::EndChild(); ImGui::SameLine(0,gap);
    ImGui::BeginChild("home-open-wrap",ImVec2(cardWidth,124));
    drawActionCard("home-open",Icon::Folder,"Abrir Projeto","Seleção pelo launcher nesta fase.","Launcher");
    ImGui::EndChild(); ImGui::SameLine(0,gap);
    ImGui::BeginChild("home-import-wrap",ImVec2(cardWidth,124));
    drawActionCard("home-import",Icon::Import,"Importar Projeto","Compatibilidade ainda não iniciada.","Reservado");
    ImGui::EndChild(); ImGui::Spacing();
    const float available=ImGui::GetContentRegionAvail().x;
    const float leftWidth=(available-gap)*.57f,rightWidth=available-leftWidth-gap;
    const float panelHeight=std::max(222.0f,ImGui::GetContentRegionAvail().y);
    ImGui::BeginChild("recent-projects",ImVec2(leftWidth,panelHeight),ImGuiChildFlags_Borders);
    drawSectionTitle("Projetos Recentes");
    bool hasRecent=false;
    try {
        for(const auto& path:ProjectManager::recentProjects(project.root)) {
            std::error_code error;
            const bool current=fs::equivalent(path,project.root,error);
            drawRecentProjectRow(path,current,document); hasRecent=true;
        }
    } catch(const std::exception&) {}
    if(!hasRecent) drawRecentProjectRow(project.root,true,document);
    if(ImGui::GetContentRegionAvail().y>85) {
        ImGui::Spacing(); ImGui::TextDisabled("Mundo atual · prévia semântica FMAP");
        drawWorldPreview(document,ImVec2(ImGui::GetContentRegionAvail().x,std::max(1.0f,ImGui::GetContentRegionAvail().y)),"home-world",document.world().developmentSpawn.z);
    }
    ImGui::EndChild(); ImGui::SameLine(0,gap);
    ImGui::BeginChild("project-info",ImVec2(rightWidth,panelHeight),ImGuiChildFlags_Borders);
    drawSectionTitle("Informações do Projeto");
    ImGui::TextDisabled("Nome"); ImGui::SameLine(65); ImGui::TextUnformatted(project.name.c_str());
    ImGui::TextDisabled("Mundo"); ImGui::SameLine(65); ImGui::TextUnformatted(document.world().info.name.c_str());
    ImGui::Separator();
    const WorldStats stats=collectWorldStats(document);
    drawMetric("Regions",stats.regions); drawMetric("Chunks",stats.chunks);
    drawMetric("Tiles",stats.tiles); drawMetric("Objects",stats.objects);
    if(ImGui::GetContentRegionAvail().y>80) {
        ImGui::TextDisabled("Mapa nativo");
        ImGui::TextWrapped("%s",project.mainMapPath.lexically_relative(project.root).generic_string().c_str());
        ImGui::TextDisabled("Spawn: %d, %d, %d",document.world().developmentSpawn.x,document.world().developmentSpawn.y,document.world().developmentSpawn.z);
    }
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(),ImGui::GetWindowHeight()-40));
    if(accentButton("Abrir Map Workspace",ImVec2(-1,30))) { state.page=StudioPage::Map; state.status="Opened Map Workspace"; }
    ImGui::EndChild(); ImGui::End();
}

void drawMapToolbar(EditorState& state) {
    struct Tool { MapTool id; Icon icon; const char* label; };
    static constexpr Tool tools[]={{MapTool::Select,Icon::Select,"Select"},{MapTool::Brush,Icon::Brush,"Paint"},
        {MapTool::Fill,Icon::Fill,"Fill"},{MapTool::Erase,Icon::Erase,"Erase"}};
    for(std::size_t i=0;i<std::size(tools);++i) {
        const bool active=state.mapTool==tools[i].id;
        if(active) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.02f,.40f,.60f,1));
        if(iconButton(tools[i].label,tools[i].icon,ImVec2(78,28))) { state.mapTool=tools[i].id; state.status=std::string("Tool selected: ")+tools[i].label; }
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s · selecione o tile e execute a ação no Inspector",tools[i].label);
        if(active) ImGui::PopStyleColor();
        if(i+1<std::size(tools)) ImGui::SameLine();
    }
}

void drawMapViewport(MapDocument& document, EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("Map Editor##v4", nullptr, kFixedWindow);
    drawMapToolbar(state);
    ImGui::Separator();

    ImGui::TextDisabled("FLOOR");
    ImGui::SameLine();
    ImGui::Text("%d", static_cast<int>(state.floor));
    ImGui::SameLine(); if (ImGui::SmallButton("-")) --state.floor;
    ImGui::SameLine(); if (ImGui::SmallButton("+")) ++state.floor;
    ImGui::SameLine(0.0f, 10.0f);
    if (ImGui::SmallButton("Center")) {
        state.panX = 0.0f;
        state.panY = 0.0f;
        state.floor = world.developmentSpawn.z;
        state.status = "Centered on development spawn";
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Zoom %.2fx", state.zoom);

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 1.0f);
    canvasSize.y = std::max(canvasSize.y, 1.0f);
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##map-canvas-v4", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), IM_COL32(5, 13, 25, 255));
    draw->AddRect(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), IM_COL32(14, 165, 233, 70));

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        state.panX += delta.x;
        state.panY += delta.y;
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

    draw->PushClipRect(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), true);

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
                draw->AddRect(p0, p1, selected ? IM_COL32(34, 211, 238, 255) : IM_COL32(30, 41, 59, 255), 0.0f, 0, selected ? 3.0f : 1.0f);
                if (!tile.objects.empty() && tilePixels >= 16.0f) {
                    draw->AddCircleFilled(
                        ImVec2(p0.x + tilePixels * 0.72f, p0.y + tilePixels * 0.28f),
                        std::max(2.0f, tilePixels * 0.10f), IM_COL32(226, 232, 240, 230));
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
    draw->PopClipRect();

    draw->PushClipRect(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), true);
    if (tilePixels >= 8.0f) {
        const float offsetX = std::fmod(centerX - canvasOrigin.x - worldCenterX * tilePixels, tilePixels);
        const float offsetY = std::fmod(centerY - canvasOrigin.y - worldCenterY * tilePixels, tilePixels);
        for (float x = canvasOrigin.x + offsetX; x < canvasOrigin.x + canvasSize.x; x += tilePixels)
            draw->AddLine(ImVec2(x, canvasOrigin.y), ImVec2(x, canvasOrigin.y + canvasSize.y), IM_COL32(226, 232, 240, 15));
        for (float y = canvasOrigin.y + offsetY; y < canvasOrigin.y + canvasSize.y; y += tilePixels)
            draw->AddLine(ImVec2(canvasOrigin.x, y), ImVec2(canvasOrigin.x + canvasSize.x, y), IM_COL32(226, 232, 240, 15));
    }
    draw->PopClipRect();
    ImGui::End();
}

void drawInspector(MapDocument& document, EditorState& state, const fs::path& mapPath) {
    const auto& world = document.world();
    ImGui::Begin("Inspector##v4", nullptr, kFixedWindow);
    if (ImGui::BeginTabBar("inspector-tabs-v4")) {
        if (ImGui::BeginTabItem("Tile")) {
            if (state.selected.has_value()) {
                const auto& selected = *state.selected;
                ImGui::Text("Tile %d, %d, %d", selected.tileX, selected.tileY, static_cast<int>(selected.floor));
                ImGui::TextWrapped("Region %s | Chunk %d,%d", selected.regionId.c_str(), selected.chunkX, selected.chunkY);
                ImGui::SeparatorText("Ground");
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputText("##ground-key-v4", state.groundKey.data(), state.groundKey.size());
                if (accentButton("Paint", ImVec2(82.0f, 0.0f))) {
                    try { EditorOperations::paintGround(document, selected, state.groundKey.data()); state.status = "Paint ground — PASS"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
                ImGui::SameLine();
                if (ImGui::Button("Fill", ImVec2(82.0f, 0.0f))) {
                    try {
                        const std::size_t changed = EditorOperations::fillConnectedGround(document, selected, state.groundKey.data());
                        state.status = "Fill connected — " + std::to_string(changed) + " tile(s)";
                    } catch (const std::exception& error) { state.status = error.what(); }
                }

                ImGui::SeparatorText("Object");
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputText("##object-key-v4", state.objectKey.data(), state.objectKey.size());
                if (accentButton("Add", ImVec2(72.0f, 0.0f))) {
                    try { EditorOperations::addObject(document, selected, state.objectKey.data()); state.status = "Add object — PASS"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove", ImVec2(82.0f, 0.0f))) {
                    try { EditorOperations::removeObject(document, selected, state.objectKey.data()); state.status = "Remove object — PASS"; }
                    catch (const std::exception& error) { state.status = error.what(); }
                }
                ImGui::SameLine();
                if (ImGui::Button("Erase", ImVec2(72.0f, 0.0f))) {
                    try {
                        const std::size_t removed = EditorOperations::eraseObjects(document, selected);
                        state.status = "Erase tile objects — " + std::to_string(removed) + " object(s)";
                    } catch (const std::exception& error) { state.status = error.what(); }
                }
            } else {
                ImGui::Text("%s", world.info.name.c_str());
                ImGui::TextDisabled("%d region(s) | %d px tiles", static_cast<int>(world.regions.size()), world.info.tileSize);
                ImGui::Separator();
                ImGui::TextDisabled("Selecione um tile no mapa.");
            }

            ImGui::Separator();
            ImGui::TextDisabled("Ctrl+Z Undo · Ctrl+Y Redo");
            if (accentButton("Save FMAP", ImVec2(-1.0f, 0.0f))) {
                try { saveFmap(document.world(), mapPath); state.status = "FMAP saved"; }
                catch (const std::exception& error) { state.status = error.what(); }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Item")) {
            ImGui::TextDisabled("Item properties will attach to the shared Asset Registry.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Object")) {
            ImGui::TextDisabled("Object inspector shell ready for the real-map phase.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void drawMiniMap(const MapDocument& document,const EditorState& state) {
    ImGui::Begin("Minimap##v4",nullptr,kFixedWindow);
    const ImVec2 available=ImGui::GetContentRegionAvail();
    drawWorldPreview(document,ImVec2(std::max(1.0f,available.x),std::max(1.0f,available.y-21)),"minimap",state.floor);
    ImGui::TextDisabled("Floor %d · spawn marker",static_cast<int>(state.floor));
    ImGui::End();
}

void drawConsole(const ProjectInfo& project, const EditorState& state) {
    ImGui::Begin("Console##v4", nullptr, kFixedWindow);
    ImGui::TextColored(kCyan, "[Fantasy Studio]");
    ImGui::SameLine();
    ImGui::TextWrapped("%s", state.status.c_str());
    ImGui::Separator();
    ImGui::TextDisabled("Project: %s", project.name.c_str());
    ImGui::TextWrapped("FMAP: %s", project.mainMapPath.lexically_relative(project.root).generic_string().c_str());
    ImGui::End();
}

void drawMapPage(MapDocument& document, EditorState& state, const ProjectInfo& project, const WorkspaceLayout& layout) {
    const float rightWidth = std::clamp(layout.contentSize.x * 0.26f, 280.0f, 320.0f);
    const float consoleHeight = std::clamp(layout.contentSize.y * 0.15f, 92.0f, 120.0f);
    const float leftWidth = layout.contentSize.x - rightWidth - layout.gap;
    const float upperHeight = layout.contentSize.y - consoleHeight - layout.gap;
    const float miniHeight = std::clamp(upperHeight * 0.28f, 140.0f, 190.0f);
    const float inspectorHeight = upperHeight - miniHeight - layout.gap;

    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftWidth, upperHeight), ImGuiCond_Always);
    drawMapViewport(document, state);

    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x + leftWidth + layout.gap, layout.contentPos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightWidth, inspectorHeight), ImGuiCond_Always);
    drawInspector(document, state, project.mainMapPath);

    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x + leftWidth + layout.gap, layout.contentPos.y + inspectorHeight + layout.gap), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightWidth, miniHeight), ImGuiCond_Always);
    drawMiniMap(document, state);

    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x, layout.contentPos.y + upperHeight + layout.gap), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftWidth, consoleHeight), ImGuiCond_Always);
    drawConsole(project, state);
}

std::vector<std::string> collectSemanticAssets(const MapDocument& document, bool objects) {
    std::set<std::string> keys;
    for (const auto& region : document.world().regions) {
        for (const auto& chunk : region.chunks) {
            for (const auto& tile : chunk.tiles) {
                if (!objects) {
                    if (!tile.ground.empty()) keys.insert(tile.ground);
                } else {
                    for (const auto& object : tile.objects) if (!object.empty()) keys.insert(object);
                }
            }
        }
    }
    return std::vector<std::string>(keys.begin(), keys.end());
}

void drawAssetGrid(const std::vector<std::string>& keys, EditorState& state) {
    const float cell = 158.0f;
    const float avail = ImGui::GetContentRegionAvail().x;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const int columns = std::max(1, static_cast<int>((avail + gap) / (cell + gap)));
    const float cardWidth = (avail - gap * (columns - 1)) / columns;
    int column = 0;
    const std::string query = state.assetSearch.data();
    std::size_t shown = 0;

    for (const auto& key : keys) {
        if (!query.empty() && key.find(query) == std::string::npos) continue;
        ++shown;
        ImGui::PushID(key.c_str());
        const bool selected = state.selectedAsset == key;
        ImGui::PushStyleColor(ImGuiCol_Border, selected ? kCyan : ImGui::GetStyleColorVec4(ImGuiCol_Border));
        ImGui::BeginChild("asset-card-v4", ImVec2(cardWidth, 104.0f), ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const ImVec2 cursor = ImGui::GetCursorPos();
        if (ImGui::Selectable("##asset-selection", selected, ImGuiSelectableFlags_None,
                              ImVec2(0.0f, 88.0f))) {
            state.selectedAsset = key;
            state.status = "Asset selected: " + key;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", key.c_str());
        ImGui::SetCursorPos(cursor);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float previewWidth = ImGui::GetContentRegionAvail().x;
        ImGui::Dummy(ImVec2(previewWidth, 36.0f));
        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + previewWidth, p.y + 36.0f), colorForKey(key), 2.0f);
        ImGui::TextWrapped("%s", key.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopID();
        ++column;
        if (column < columns) ImGui::SameLine(); else column = 0;
    }
    if (shown == 0) ImGui::TextDisabled("Nenhum asset semântico corresponde ao filtro atual.");
}

void drawItemsAssetsPage(const MapDocument& document, EditorState& state, const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.contentSize, ImGuiCond_Always);
    ImGui::Begin("Items & Assets##v4", nullptr, kFixedWindow);
    drawSectionTitle("Items & Assets", "Referências semânticas do FMAP atual. Cores identificam chaves; não são sprites.");

    const auto tab = [&state](AssetTab id, const char* label) {
        const bool selected = state.assetTab == id;
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f, 0.52f, 0.78f, 1.0f));
        if (ImGui::Button(label, ImVec2(86.0f, 0.0f))) state.assetTab = id;
        if (selected) ImGui::PopStyleColor();
    };
    tab(AssetTab::Items, "Items"); ImGui::SameLine();
    tab(AssetTab::Sprites, "Sprites"); ImGui::SameLine();
    tab(AssetTab::Textures, "Textures"); ImGui::SameLine();
    tab(AssetTab::Sounds, "Sounds");
    ImGui::Separator();

    const float leftWidth = ImGui::GetContentRegionAvail().x < 900.0f ? 156.0f : 176.0f;
    ImGui::BeginChild("asset-tree-v4", ImVec2(leftWidth, 0.0f), ImGuiChildFlags_Borders);
    ImGui::TextColored(kCyan, "LIBRARY");
    ImGui::Separator();
    if (ImGui::Selectable("  All", state.assetFilter == "All", ImGuiSelectableFlags_None, ImVec2(0.0f, 32.0f))) state.assetFilter = "All";
    if (ImGui::Selectable("  Grounds", state.assetFilter == "Grounds", ImGuiSelectableFlags_None, ImVec2(0.0f, 32.0f))) state.assetFilter = "Grounds";
    if (ImGui::Selectable("  Objects", state.assetFilter == "Objects", ImGuiSelectableFlags_None, ImVec2(0.0f, 32.0f))) state.assetFilter = "Objects";
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(kCyan, "FUTURE SOURCE");
    ImGui::Text("PokeFans / 10.98");
    ImGui::TextDisabled("DAT + SPR + OTB");
    ImGui::TextDisabled("OTBM bridge: next");
    ImGui::Spacing();
    drawStatusPill("NOT CONNECTED", ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("asset-grid-v4", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##asset-search-v4", "Buscar item, ground ou object...", state.assetSearch.data(), state.assetSearch.size());
    ImGui::Separator();

    if (state.assetTab != AssetTab::Items) {
        drawStatusPill("ASSET REGISTRY NEXT", kCyan);
        ImGui::TextWrapped("Não há conteúdo deste tipo conectado nesta fase visual.");
    } else {
        const bool showGrounds = state.assetFilter == "All" || state.assetFilter == "Grounds";
        const bool showObjects = state.assetFilter == "All" || state.assetFilter == "Objects";
        if (showGrounds) {
            ImGui::TextColored(kCyan, "GROUNDS");
            drawAssetGrid(collectSemanticAssets(document, false), state);
        }
        if (showObjects) {
            ImGui::Spacing();
            ImGui::TextColored(kCyan, "OBJECTS");
            drawAssetGrid(collectSemanticAssets(document, true), state);
        }
    }
    if (!state.selectedAsset.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("Selecionado: %s", state.selectedAsset.c_str());
    }
    ImGui::EndChild();
    ImGui::End();
}

void drawModuleShell(StudioPage page, const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.contentSize, ImGuiCond_Always);
    ImGui::Begin(pageLabel(page), nullptr, kFixedWindow);
    drawSectionTitle(pageLabel(page), "Workspace reservado dentro da mesma linguagem visual do Fantasy Studio.");
    ImGui::Separator();

    ImGui::BeginChild("module-summary-v4", ImVec2(0.0f, 128.0f), ImGuiChildFlags_Borders);
    drawStatusPill("VISUAL FOUNDATION", kCyan);
    ImGui::TextWrapped("A estrutura visual está pronta sem simular regras de jogo. O módulo será conectado ao domínio compartilhado em sua fase oficial.");
    ImGui::TextDisabled("Módulo reservado — sem implementação de domínio.");
    ImGui::EndChild();
    ImGui::Spacing();

    ImGui::BeginChild("module-workspace-v4", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
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
    ImGui::TextDisabled("Próximo conteúdo real entra sem trocar shell, navegação ou Map core.");
    ImGui::EndChild();
    ImGui::End();
}

void executeTopAction(TopAction action, MapDocument& document, EditorState& state, const fs::path& mapPath) {
    try {
        switch (action) {
            case TopAction::Save:
                saveFmap(document.world(), mapPath);
                state.status = "FMAP saved";
                break;
            case TopAction::Undo:
                if (document.canUndo()) { document.undo(); state.status = "Undo — PASS"; }
                else state.status = "Nothing to undo";
                break;
            case TopAction::Redo:
                if (document.canRedo()) { document.redo(); state.status = "Redo — PASS"; }
                else state.status = "Nothing to redo";
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

    SDL_Rect usable{};
    int windowWidth = static_cast<int>(1440 * mainScale);
    int windowHeight = static_cast<int>(900 * mainScale);
    if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable)) {
        windowWidth = std::min(windowWidth, std::max(1, usable.w - 32));
        windowHeight = std::min(windowHeight, std::max(1, usable.h - 64));
    }
    SDL_Window* window = SDL_CreateWindow(
        "Fantasy Studio",
        windowWidth,
        windowHeight,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr) {
        const std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow failed: " + error);
    }
    (void)SDL_SetWindowMinimumSize(window, std::min(windowWidth, static_cast<int>(960 * mainScale)),
                                 std::min(windowHeight, static_cast<int>(640 * mainScale)));

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
    io.IniFilename = nullptr; // Fixed shell geometry must not inherit stale panel scroll/layout caches.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    loadStudioFonts();
    std::cout << "Fantasy Studio V4 renderer=" << SDL_GetRendererName(renderer)
              << " ui_font=" << uiFont->GetDebugName() << '\n';
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

        const WorkspaceLayout layout = computeLayout();
        const TopAction topAction = drawTopbar(project, document, state, layout);
        drawSidebar(state, layout);
        executeTopAction(topAction, document, state, project.mainMapPath);

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) executeTopAction(TopAction::Save, document, state, project.mainMapPath);
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) executeTopAction(TopAction::Undo, document, state, project.mainMapPath);
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) executeTopAction(TopAction::Redo, document, state, project.mainMapPath);

        switch (state.page) {
            case StudioPage::Home: drawHomePage(project, document, state, layout); break;
            case StudioPage::Map: drawMapPage(document, state, project, layout); break;
            case StudioPage::ItemsAssets: drawItemsAssetsPage(document, state, layout); break;
            default: drawModuleShell(state.page, layout); break;
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
