#include "UI/StudioTheme.hpp"

#include "imgui.h"

namespace fantasy::studio::ui {
namespace {

ImVec4 rgb(int r, int g, int b, int a = 255) {
    return ImVec4(
        static_cast<float>(r) / 255.0f,
        static_cast<float>(g) / 255.0f,
        static_cast<float>(b) / 255.0f,
        static_cast<float>(a) / 255.0f);
}

} // namespace

void applyFantasyStudioTheme(float dpiScale) {
    ImGuiStyle& style = ImGui::GetStyle();

    // The approved styleboard is compact and tool-oriented. Keep corners subtle so
    // the Studio feels like a professional editor instead of a rounded dashboard.
    style.WindowRounding = 2.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;

    style.WindowPadding = ImVec2(10.0f, 8.0f);
    style.FramePadding = ImVec2(9.0f, 5.0f);
    style.ItemSpacing = ImVec2(7.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 9.0f;
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    // Styleboard tokens:
    //   background  #081220
    //   surface     #111827
    //   panel       #1E293B
    //   primary     #0EA5E9
    //   cyan        #22D3EE
    //   active      #60A5FA
    //   text        #E2E8F0
    //   secondary   #94A3B8
    auto& colors = style.Colors;
    colors[ImGuiCol_Text] = rgb(226, 232, 240);
    colors[ImGuiCol_TextDisabled] = rgb(148, 163, 184);

    // Deep navy hierarchy. The top-level canvas is the darkest layer, then
    // child/panel surfaces step upward slightly to preserve separation without
    // relying on expensive shadows or blur.
    colors[ImGuiCol_WindowBg] = rgb(8, 18, 32);
    colors[ImGuiCol_ChildBg] = rgb(10, 24, 41);
    colors[ImGuiCol_PopupBg] = rgb(9, 22, 38, 252);
    colors[ImGuiCol_Border] = rgb(14, 165, 233, 105);
    colors[ImGuiCol_BorderShadow] = rgb(0, 0, 0, 0);

    colors[ImGuiCol_FrameBg] = rgb(11, 29, 49);
    colors[ImGuiCol_FrameBgHovered] = rgb(16, 43, 68);
    colors[ImGuiCol_FrameBgActive] = rgb(17, 56, 86);

    colors[ImGuiCol_TitleBg] = rgb(6, 16, 29);
    colors[ImGuiCol_TitleBgActive] = rgb(8, 24, 42);
    colors[ImGuiCol_TitleBgCollapsed] = rgb(6, 16, 29);
    colors[ImGuiCol_MenuBarBg] = rgb(7, 19, 33);

    colors[ImGuiCol_ScrollbarBg] = rgb(6, 15, 27);
    colors[ImGuiCol_ScrollbarGrab] = rgb(25, 50, 74);
    colors[ImGuiCol_ScrollbarGrabHovered] = rgb(14, 165, 233, 205);
    colors[ImGuiCol_ScrollbarGrabActive] = rgb(34, 211, 238, 230);

    colors[ImGuiCol_CheckMark] = rgb(34, 211, 238);
    colors[ImGuiCol_SliderGrab] = rgb(14, 165, 233);
    colors[ImGuiCol_SliderGrabActive] = rgb(34, 211, 238);

    // Normal controls are restrained; cyan is reserved for hover/selection so
    // the active hierarchy matches the approved mockup.
    colors[ImGuiCol_Button] = rgb(14, 48, 73, 235);
    colors[ImGuiCol_ButtonHovered] = rgb(10, 126, 186, 245);
    colors[ImGuiCol_ButtonActive] = rgb(14, 165, 233, 255);

    colors[ImGuiCol_Header] = rgb(8, 70, 106, 215);
    colors[ImGuiCol_HeaderHovered] = rgb(10, 126, 186, 235);
    colors[ImGuiCol_HeaderActive] = rgb(14, 165, 233, 245);

    colors[ImGuiCol_Separator] = rgb(24, 50, 72);
    colors[ImGuiCol_SeparatorHovered] = rgb(14, 165, 233);
    colors[ImGuiCol_SeparatorActive] = rgb(34, 211, 238);

    colors[ImGuiCol_ResizeGrip] = rgb(14, 165, 233, 55);
    colors[ImGuiCol_ResizeGripHovered] = rgb(96, 165, 250, 175);
    colors[ImGuiCol_ResizeGripActive] = rgb(34, 211, 238, 215);

    colors[ImGuiCol_Tab] = rgb(11, 29, 49);
    colors[ImGuiCol_TabHovered] = rgb(10, 126, 186, 220);
    colors[ImGuiCol_TabSelected] = rgb(8, 112, 166, 235);
    colors[ImGuiCol_TabSelectedOverline] = rgb(34, 211, 238);
    colors[ImGuiCol_TabDimmed] = rgb(8, 21, 36);
    colors[ImGuiCol_TabDimmedSelected] = rgb(16, 50, 75);
    colors[ImGuiCol_TabDimmedSelectedOverline] = rgb(14, 165, 233);

    colors[ImGuiCol_PlotLines] = rgb(148, 163, 184);
    colors[ImGuiCol_PlotLinesHovered] = rgb(34, 211, 238);
    colors[ImGuiCol_PlotHistogram] = rgb(14, 165, 233);
    colors[ImGuiCol_PlotHistogramHovered] = rgb(34, 211, 238);

    colors[ImGuiCol_TableHeaderBg] = rgb(12, 31, 52);
    colors[ImGuiCol_TableBorderStrong] = rgb(24, 50, 72);
    colors[ImGuiCol_TableBorderLight] = rgb(24, 50, 72, 170);
    colors[ImGuiCol_TableRowBg] = rgb(8, 18, 32, 0);
    colors[ImGuiCol_TableRowBgAlt] = rgb(30, 41, 59, 38);

    colors[ImGuiCol_TextSelectedBg] = rgb(14, 165, 233, 95);
    colors[ImGuiCol_DragDropTarget] = rgb(34, 211, 238);
    colors[ImGuiCol_NavCursor] = rgb(34, 211, 238);
    colors[ImGuiCol_ModalWindowDimBg] = rgb(0, 0, 0, 175);

    if (dpiScale > 0.0f && dpiScale != 1.0f) {
        style.ScaleAllSizes(dpiScale);
    }
}

} // namespace fantasy::studio::ui
