#include "MapEngine/EditorOperations.hpp"
#include "MapEngine/FmapCore.hpp"
#include "Project/ProjectManager.hpp"
#include "UI/StudioTheme.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <dwmapi.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
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
                  New, Folder, Import, Save, Undo, Redo, Play, Stop, Build, Select, Brush, Erase, Fill,
                  Zone, Path, Event };

ImFont* uiFont = nullptr;
ImFont* headingFont = nullptr;
ImFont* brandFont = nullptr;

void styleNativeTitleBar(SDL_Window* window) {
#ifdef _WIN32
    const auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    if (hwnd != nullptr) {
        const BOOL dark = TRUE;
        const COLORREF caption = RGB(4, 16, 28);
        const COLORREF text = RGB(226, 232, 240);
        (void)DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        const HRESULT captionResult = DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
        const HRESULT textResult = DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &text, sizeof(text));
        // Older Windows versions retain their native frame if these cosmetic
        // Windows 11 attributes are unsupported. Rendering and resize stay SDL.
        if (FAILED(captionResult) || FAILED(textResult))
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Studio native caption colors unavailable; retaining platform frame.");
    }
#else
    (void)window;
#endif
}

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
    brandFont = load(fs::path(SDL_GetBasePath()) / "StudioVisual/Fonts/Cinzel.ttf", 42.0f);
    if (brandFont == nullptr) brandFont = headingFont;
    io.FontDefault = uiFont;
}

struct EditorState {
    StudioPage page = StudioPage::Home;
    MapTool mapTool = MapTool::Select;
    AssetTab assetTab = AssetTab::Items;
    std::int16_t floor = 7;
    float zoom = 1.0f;
    bool frameMap = true;
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
    float topbarHeight = 46.0f;
    float sidebarWidth = 188.0f;
    float gap = 6.0f;
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
    if(key=="terrain.grass.basic") return IM_COL32(78,124,42,255);
    if(key=="terrain.dirt.basic") return IM_COL32(135,91,49,255);
    if(key=="terrain.sand.basic") return IM_COL32(201,161,82,255);
    if(key=="terrain.water.shallow") return IM_COL32(20,108,152,255);
    if(key=="terrain.stone.basic") return IM_COL32(112,131,146,255);
    if(key=="terrain.stone.road") return IM_COL32(144,150,146,255);
    if(key=="nature.tree.oak.small") return IM_COL32(68,139,79,255);
    if(key=="nature.tree.pine.small") return IM_COL32(42,108,78,255);
    if(key=="nature.flower.blue") return IM_COL32(91,160,219,255);
    if(key=="nature.rock.small") return IM_COL32(125,135,151,255);
    return IM_COL32(61,95,119,255);
}

using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
struct PresentationArt {
    Texture emblem{nullptr,SDL_DestroyTexture}, emblemSmall{nullptr,SDL_DestroyTexture}, hero{nullptr,SDL_DestroyTexture};
    explicit PresentationArt(SDL_Renderer* renderer) {
        const auto load=[renderer](const char* name,int targetHeight=0) -> Texture {
            const fs::path file=fs::path(SDL_GetBasePath())/"StudioVisual"/name;
            std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> surface(SDL_LoadPNG(file.string().c_str()),SDL_DestroySurface);
            if(!surface) throw std::runtime_error(std::string("Studio presentation asset missing: ")+name+" — "+SDL_GetError());
            // Successive half-size filtering preserves the fine silver facets at
            // icon sizes. This only builds runtime presentation textures.
            while(targetHeight>0 && surface->h>targetHeight) {
                std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> reduced(
                    SDL_CreateSurface(std::max(1,surface->w/2),std::max(1,surface->h/2),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
                if(!reduced || !SDL_BlitSurfaceScaled(surface.get(),nullptr,reduced.get(),nullptr,SDL_SCALEMODE_LINEAR))
                    throw std::runtime_error(std::string("Studio presentation resample failed: ")+SDL_GetError());
                surface=std::move(reduced);
            }
            Texture texture(SDL_CreateTextureFromSurface(renderer,surface.get()),SDL_DestroyTexture);
            if(!texture) throw std::runtime_error(std::string("Studio presentation texture failed: ")+SDL_GetError());
            SDL_SetTextureBlendMode(texture.get(),SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_LINEAR);
            return texture;
        };
        emblem=load("fantasy-emblem-v5.png",192); emblemSmall=load("fantasy-emblem-v5.png",48);
        hero=load("fantasy-hero-v5.png");
    }
};
PresentationArt* presentation=nullptr;
ImTextureRef textureRef(SDL_Texture* texture) { return ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(texture))); }

void surface(ImDrawList* draw,ImVec2 a,ImVec2 b,bool active=false) {
    draw->AddRectFilledMultiColor(a,b,active?IM_COL32(7,83,135,255):IM_COL32(12,32,50,255),
        active?IM_COL32(10,105,164,255):IM_COL32(12,29,46,255),active?IM_COL32(3,50,84,255):IM_COL32(5,17,29,255),
        active?IM_COL32(5,59,99,255):IM_COL32(7,21,35,255));
    draw->AddRect(a,b,active?IM_COL32(28,189,235,230):IM_COL32(33,73,98,230),3.0f,0,1.0f);
    draw->AddLine(ImVec2(a.x+2,a.y+1),ImVec2(b.x-2,a.y+1),active?IM_COL32(122,230,255,110):IM_COL32(111,172,203,35));
}

void panel(const char* id,const char* title) {
    ImGui::Begin(id,nullptr,kFixedWindow|ImGuiWindowFlags_NoTitleBar);
    const ImVec2 a=ImGui::GetWindowPos(),z=ImGui::GetWindowSize();
    surface(ImGui::GetWindowDrawList(),a,ImVec2(a.x+z.x,a.y+z.y));
    ImGui::PushFont(headingFont,15.0f); ImGui::TextUnformatted(title); ImGui::PopFont();
    ImGui::Spacing();
}

bool sameLocator(const TileLocator& a, const TileLocator& b) { return a == b; }

