#pragma once

#include <string>

// Dark theme modelled on Blender's default UI theme (colours approximated by eye)
void LsApplyBlenderTheme();

// Loads the UI font, falling back to ImGui's built-in font when the file is missing
void LsLoadUiFont(const std::string& path, float size);
