#include <iostream>
#include <fstream>
#include <imgui.h>
#include <LsUiTheme.h>

// 0xRRGGBB to ImVec4
static ImVec4 Hex(unsigned int rgb, float alpha = 1.0f) {
  return ImVec4(((rgb >> 16) & 0xFF) / 255.0f,
                ((rgb >> 8) & 0xFF) / 255.0f,
                (rgb & 0xFF) / 255.0f,
                alpha);
}

void LsApplyBlenderTheme() {
  ImGuiStyle& style = ImGui::GetStyle();
  style = ImGuiStyle();

  // Shape: flush panels, softly rounded widgets
  style.WindowRounding = 0.0f;
  style.ChildRounding = 0.0f;
  style.PopupRounding = 4.0f;
  style.FrameRounding = 4.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;
  style.ScrollbarRounding = 8.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;

  // Spacing: compact, like Blender's property editors
  style.WindowPadding = ImVec2(8.0f, 8.0f);
  style.FramePadding = ImVec2(8.0f, 3.0f);
  style.ItemSpacing = ImVec2(4.0f, 4.0f);
  style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
  style.IndentSpacing = 12.0f;
  style.ScrollbarSize = 10.0f;
  style.GrabMinSize = 8.0f;
  style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
  style.WindowMenuButtonPosition = ImGuiDir_None;

  const ImVec4 background  = Hex(0x303030); // editor region
  const ImVec4 header      = Hex(0x3D3D3D); // panel headers, tabs
  const ImVec4 widget      = Hex(0x545454); // buttons
  const ImVec4 widgetHover = Hex(0x656565);
  const ImVec4 field       = Hex(0x1D1D1D); // text and number fields
  const ImVec4 fieldHover  = Hex(0x232323);
  const ImVec4 accent      = Hex(0x4772B3); // selection, slider fill
  const ImVec4 accentHover = Hex(0x5680C2);
  const ImVec4 outline     = Hex(0x191919);
  const ImVec4 text        = Hex(0xE6E6E6);

  ImVec4* c = style.Colors;
  c[ImGuiCol_Text]                  = text;
  c[ImGuiCol_TextDisabled]          = Hex(0x8C8C8C);
  c[ImGuiCol_WindowBg]              = background;
  c[ImGuiCol_ChildBg]               = background;
  c[ImGuiCol_PopupBg]               = Hex(0x181818, 0.96f);
  c[ImGuiCol_Border]                = outline;
  c[ImGuiCol_BorderShadow]          = Hex(0x000000, 0.0f);
  c[ImGuiCol_FrameBg]               = field;
  c[ImGuiCol_FrameBgHovered]        = fieldHover;
  c[ImGuiCol_FrameBgActive]         = Hex(0x181818);
  c[ImGuiCol_TitleBg]               = header;
  c[ImGuiCol_TitleBgActive]         = header;
  c[ImGuiCol_TitleBgCollapsed]      = header;
  c[ImGuiCol_MenuBarBg]             = Hex(0x2B2B2B);
  c[ImGuiCol_ScrollbarBg]           = Hex(0x000000, 0.0f);
  c[ImGuiCol_ScrollbarGrab]         = Hex(0x5E5E5E);
  c[ImGuiCol_ScrollbarGrabHovered]  = Hex(0x707070);
  c[ImGuiCol_ScrollbarGrabActive]   = Hex(0x808080);
  c[ImGuiCol_CheckMark]             = Hex(0xFFFFFF);
  c[ImGuiCol_SliderGrab]            = accent;
  c[ImGuiCol_SliderGrabActive]      = accentHover;
  c[ImGuiCol_Button]                = widget;
  c[ImGuiCol_ButtonHovered]         = widgetHover;
  c[ImGuiCol_ButtonActive]          = accent;
  c[ImGuiCol_Header]                = header;
  c[ImGuiCol_HeaderHovered]         = Hex(0x474747);
  c[ImGuiCol_HeaderActive]          = accent;
  c[ImGuiCol_Separator]             = outline;
  c[ImGuiCol_SeparatorHovered]      = accent;
  c[ImGuiCol_SeparatorActive]       = accentHover;
  c[ImGuiCol_ResizeGrip]            = Hex(0x000000, 0.0f);
  c[ImGuiCol_ResizeGripHovered]     = accent;
  c[ImGuiCol_ResizeGripActive]      = accentHover;
  c[ImGuiCol_InputTextCursor]       = text;
  c[ImGuiCol_Tab]                   = Hex(0x232323);
  c[ImGuiCol_TabHovered]            = Hex(0x474747);
  c[ImGuiCol_TabSelected]           = header;
  c[ImGuiCol_TabSelectedOverline]   = accent;
  c[ImGuiCol_TabDimmed]             = Hex(0x232323);
  c[ImGuiCol_TabDimmedSelected]     = header;
  c[ImGuiCol_TabDimmedSelectedOverline] = Hex(0x000000, 0.0f);
  c[ImGuiCol_DockingPreview]        = Hex(0x4772B3, 0.5f);
  c[ImGuiCol_DockingEmptyBg]        = Hex(0x1D1D1D);
  c[ImGuiCol_PlotLines]             = accent;
  c[ImGuiCol_PlotLinesHovered]      = accentHover;
  c[ImGuiCol_PlotHistogram]         = accent;
  c[ImGuiCol_PlotHistogramHovered]  = accentHover;
  c[ImGuiCol_TableHeaderBg]         = header;
  c[ImGuiCol_TableBorderStrong]     = outline;
  c[ImGuiCol_TableBorderLight]      = Hex(0x262626);
  c[ImGuiCol_TableRowBg]            = Hex(0x000000, 0.0f);
  c[ImGuiCol_TableRowBgAlt]         = Hex(0xFFFFFF, 0.03f);
  c[ImGuiCol_TextLink]              = accentHover;
  c[ImGuiCol_TextSelectedBg]        = Hex(0x4772B3, 0.6f);
  c[ImGuiCol_TreeLines]             = outline;
  c[ImGuiCol_DragDropTarget]        = accentHover;
  c[ImGuiCol_NavCursor]             = accentHover;
  c[ImGuiCol_NavWindowingHighlight] = Hex(0xFFFFFF, 0.7f);
  c[ImGuiCol_NavWindowingDimBg]     = Hex(0x000000, 0.4f);
  c[ImGuiCol_ModalWindowDimBg]      = Hex(0x000000, 0.4f);
}

void LsLoadUiFont(const std::string& path, float size) {
  // ImGui asserts on a missing file, so check first
  if ( !std::ifstream(path).good() || !ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), size) ) {
    std::cout << "Could not load UI font " << path << ", using the built-in font" << std::endl;
  }
}