WorkspaceLayout computeLayout(StudioPage page) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    WorkspaceLayout layout;
    layout.sidebarWidth = page == StudioPage::ItemsAssets ? 56.0f :
        (viewport->WorkSize.x < 1150.0f ? 164.0f : 188.0f);
    layout.topbarHeight = 46.0f;
    layout.contentPos = ImVec2(
        viewport->WorkPos.x + layout.sidebarWidth + layout.gap,
        viewport->WorkPos.y + layout.topbarHeight + layout.gap);
    layout.contentSize = ImVec2(
        std::max(1.0f, viewport->WorkSize.x - layout.sidebarWidth - layout.gap * 2.0f),
        std::max(1.0f, viewport->WorkSize.y - layout.topbarHeight - layout.gap * 2.0f));
    return layout;
}


void drawBrandMark(ImDrawList* draw,ImVec2 center,float radius) {
    const ImVec2 a(center.x-radius*.67f,center.y-radius), b(center.x+radius*.67f,center.y+radius);
    draw->AddImage(textureRef(radius<30?presentation->emblemSmall.get():presentation->emblem.get()),a,b);
}

void drawIconOutline(ImDrawList* draw, Icon icon, ImVec2 center, float radius, ImU32 color) {
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
        case Icon::Zone:
            box(-.75f,-.75f,.75f,.75f); line(-.25f,-.75f,-.25f,.75f); line(.25f,-.75f,.25f,.75f); line(-.75f,-.25f,.75f,-.25f); line(-.75f,.25f,.75f,.25f); break;
        case Icon::Path:
            draw->AddBezierCubic(p(-.75f,.65f),p(-.55f,-.45f),p(.5f,.45f),p(.75f,-.65f),color,1.5f);
            box(-.9f,.5f,-.6f,.8f); box(.6f,-.8f,.9f,-.5f); break;
        case Icon::Event:
            line(-.5f,-.9f,-.5f,.9f); draw->AddTriangle(p(-.45f,-.8f),p(.8f,-.4f),p(-.45f,0),color,1.5f); line(-.8f,.9f,-.2f,.9f); break;
    }
}


