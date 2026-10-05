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

    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    style.WindowPadding = ImVec2(12.0f, 10.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 7.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.ScrollbarSize = 12.0f;

    auto& colors = style.Colors;
    colors[ImGuiCol_Text] = rgb(226, 232, 240);
    colors[ImGuiCol_TextDisabled] = rgb(148, 163, 184);
    colors[ImGuiCol_WindowBg] = rgb(8, 18, 32);
    colors[ImGuiCol_ChildBg] = rgb(17, 24, 39);
    colors[ImGuiCol_PopupBg] = rgb(17, 24, 39, 250);
    colors[ImGuiCol_Border] = rgb(14, 165, 233, 115);
    colors[ImGuiCol_BorderShadow] = rgb(0, 0, 0, 0);
    colors[ImGuiCol_FrameBg] = rgb(17, 24, 39);
    colors[ImGuiCol_FrameBgHovered] = rgb(30, 41, 59);
    colors[ImGuiCol_FrameBgActive] = rgb(30, 41, 59);
    colors[ImGuiCol_TitleBg] = rgb(8, 18, 32);
    colors[ImGuiCol_TitleBgActive] = rgb(8, 18, 32);
    colors[ImGuiCol_TitleBgCollapsed] = rgb(8, 18, 32);
    colors[ImGuiCol_MenuBarBg] = rgb(8, 18, 32);
    colors[ImGuiCol_ScrollbarBg] = rgb(8, 18, 32);
    colors[ImGuiCol_ScrollbarGrab] = rgb(30, 41, 59);
    colors[ImGuiCol_ScrollbarGrabHovered] = rgb(59, 130, 246);
    colors[ImGuiCol_ScrollbarGrabActive] = rgb(14, 165, 233);
    colors[ImGuiCol_CheckMark] = rgb(34, 211, 238);
    colors[ImGuiCol_SliderGrab] = rgb(14, 165, 233);
    colors[ImGuiCol_SliderGrabActive] = rgb(34, 211, 238);
    colors[ImGuiCol_Button] = rgb(14, 165, 233, 205);
    colors[ImGuiCol_ButtonHovered] = rgb(59, 130, 246);
    colors[ImGuiCol_ButtonActive] = rgb(2, 132, 199);
    colors[ImGuiCol_Header] = rgb(14, 165, 233, 145);
    colors[ImGuiCol_HeaderHovered] = rgb(59, 130, 246, 190);
    colors[ImGuiCol_HeaderActive] = rgb(14, 165, 233, 220);
    colors[ImGuiCol_Separator] = rgb(30, 41, 59);
    colors[ImGuiCol_SeparatorHovered] = rgb(14, 165, 233);
    colors[ImGuiCol_SeparatorActive] = rgb(34, 211, 238);
    colors[ImGuiCol_ResizeGrip] = rgb(14, 165, 233, 65);
    colors[ImGuiCol_ResizeGripHovered] = rgb(59, 130, 246, 170);
    colors[ImGuiCol_ResizeGripActive] = rgb(34, 211, 238, 210);
    colors[ImGuiCol_Tab] = rgb(17, 24, 39);
    colors[ImGuiCol_TabHovered] = rgb(59, 130, 246, 190);
    colors[ImGuiCol_TabSelected] = rgb(14, 165, 233, 180);
    colors[ImGuiCol_TabSelectedOverline] = rgb(34, 211, 238);
    colors[ImGuiCol_PlotLines] = rgb(148, 163, 184);
    colors[ImGuiCol_PlotLinesHovered] = rgb(34, 211, 238);
    colors[ImGuiCol_PlotHistogram] = rgb(14, 165, 233);
    colors[ImGuiCol_PlotHistogramHovered] = rgb(34, 211, 238);
    colors[ImGuiCol_TableHeaderBg] = rgb(17, 24, 39);
    colors[ImGuiCol_TableBorderStrong] = rgb(30, 41, 59);
    colors[ImGuiCol_TableBorderLight] = rgb(30, 41, 59, 170);
    colors[ImGuiCol_TableRowBg] = rgb(8, 18, 32, 0);
    colors[ImGuiCol_TableRowBgAlt] = rgb(30, 41, 59, 55);
    colors[ImGuiCol_TextSelectedBg] = rgb(14, 165, 233, 100);
    colors[ImGuiCol_DragDropTarget] = rgb(34, 211, 238);
    colors[ImGuiCol_NavCursor] = rgb(34, 211, 238);
    colors[ImGuiCol_ModalWindowDimBg] = rgb(0, 0, 0, 150);

    if (dpiScale > 0.0f && dpiScale != 1.0f) {
        style.ScaleAllSizes(dpiScale);
    }
}

} // namespace fantasy::studio::ui