void drawIcon(ImDrawList* draw,Icon icon,ImVec2 center,float radius,ImU32 color) {
    const auto p=[center,radius](float x,float y){return ImVec2(center.x+x*radius,center.y+y*radius);};
    // Fixed metal/blue materials keep one icon family; caller alpha preserves
    // the real disabled state of reserved toolbar/runtime commands.
    const float alpha=static_cast<float>((color>>IM_COL32_A_SHIFT)&0xff)/255.0f;
    const auto ink=[alpha](int r,int g,int b,int a=255){return IM_COL32(r,g,b,static_cast<int>(a*alpha));};
    const ImU32 silver=ink(200,235,253),blue=ink(18,147,205),dark=ink(3,24,42),edge=ink(119,204,239,225);
    const auto line=[&](float x1,float y1,float x2,float y2,ImU32 c,float thickness=1.1f){draw->AddLine(p(x1,y1),p(x2,y2),c,thickness);};
    switch(icon) {
        case Icon::Map: {
            draw->AddQuadFilled(p(-.86f,-.61f),p(-.08f,-.76f),p(-.08f,.73f),p(-.86f,.86f),ink(18,112,168));
            draw->AddQuadFilled(p(.08f,-.76f),p(.86f,-.61f),p(.86f,.86f),p(.08f,.73f),ink(27,168,216));
            draw->AddQuadFilled(p(-.66f,-.48f),p(-.24f,-.57f),p(-.24f,.51f),p(-.66f,.61f),ink(150,219,243));
            draw->AddQuadFilled(p(.24f,-.57f),p(.66f,-.48f),p(.66f,.61f),p(.24f,.51f),ink(41,119,174));
            line(-.86f,-.61f,-.08f,-.76f,silver); line(.08f,-.76f,.86f,-.61f,silver);
            line(-.86f,-.61f,-.86f,.86f,edge); line(.86f,-.61f,.86f,.86f,edge);
            line(-.08f,-.76f,-.08f,.73f,dark); line(.08f,-.76f,.08f,.73f,silver);
            line(-.57f,-.2f,-.31f,-.25f,ink(45,119,159)); line(.33f,.14f,.58f,.19f,edge);
            return;
        }
        case Icon::Assets:
            draw->AddQuadFilled(p(0,-.87f),p(.83f,-.4f),p(0,.07f),p(-.83f,-.4f),ink(179,226,249));
            draw->AddQuadFilled(p(-.83f,-.4f),p(0,.07f),p(0,.88f),p(-.83f,.4f),ink(18,126,183));
            draw->AddQuadFilled(p(0,.07f),p(.83f,-.4f),p(.83f,.4f),p(0,.88f),ink(45,170,217));
            line(0,-.87f,.83f,-.4f,silver); line(0,-.87f,-.83f,-.4f,silver);
            line(-.83f,-.4f,0,.07f,edge); line(0,.07f,.83f,-.4f,silver);
            line(-.83f,-.4f,-.83f,.4f,edge); line(.83f,-.4f,.83f,.4f,edge);
            line(-.83f,.4f,0,.88f,silver); line(.83f,.4f,0,.88f,edge); line(0,.07f,0,.88f,silver);
            return;
        case Icon::Npc: {
            const ImVec2 shoulders[]={p(-.78f,.83f),p(-.65f,.25f),p(-.32f,.03f),p(.32f,.03f),p(.65f,.25f),p(.78f,.83f)};
            draw->AddConvexPolyFilled(shoulders,6,ink(16,105,166));
            draw->AddQuadFilled(p(-.32f,.03f),p(0,.22f),p(0,.83f),p(-.78f,.83f),ink(30,161,213));
            draw->AddTriangleFilled(p(-.32f,.03f),p(0,.22f),p(-.14f,.39f),silver);
            draw->AddTriangleFilled(p(.32f,.03f),p(.14f,.39f),p(0,.22f),ink(101,184,218));
            draw->AddCircleFilled(p(0,-.43f),radius*.38f,ink(30,143,196),20);
            draw->AddCircleFilled(p(-.055f,-.49f),radius*.29f,ink(157,220,247),20);
            draw->AddCircle(p(0,-.43f),radius*.38f,edge,20,1.0f);
            line(-.65f,.25f,-.78f,.83f,edge); line(-.78f,.83f,.78f,.83f,ink(58,116,154));
            return;
        }
        case Icon::Monster:
            draw->AddCircleFilled(p(0,-.16f),radius*.73f,ink(41,154,203),20);
            draw->AddCircleFilled(p(-.045f,-.22f),radius*.65f,ink(183,226,246),20);
            draw->AddRectFilled(p(-.39f,.24f),p(.39f,.8f),ink(129,195,224),1.0f);
            draw->AddCircle(p(0,-.16f),radius*.73f,edge,20,1.0f);
            draw->AddCircleFilled(p(-.28f,-.12f),radius*.18f,dark,12);
            draw->AddCircleFilled(p(.28f,-.12f),radius*.18f,dark,12);
            draw->AddTriangleFilled(p(0,.08f),p(-.1f,.28f),p(.1f,.28f),ink(20,81,117));
            line(-.29f,.54f,.29f,.54f,ink(14,72,109));
            line(-.13f,.54f,-.13f,.78f,ink(14,72,109)); line(.13f,.54f,.13f,.78f,ink(14,72,109));
            return;
        case Icon::Server:
            for(int i=0;i<3;++i) {
                const float y=-.81f+i*.59f;
                draw->AddRectFilledMultiColor(p(-.88f,y),p(.88f,y+.4f),ink(48,186,230),ink(24,140,190),ink(7,66,111),ink(16,112,164));
                draw->AddRect(p(-.88f,y),p(.88f,y+.4f),edge,1.0f,0,1.0f);
                draw->AddCircleFilled(p(-.58f,y+.2f),radius*.065f,silver,8);
                line(-.29f,y+.19f,.5f,y+.19f,dark); line(-.88f,y,.88f,y,silver);
            }
            return;
        case Icon::Client:
            draw->AddRectFilledMultiColor(p(-.89f,-.7f),p(.89f,.43f),ink(101,218,247),ink(20,158,211),ink(7,74,119),ink(11,116,170));
            draw->AddRectFilledMultiColor(p(-.68f,-.47f),p(.68f,.19f),ink(4,51,81),ink(8,90,126),ink(3,27,45),ink(4,35,54));
            draw->AddRect(p(-.89f,-.7f),p(.89f,.43f),edge,1.0f,0,1.0f);
            line(-.85f,-.69f,.85f,-.69f,silver);
            draw->AddRectFilled(p(-.1f,.43f),p(.1f,.82f),ink(139,201,226));
            draw->AddRectFilled(p(-.48f,.78f),p(.48f,.92f),silver);
            return;
        case Icon::Settings:
            for(int i=0;i<8;++i) {
                const float a=i*3.14159265f/4.0f;
                const auto radial=[&](float r,float offset){return p(std::cos(a+offset)*r,std::sin(a+offset)*r);};
                draw->AddQuadFilled(radial(.54f,-.22f),radial(.94f,-.13f),radial(.94f,.13f),radial(.54f,.22f),i<4?ink(35,159,208):ink(77,191,229));
                draw->AddLine(radial(.94f,-.13f),radial(.94f,.13f),edge,1.0f);
            }
            draw->AddCircleFilled(center,radius*.7f,blue,24);
            draw->AddCircleFilled(p(-.035f,-.06f),radius*.51f,ink(148,214,243),24);
            draw->AddCircleFilled(center,radius*.32f,dark,20);
            draw->AddCircle(center,radius*.7f,edge,24,1.0f);
            draw->AddCircle(center,radius*.32f,ink(41,115,151),20,1.0f);
            return;
        case Icon::Zone:
            draw->AddRectFilled(p(-.73f,-.73f),p(.73f,.73f),ink(18,102,151));
            draw->AddRect(p(-.83f,-.83f),p(.83f,.83f),edge,1.0f,0,1.1f);
            line(-.24f,-.73f,-.24f,.73f,ink(143,213,240,150)); line(.24f,-.73f,.24f,.73f,ink(143,213,240,150));
            line(-.73f,-.24f,.73f,-.24f,ink(143,213,240,150)); line(-.73f,.24f,.73f,.24f,ink(143,213,240,150));
            return;
        case Icon::Path:
            draw->AddBezierCubic(p(-.75f,.65f),p(-.55f,-.45f),p(.5f,.45f),p(.75f,-.65f),blue,3.0f);
            draw->AddBezierCubic(p(-.75f,.65f),p(-.55f,-.45f),p(.5f,.45f),p(.75f,-.65f),silver,1.0f);
            draw->AddRectFilled(p(-.91f,.49f),p(-.59f,.81f),silver,1.0f);
            draw->AddRectFilled(p(.59f,-.81f),p(.91f,-.49f),silver,1.0f);
            return;
        case Icon::Event:
            draw->AddTriangleFilled(p(-.45f,-.83f),p(.8f,-.4f),p(-.45f,.03f),blue);
            draw->AddTriangleFilled(p(-.45f,-.83f),p(.8f,-.4f),p(-.45f,-.38f),ink(140,218,245));
            line(-.5f,-.9f,-.5f,.88f,silver,1.4f); line(-.8f,.88f,-.2f,.88f,edge);
            return;
        default: break;
    }
    if(icon==Icon::Folder) {
        draw->AddRectFilledMultiColor(p(-.85f,-.4f),p(.85f,.7f),ink(41,192,239),ink(17,171,227),ink(4,88,147),ink(6,115,176));
        draw->AddRectFilled(p(-.85f,-.72f),p(-.1f,-.36f),ink(41,180,221));
    } else if(icon==Icon::Home) {
        draw->AddTriangleFilled(p(-.88f,-.1f),p(0,-.9f),p(.88f,-.1f),ink(17,182,229));
        draw->AddRectFilledMultiColor(p(-.6f,-.1f),p(.6f,.75f),ink(19,122,177),ink(10,159,211),ink(4,66,112),ink(7,91,142));
    } else if(icon==Icon::New) {
        draw->AddRectFilled(p(-.85f,-.13f),p(.85f,.13f),ink(14,174,228));
        draw->AddRectFilled(p(-.13f,-.85f),p(.13f,.85f),ink(14,174,228));
    }
    drawIconOutline(draw,icon,ImVec2(center.x,center.y+1.2f),radius,ink(1,8,17,240));
    drawIconOutline(draw,icon,center,radius,color);
    drawIconOutline(draw,icon,ImVec2(center.x-.35f,center.y-.5f),radius*.94f,ink(182,232,253,150));
}

bool iconButton(const char* label,Icon icon,ImVec2 size,bool iconOnly=false) {
    const bool clicked=ImGui::InvisibleButton((std::string("##command-")+label).c_str(),size);
    const bool hovered=ImGui::IsItemHovered(),pressed=ImGui::IsItemActive();
    const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    ImDrawList* draw=ImGui::GetWindowDrawList();
    surface(draw,a,b,hovered||pressed);
    const ImU32 ink=ImGui::GetColorU32(ImGuiCol_Text);
    drawIcon(draw,icon,ImVec2(a.x+(iconOnly?size.x*.5f:14),(a.y+b.y)*.5f),8.5f,ink);
    if(!iconOnly) draw->AddText(ImVec2(a.x+28,a.y+(size.y-ImGui::GetFontSize())*.5f),ink,label);
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s",label);
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


TopAction drawTopbar(const ProjectInfo& project,const MapDocument& document,const EditorState& state,const WorkspaceLayout& layout) {
    const ImGuiViewport* vp=ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos,ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x,layout.topbarHeight),ImGuiCond_Always);
    ImGui::Begin("##topbar-v5",nullptr,kFixedWindow|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoScrollbar);
    ImDrawList* draw=ImGui::GetWindowDrawList(); const ImVec2 a=ImGui::GetWindowPos();
    surface(draw,a,ImVec2(a.x+vp->WorkSize.x,a.y+layout.topbarHeight));
    drawBrandMark(draw,ImVec2(a.x+26,a.y+23),20);
    if(state.page!=StudioPage::ItemsAssets)
        draw->AddText(uiFont,14.0f,ImVec2(a.x+45,a.y+15),IM_COL32(226,239,249,255),"Fantasy Studio");
    ImGui::SetCursorPos(ImVec2(layout.sidebarWidth+12,9));
    TopAction action=TopAction::None;
    if(iconButton("Save",Icon::Save,ImVec2(30,28),true)) action=TopAction::Save;
    ImGui::SameLine(0,5); ImGui::BeginDisabled(!document.canUndo());
    if(iconButton("Undo",Icon::Undo,ImVec2(30,28),true)) action=TopAction::Undo;
    ImGui::EndDisabled(); ImGui::SameLine(0,5); ImGui::BeginDisabled(!document.canRedo());
    if(iconButton("Redo",Icon::Redo,ImVec2(30,28),true)) action=TopAction::Redo;
    ImGui::EndDisabled();
    const float controls=vp->WorkSize.x-286;
    const ImVec2 context(a.x+layout.sidebarWidth+126,a.y+16);
    draw->PushClipRect(context,ImVec2(a.x+controls-15,a.y+40),true);
    const std::string breadcrumb=project.name+"  /  "+pageLabel(state.page);
    draw->AddText(uiFont,13.0f,context,IM_COL32(119,154,179,255),breadcrumb.c_str()); draw->PopClipRect();
    ImGui::SetCursorPos(ImVec2(controls,9));
    ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Text,kCyan);
    iconButton("Play",Icon::Play,ImVec2(72,28)); ImGui::PopStyleColor(); ImGui::SameLine(0,6);
    ImGui::PushStyleColor(ImGuiCol_Text,kError);
    iconButton("Stop",Icon::Stop,ImVec2(72,28)); ImGui::PopStyleColor(); ImGui::SameLine(0,6);
    iconButton("Build",Icon::Build,ImVec2(72,28)); ImGui::SameLine(0,8);
    iconButton("Settings",Icon::Settings,ImVec2(30,28),true);
    ImGui::EndDisabled(); ImGui::End(); return action;
}

void drawSidebar(EditorState& state,const WorkspaceLayout& layout) {
    const ImGuiViewport* vp=ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x,vp->WorkPos.y+layout.topbarHeight),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.sidebarWidth,vp->WorkSize.y-layout.topbarHeight),ImGuiCond_Always);
    ImGui::Begin("##sidebar-v5",nullptr,kFixedWindow|ImGuiWindowFlags_NoTitleBar);
    const ImVec2 p=ImGui::GetWindowPos(),z=ImGui::GetWindowSize();
    ImDrawList* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilledMultiColor(p,ImVec2(p.x+z.x,p.y+z.y),IM_COL32(7,25,40,255),IM_COL32(4,18,31,255),IM_COL32(4,16,28,255),IM_COL32(7,22,36,255));
    draw->AddLine(ImVec2(p.x+z.x-1,p.y),ImVec2(p.x+z.x-1,p.y+z.y),IM_COL32(27,66,91,255));
    const bool compact=state.page==StudioPage::ItemsAssets;
    const auto nav=[&](StudioPage page,Icon icon,const char* label) {
        ImGui::PushID(label); const bool selected=state.page==page;
        if(ImGui::InvisibleButton("##nav",ImVec2(ImGui::GetContentRegionAvail().x,38))) {state.page=page;state.status=std::string("Opened ")+label;}
        const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        if(selected||ImGui::IsItemHovered()) surface(draw,a,b,selected);
        drawIcon(draw,icon,ImVec2(a.x+18,(a.y+b.y)*.5f),10.0f,selected?IM_COL32(36,220,255,255):IM_COL32(129,194,225,255));
        if(!compact)draw->AddText(ImVec2(a.x+39,a.y+11),IM_COL32(226,238,249,255),label);
        else if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",label);
        ImGui::PopID();
    };
    nav(StudioPage::Home,Icon::Home,"Home"); nav(StudioPage::Map,Icon::Map,"Map");
    nav(StudioPage::ItemsAssets,Icon::Assets,"Items & Assets"); nav(StudioPage::Monsters,Icon::Monster,"Monsters");
    nav(StudioPage::Npcs,Icon::Npc,"NPCs"); nav(StudioPage::Spells,Icon::Spell,"Spells");
    nav(StudioPage::Quests,Icon::Quest,"Quests"); nav(StudioPage::Systems,Icon::Settings,"Systems");
    nav(StudioPage::Server,Icon::Server,"Server"); nav(StudioPage::Client,Icon::Client,"Client");
    ImGui::End();
}

void drawHeroBanner(const ProjectInfo&,const MapDocument&) {
    const ImVec2 p=ImGui::GetCursorScreenPos(); const float width=ImGui::GetContentRegionAvail().x;
    const float height=ImGui::GetWindowHeight()<720?140.0f:182.0f;
    ImDrawList* draw=ImGui::GetWindowDrawList(); const ImVec2 end(p.x+width,p.y+height);
    draw->PushClipRect(p,end,true);
    draw->AddRectFilled(p,end,IM_COL32(3,14,25,255));
    const float visible=std::min(1.0f,3.0f*height/width);
    draw->AddImage(textureRef(presentation->hero.get()),p,end,
        ImVec2(0,(1-visible)*.42f),ImVec2(1,(1-visible)*.42f+visible));
    draw->AddRectFilled(p,ImVec2(p.x+width*.20f,p.y+height),IM_COL32(3,14,25,255));
    draw->AddRectFilledMultiColor(ImVec2(p.x+width*.20f,p.y),ImVec2(p.x+width*.52f,p.y+height),
        IM_COL32(3,14,25,255),IM_COL32(3,14,25,0),IM_COL32(3,14,25,0),IM_COL32(3,14,25,255));
    drawBrandMark(draw,ImVec2(p.x+width*.08f,p.y+height*.5f),height*.40f);
    const float brandSize=width<900?36.0f:42.0f;
    const ImVec2 brand(p.x+width*.15f,p.y+height*.29f);
    draw->AddText(brandFont,brandSize,ImVec2(brand.x+1,brand.y+2),IM_COL32(0,8,18,255),"FANTASY");
    draw->AddText(brandFont,brandSize,ImVec2(brand.x+.6f,brand.y),IM_COL32(154,176,196,255),"FANTASY");
    draw->AddText(brandFont,brandSize,brand,IM_COL32(234,243,250,255),"FANTASY");
    const float fantasyWidth=brandFont->CalcTextSizeA(brandSize,9999,0,"FANTASY").x;
    draw->AddText(uiFont,19.0f,ImVec2(brand.x+fantasyWidth+12,brand.y+12),IM_COL32(34,211,238,255),"S T U D I O");
    draw->AddText(uiFont,16.0f,ImVec2(brand.x,p.y+height*.60f),IM_COL32(205,226,240,255),"Crie, edite e publique seus jogos 2D.");
    draw->PopClipRect(); draw->AddRect(p,end,IM_COL32(38,118,157,215),4.0f,0,1.0f);
    ImGui::Dummy(ImVec2(width,height));
}

void drawActionCard(const char* id,Icon icon,const char* title,const char* body,const char* stateLabel) {
    ImGui::BeginChild(id,ImVec2(0,138),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p=ImGui::GetWindowPos(),z=ImGui::GetWindowSize();
    ImDrawList* draw=ImGui::GetWindowDrawList(); surface(draw,p,ImVec2(p.x+z.x,p.y+z.y));
    drawIcon(draw,icon,ImVec2(p.x+z.x*.5f,p.y+34),16,IM_COL32(18,190,239,255));
    ImGui::PushFont(headingFont,18); const ImVec2 label=ImGui::CalcTextSize(title);
    ImGui::SetCursorPos(ImVec2((z.x-label.x)*.5f,58)); ImGui::TextUnformatted(title); ImGui::PopFont();
    ImGui::PushFont(uiFont,13);
    std::string lines(body); std::size_t start=0; float lineY=91;
    do {
        const std::size_t end=lines.find('\n',start);
        const std::string line=lines.substr(start,end==std::string::npos?end:end-start);
        const ImVec2 text=ImGui::CalcTextSize(line.c_str());
        ImGui::SetCursorPos(ImVec2(std::max(8.0f,(z.x-text.x)*.5f),lineY)); ImGui::TextUnformatted(line.c_str());
        if(end==std::string::npos)break;
        start=end+1; lineY+=17;
    }while(start<lines.size());
    ImGui::PopFont();
    if(ImGui::IsWindowHovered()) ImGui::SetTooltip("%s",stateLabel);
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
    ImGui::SetNextWindowPos(layout.contentPos,ImGuiCond_Always); ImGui::SetNextWindowSize(layout.contentSize,ImGuiCond_Always);
    ImGui::Begin("Home##v5",nullptr,kFixedWindow|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground);
    drawHeroBanner(project,document); ImGui::Spacing();
    ImGui::PushFont(headingFont,15); ImGui::TextUnformatted("Projetos"); ImGui::PopFont();
    const float gap=8,cardWidth=(ImGui::GetContentRegionAvail().x-gap*2)/3;
    ImGui::BeginChild("new-wrap",ImVec2(cardWidth,138)); drawActionCard("new",Icon::New,"Novo Projeto","Crie um novo projeto\ndo zero","Criação pela CLI nesta fase"); ImGui::EndChild(); ImGui::SameLine(0,gap);
    ImGui::BeginChild("open-wrap",ImVec2(cardWidth,138)); drawActionCard("open",Icon::Folder,"Abrir Projeto","Abra um projeto\nexistente","Seleção de projeto pelo launcher nesta fase"); ImGui::EndChild(); ImGui::SameLine(0,gap);
    ImGui::BeginChild("import-wrap",ImVec2(cardWidth,138)); drawActionCard("import",Icon::Import,"Importar Projeto","Importe um projeto\nde outra fonte","Importação reservada; compatibilidade não iniciada"); ImGui::EndChild(); ImGui::Spacing();
    const float available=ImGui::GetContentRegionAvail().x,left=(available-gap)*.56f;
    const float height=std::max(245.0f,ImGui::GetContentRegionAvail().y);
    ImGui::BeginChild("recent-projects",ImVec2(left,height),ImGuiChildFlags_AlwaysUseWindowPadding);
    const ImVec2 a=ImGui::GetWindowPos(),z=ImGui::GetWindowSize(); surface(ImGui::GetWindowDrawList(),a,ImVec2(a.x+z.x,a.y+z.y));
    ImGui::PushFont(headingFont,15); ImGui::TextUnformatted("Projetos Recentes"); ImGui::PopFont(); bool recent=false;
    try { for(const auto& path:ProjectManager::recentProjects(project.root)) {
        std::error_code error; const bool current=fs::equivalent(path,project.root,error);
        drawRecentProjectRow(path,current,document); recent=true;
    }} catch(const std::exception&) {}
    if(!recent) drawRecentProjectRow(project.root,true,document);
    ImGui::Spacing(); ImGui::TextDisabled("%s","Projetos disponíveis neste workspace");
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(),height-62));
    ImGui::Separator(); ImGui::TextDisabled("Mapa principal");
    ImGui::TextWrapped("%s",project.mainMapPath.lexically_relative(project.root).generic_string().c_str());
    ImGui::EndChild(); ImGui::SameLine(0,gap);
    ImGui::BeginChild("project-info",ImVec2(available-left-gap,height),ImGuiChildFlags_AlwaysUseWindowPadding);
    const ImVec2 b=ImGui::GetWindowPos(),q=ImGui::GetWindowSize(); surface(ImGui::GetWindowDrawList(),b,ImVec2(b.x+q.x,b.y+q.y));
    ImGui::PushFont(headingFont,15); ImGui::TextUnformatted("Informações do Projeto"); ImGui::PopFont();
    drawWorldPreview(document,ImVec2(ImGui::GetContentRegionAvail().x,76),"project-preview",document.world().developmentSpawn.z);
    ImGui::Spacing(); ImGui::TextDisabled("Nome:"); ImGui::SameLine(64); ImGui::TextColored(kCyan,"%s",project.name.c_str());
    ImGui::TextDisabled("Mundo:"); ImGui::SameLine(64); ImGui::TextUnformatted(document.world().info.name.c_str());
    ImGui::TextDisabled("Formato:"); ImGui::SameLine(64); ImGui::TextUnformatted("FMAP nativo");
    const WorldStats stats=collectWorldStats(document);
    ImGui::TextDisabled("%d região · %d chunks · %d tiles · %d objetos",static_cast<int>(stats.regions),static_cast<int>(stats.chunks),static_cast<int>(stats.tiles),static_cast<int>(stats.objects));
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY()+5,height-40));
    if(accentButton("Abrir Mapa",ImVec2(-1,30))) {state.page=StudioPage::Map;state.status="Opened Map Workspace";}
    ImGui::EndChild(); ImGui::End();
}


void drawMapToolbar(EditorState& state) {
    struct Tool{MapTool id;Icon icon;const char* label;};
    static constexpr Tool tools[]={{MapTool::Select,Icon::Select,"Select"},{MapTool::Brush,Icon::Brush,"Paint"},{MapTool::Fill,Icon::Fill,"Fill"},{MapTool::Erase,Icon::Erase,"Erase"}};
    for(const auto& tool:tools) {
        if(iconButton(tool.label,tool.icon,ImVec2(29,28),true)) {state.mapTool=tool.id;state.status=std::string("Tool selected: ")+tool.label;}
        if(state.mapTool==tool.id) {const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax(); ImGui::GetWindowDrawList()->AddRect(a,b,IM_COL32(34,211,238,255),3.0f,0,1.5f);}
        ImGui::SameLine(0,5);
    }
    ImGui::BeginDisabled(); iconButton("Tile",Icon::Map,ImVec2(29,28),true); ImGui::SameLine(0,5);
    iconButton("Object",Icon::Assets,ImVec2(29,28),true); ImGui::SameLine(0,5);
    iconButton("Spawn",Icon::Npc,ImVec2(29,28),true); ImGui::SameLine(0,5);
    iconButton("Zone",Icon::Zone,ImVec2(29,28),true); ImGui::SameLine(0,5);
    iconButton("Path",Icon::Path,ImVec2(29,28),true); ImGui::SameLine(0,5);
    iconButton("Event",Icon::Event,ImVec2(29,28),true); ImGui::SameLine(0,5);
    iconButton("Config",Icon::Settings,ImVec2(29,28),true); ImGui::EndDisabled();
}

void drawMapViewport(MapDocument& document, EditorState& state) {
    const auto& world = document.world();
    ImGui::Begin("Map Editor##v5", nullptr, kFixedWindow|ImGuiWindowFlags_NoTitleBar);
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
        state.frameMap = true;
        state.floor = world.developmentSpawn.z;
        state.status = "Framed current map";
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Zoom %.2fx", state.zoom);

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 1.0f);
    canvasSize.y = std::max(canvasSize.y, 1.0f);
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##map-canvas-v5", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
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

    if(state.frameMap && !world.regions.empty()) {
        int minX=world.regions.front().origin.x,minY=world.regions.front().origin.y,maxX=minX+1,maxY=minY+1;
        for(const auto& region:world.regions) { minX=std::min(minX,region.origin.x); minY=std::min(minY,region.origin.y);
            maxX=std::max(maxX,region.origin.x+region.size.width); maxY=std::max(maxY,region.origin.y+region.size.height); }
        const float tile=static_cast<float>(world.info.tileSize);
        state.zoom=std::clamp(std::min(canvasSize.x/(maxX-minX+1),canvasSize.y/(maxY-minY+1))/tile,.25f,4.0f);
        state.panX=(world.developmentSpawn.x-(minX+maxX)*.5f)*tile*state.zoom;
        state.panY=(world.developmentSpawn.y-(minY+maxY)*.5f)*tile*state.zoom;
        state.frameMap=false;
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
    panel("Inspector##v5","Inspector");
    if (ImGui::BeginTabBar("inspector-tabs-v5")) {
        if (ImGui::BeginTabItem("Tile")) {
            if (state.selected.has_value()) {
                const auto& selected = *state.selected;
                const fantasy::studio::map::Tile* currentTile=nullptr;
                int globalX=selected.tileX,globalY=selected.tileY;
                for(const auto& region:world.regions) if(region.id==selected.regionId) {
                    globalX+=region.origin.x+selected.chunkX; globalY+=region.origin.y+selected.chunkY;
                    for(const auto& chunk:region.chunks) if(chunk.x==selected.chunkX && chunk.y==selected.chunkY && chunk.floor==selected.floor)
                        for(const auto& tile:chunk.tiles) if(tile.x==selected.tileX && tile.y==selected.tileY) currentTile=&tile;
                }
                if(currentTile) {
                    const ImVec2 p=ImGui::GetCursorScreenPos(); ImGui::Dummy(ImVec2(48,44));
                    ImDrawList* draw=ImGui::GetWindowDrawList();
                    draw->AddRectFilled(p,ImVec2(p.x+44,p.y+44),colorForKey(currentTile->ground),2.0f);
                    draw->AddRect(p,ImVec2(p.x+44,p.y+44),IM_COL32(102,190,222,180),2.0f,0,1.0f);
                    if(!currentTile->objects.empty())draw->AddCircleFilled(ImVec2(p.x+31,p.y+12),3,IM_COL32(232,243,251,255));
                    ImGui::SameLine();
                }
                ImGui::BeginGroup();
                ImGui::Text("Tile %d, %d, %d",globalX,globalY,static_cast<int>(selected.floor));
                ImGui::TextDisabled("Chunk %d,%d",selected.chunkX,selected.chunkY);
                ImGui::EndGroup();
                ImGui::SeparatorText("Ground");
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputText("##ground-key-v5", state.groundKey.data(), state.groundKey.size());
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
                ImGui::InputText("##object-key-v5", state.objectKey.data(), state.objectKey.size());
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
    panel("Minimap##v5","Minimap");
    const ImVec2 available=ImGui::GetContentRegionAvail();
    drawWorldPreview(document,ImVec2(std::max(1.0f,available.x),std::max(1.0f,available.y-21)),"minimap",state.floor);
    ImGui::TextDisabled("Floor %d · spawn marker",static_cast<int>(state.floor));
    ImGui::End();
}


void drawConsole(const ProjectInfo& project,const EditorState& state) {
    panel("Console##v5","Console");
    ImGui::TextColored(kCyan,"[Studio] %s",state.status.c_str());
    ImGui::TextDisabled("%s · %s",project.name.c_str(),project.mainMapPath.lexically_relative(project.root).generic_string().c_str());
    ImGui::End();
}

void drawMapPage(MapDocument& document,EditorState& state,const ProjectInfo& project,const WorkspaceLayout& layout) {
    const float right=std::clamp(layout.contentSize.x*.255f,280.0f,310.0f),console=82;
    const float left=layout.contentSize.x-right-layout.gap,upper=layout.contentSize.y-console-layout.gap;
    const float mini=std::clamp(upper*.23f,122.0f,155.0f),inspector=upper-mini-layout.gap;
    ImGui::SetNextWindowPos(layout.contentPos,ImGuiCond_Always); ImGui::SetNextWindowSize(ImVec2(left,upper),ImGuiCond_Always); drawMapViewport(document,state);
    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x+left+layout.gap,layout.contentPos.y),ImGuiCond_Always); ImGui::SetNextWindowSize(ImVec2(right,inspector),ImGuiCond_Always); drawInspector(document,state,project.mainMapPath);
    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x+left+layout.gap,layout.contentPos.y+inspector+layout.gap),ImGuiCond_Always); ImGui::SetNextWindowSize(ImVec2(right,mini),ImGuiCond_Always); drawMiniMap(document,state);
    ImGui::SetNextWindowPos(ImVec2(layout.contentPos.x,layout.contentPos.y+upper+layout.gap),ImGuiCond_Always); ImGui::SetNextWindowSize(ImVec2(layout.contentSize.x,console),ImGuiCond_Always); drawConsole(project,state);
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


std::string assetLabel(const std::string& key) {
    if(key=="terrain.grass.basic")return "Grama"; if(key=="terrain.dirt.basic")return "Terra";
    if(key=="terrain.sand.basic")return "Areia"; if(key=="terrain.water.shallow")return "Água";
    if(key=="terrain.stone.basic")return "Pedra"; if(key=="terrain.stone.road")return "Estrada";
    if(key=="nature.tree.oak.small")return "Carvalho"; if(key=="nature.tree.pine.small")return "Pinheiro";
    if(key=="nature.flower.blue")return "Flor azul"; if(key=="nature.rock.small")return "Rocha";
    return key;
}

void drawAssetGrid(const std::vector<std::string>& keys,EditorState& state) {
    const float width=ImGui::GetContentRegionAvail().x,cell=86,gap=8;
    const int columns=std::max(1,static_cast<int>((width+gap)/(cell+gap))); int col=0; std::size_t shown=0;
    for(const auto& key:keys) {
        const std::string label=assetLabel(key);
        if(state.assetSearch[0] && !SDL_strcasestr(key.c_str(),state.assetSearch.data()) &&
           !SDL_strcasestr(label.c_str(),state.assetSearch.data()))continue;
        ++shown; ImGui::PushID(key.c_str()); ImGui::BeginGroup();
        const bool selected=state.selectedAsset==key;
        if(ImGui::InvisibleButton("##asset",ImVec2(cell,cell))) {state.selectedAsset=key;state.status="Asset selected: "+key;}
        const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax(); ImDrawList* draw=ImGui::GetWindowDrawList();
        surface(draw,a,b,selected||ImGui::IsItemHovered());
        const ImVec2 x(a.x+17,a.y+15),y(b.x-17,b.y-19);
        draw->AddRectFilled(x,y,colorForKey(key),2.0f);
        draw->AddRect(x,y,IM_COL32(182,218,239,120),2.0f,0,1.0f);
        if(key.starts_with("nature.")) drawIcon(draw,Icon::Assets,ImVec2((x.x+y.x)*.5f,(x.y+y.y)*.5f),12,IM_COL32(229,241,249,230));
        if(ImGui::IsItemHovered())ImGui::SetTooltip("%s\nReferência semântica FMAP · sem sprite",key.c_str());
        ImGui::PushFont(uiFont,12); const ImVec2 text=ImGui::CalcTextSize(label.c_str());
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.0f,(cell-text.x)*.5f)); ImGui::TextUnformatted(label.c_str()); ImGui::PopFont();
        ImGui::EndGroup(); ImGui::PopID(); if(++col<columns)ImGui::SameLine(0,gap); else col=0;
    }
    if(!shown)ImGui::TextDisabled("Nenhuma referência neste filtro.");
}

void drawItemsAssetsPage(const MapDocument& document,EditorState& state,const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos,ImGuiCond_Always); ImGui::SetNextWindowSize(layout.contentSize,ImGuiCond_Always);
    ImGui::Begin("Items & Assets##v5",nullptr,kFixedWindow|ImGuiWindowFlags_NoTitleBar);
    const auto tab=[&](AssetTab id,const char* name){const bool active=state.assetTab==id;
        if(active)ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.01f,.39f,.65f,1));
        if(ImGui::Button(name,ImVec2(112,28)))state.assetTab=id;
        if(active)ImGui::PopStyleColor();};
    tab(AssetTab::Items,"Items"); ImGui::SameLine(0,5); tab(AssetTab::Sprites,"Sprites"); ImGui::SameLine(0,5);
    tab(AssetTab::Textures,"Textures"); ImGui::SameLine(0,5); tab(AssetTab::Sounds,"Sounds"); ImGui::Spacing();
    ImGui::SetNextItemWidth(-1); ImGui::InputTextWithHint("##asset-search-v5","Buscar...",state.assetSearch.data(),state.assetSearch.size()); ImGui::Spacing();
    const float tree=156;
    ImGui::BeginChild("asset-tree-v5",ImVec2(tree,-26),ImGuiChildFlags_Borders);
    const auto category=[&](const char* id,const char* label,Icon icon){
        const bool selected=state.assetFilter==id; ImGui::PushID(id);
        if(ImGui::InvisibleButton("##category",ImVec2(ImGui::GetContentRegionAvail().x,32)))state.assetFilter=id;
        const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax(); ImDrawList* draw=ImGui::GetWindowDrawList();
        if(selected||ImGui::IsItemHovered())surface(draw,a,b,selected);
        drawIcon(draw,icon,ImVec2(a.x+14,a.y+16),8,IM_COL32(155,217,245,255)); draw->AddText(ImVec2(a.x+31,a.y+8),IM_COL32(218,232,243,255),label); ImGui::PopID();};
    category("All","Biblioteca",Icon::Assets); category("Grounds","Terrenos",Icon::Folder); category("Objects","Objetos",Icon::Folder);
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY()+15,ImGui::GetWindowHeight()-68)); ImGui::Separator();
    ImGui::TextDisabled("FMAP nativo"); ImGui::TextDisabled("Sprites não conectados"); ImGui::EndChild(); ImGui::SameLine(0,8);
    ImGui::BeginChild("asset-grid-v5",ImVec2(0,-26),ImGuiChildFlags_Borders);
    if(state.assetTab!=AssetTab::Items){ImGui::TextDisabled("Nenhum conteúdo conectado nesta aba.");}
    else {
        if(state.assetFilter=="All"||state.assetFilter=="Grounds") {ImGui::TextUnformatted("Terrenos"); ImGui::Spacing(); drawAssetGrid(collectSemanticAssets(document,false),state);}
        if(state.assetFilter=="All"||state.assetFilter=="Objects") {ImGui::Spacing(); ImGui::Separator(); ImGui::TextUnformatted("Objetos"); ImGui::Spacing(); drawAssetGrid(collectSemanticAssets(document,true),state);}
    }
    ImGui::EndChild();
    ImGui::TextDisabled("%s",state.selectedAsset.empty()?"Biblioteca semântica do mapa atual":state.selectedAsset.c_str()); ImGui::End();
}

void drawModuleShell(StudioPage page, const WorkspaceLayout& layout) {
    ImGui::SetNextWindowPos(layout.contentPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(layout.contentSize, ImGuiCond_Always);
    panel(pageLabel(page),pageLabel(page));
    drawSectionTitle(pageLabel(page), "Workspace reservado dentro da mesma linguagem visual do Fantasy Studio.");
    ImGui::Separator();

    ImGui::BeginChild("module-summary-v5", ImVec2(0.0f, 128.0f), ImGuiChildFlags_Borders);
    drawStatusPill("VISUAL FOUNDATION", kCyan);
    ImGui::TextWrapped("A estrutura visual está pronta sem simular regras de jogo. O módulo será conectado ao domínio compartilhado em sua fase oficial.");
    ImGui::TextDisabled("Módulo reservado — sem implementação de domínio.");
    ImGui::EndChild();
    ImGui::Spacing();

    ImGui::BeginChild("module-workspace-v5", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
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
        const float fit=std::min({1.0f,static_cast<float>(std::max(1,usable.w-32))/windowWidth,
            static_cast<float>(std::max(1,usable.h-64))/windowHeight});
        windowWidth=static_cast<int>(windowWidth*fit);
        windowHeight=static_cast<int>(windowHeight*fit);
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
    styleNativeTitleBar(window);

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
    PresentationArt art(renderer);
    presentation=&art;
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Fixed shell geometry must not inherit stale panel scroll/layout caches.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    loadStudioFonts();
    std::cout << "Fantasy Studio V5 renderer=" << SDL_GetRendererName(renderer)
              << " ui_font=" << uiFont->GetDebugName()
              << " brand_font=" << brandFont->GetDebugName() << '\n';
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
        const TopAction topAction = drawTopbar(project, document, state, layout);
        drawSidebar(state, layout);
        executeTopAction(topAction, document, state, project.mainMapPath);

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) executeTopAction(TopAction::Save, document, state, project.mainMapPath);
        if (io.KeyCtrl && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Z, false)) executeTopAction(TopAction::Undo, document, state, project.mainMapPath);
        if (io.KeyCtrl && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Y, false)) executeTopAction(TopAction::Redo, document, state, project.mainMapPath);

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
    art.emblem.reset(); art.emblemSmall.reset(); art.hero.reset(); presentation=nullptr;
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
