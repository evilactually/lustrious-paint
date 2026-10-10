#include <LsLightingPanel.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <imgui.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <LsCanvasPainter.h>

// Colour pickers show sRGB values, the renderer works in linear ones
static float SrgbToLinear(float c) {
  return c <= 0.04045f ? c/12.92f : std::pow((c + 0.055f)/1.055f, 2.4f);
}

static float LinearToSrgb(float c) {
  c = glm::clamp(c, 0.0f, 1.0f);
  return c <= 0.0031308f ? c*12.92f : 1.055f*std::pow(c, 1.0f/2.4f) - 0.055f;
}

// A colour scaled so that its brightest channel is 1, for showing a light's hue
static ImU32 HueColor(glm::vec3 color, float alpha = 1.0f) {
  float peak = std::max(std::max(color.r, color.g), std::max(color.b, 1.0e-6f));
  glm::vec3 c = color/peak;
  return ImGui::ColorConvertFloat4ToU32(ImVec4(LinearToSrgb(c.r), LinearToSrgb(c.g), LinearToSrgb(c.b), alpha));
}

LsLightingPanel::LsLightingPanel(std::shared_ptr<LsCanvas> canvas, std::shared_ptr<LsCanvasPainter> painter) :
  canvas(canvas), painter(painter) {}

void LsLightingPanel::Draw(const LsCamera* camera) {
  // The selected light or occluder can have been removed
  if (selected >= 0 && !canvas->GetLocalLight(static_cast<uint32_t>(selected))) {
    selected = -1;
  }
  if (selectedOccluder >= 0 && !canvas->GetOccluder(static_cast<uint32_t>(selectedOccluder))) {
    selectedOccluder = -1;
  }
  // The current rig can have been removed
  if (!canvas->GetRig(currentRig)) {
    currentRig = 0;
  }
  DrawSphere(camera);
  DrawNormal();
  DrawRigs();
  DrawLightList();
  DrawSelectedLight();
  DrawOccluderList();
  DrawSelectedOccluder();
  painter->SetRig(currentRig);
}

bool LsLightingPanel::InRig(uint32_t slot, bool occluder) const {
  const LsLightRig* rig = canvas->GetRig(currentRig);
  return rig && ((occluder ? rig->occluders : rig->lights) & (1u << slot)) != 0;
}

void LsLightingPanel::SetInRig(uint32_t slot, bool occluder, bool in) {
  const LsLightRig* current = canvas->GetRig(currentRig);
  if (!current) {
    return;
  }
  LsLightRig rig = *current;
  uint32_t& bits = occluder ? rig.occluders : rig.lights;
  bits = in ? bits | (1u << slot) : bits & ~(1u << slot);
  canvas->SetRig(currentRig, rig);
}

// A rig with a colour of its own, a copy of another one or empty, made current
int LsLightingPanel::NewRig(const LsLightRig* copy) {
  LsLightRig rig = copy ? *copy : LsLightRig();
  char name[64];
  if (copy) {
    snprintf(name, sizeof(name), "%s copy", copy->name.c_str());
  }
  else {
    snprintf(name, sizeof(name), "Rig %d", nextRigNumber);
  }
  rig.name = name;
  // Hues a golden angle apart, so neighbouring rigs differ
  float hue = std::fmod(0.08f + 0.381966f*static_cast<float>(nextRigNumber), 1.0f);
  ImGui::ColorConvertHSVtoRGB(hue, 0.65f, 0.95f, rig.color.r, rig.color.g, rig.color.b);
  int slot = canvas->AddRig(rig);
  if (slot >= 0) {
    currentRig = static_cast<uint32_t>(slot);
    nextRigNumber++;
  }
  return slot;
}

// The rigs, each with its colour and how much of the painted canvas it covers.
// The current one is what light strokes paint.
void LsLightingPanel::DrawRigs() {
  ImGui::SeparatorText("Light rigs");
  if (ImGui::Button("New##Rig")) {
    NewRig(nullptr);
  }
  ImGui::SetItemTooltip("An empty rig, check the lights and occluders it has below");
  ImGui::SameLine();
  ImGui::BeginDisabled(currentRig == 0);
  if (ImGui::Button("Duplicate##Rig")) {
    NewRig(canvas->GetRig(currentRig));
  }
  ImGui::SetItemTooltip("A new rig with the same lights and occluders");
  ImGui::SameLine();
  if (ImGui::Button("Delete##Rig")) {
    canvas->RemoveRig(currentRig);
    currentRig = 0;
  }
  ImGui::SetItemTooltip("Removes the rig, the pixels it is painted on keep only the scene's lights");
  ImGui::EndDisabled();

  uint32_t painted = std::max(canvas->GetPaintedPixelCount(), 1u);
  for (uint32_t slot = 1; slot < kLsMaxRigs; slot++) {
    const LsLightRig* rig = canvas->GetRig(slot);
    if (!rig) {
      continue;
    }
    ImGui::PushID(static_cast<int>(slot));
    LsLightRig edited = *rig;
    if (ImGui::ColorEdit3("##Color", &edited.color.r, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
      canvas->SetRig(slot, edited);
    }
    ImGui::SetItemTooltip("Shown where it is painted in the Rigs display");
    ImGui::SameLine();
    // Its share of the pixels with a position
    uint32_t count = canvas->GetRigPixelCount(slot);
    char coverage[32];
    if (count == 0) {
      snprintf(coverage, sizeof(coverage), "unpainted");
    }
    else {
      float percent = 100.0f*static_cast<float>(count)/static_cast<float>(painted);
      snprintf(coverage, sizeof(coverage), percent < 1.0f ? "<1%%" : "%.0f%%", percent);
    }
    float coverageWidth = ImGui::CalcTextSize(coverage).x;
    float right = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    if (ImGui::Selectable(rig->name.c_str(), slot == currentRig, 0,
                          ImVec2(ImGui::GetContentRegionAvail().x - coverageWidth - ImGui::GetStyle().ItemSpacing.x, 0.0f))) {
      currentRig = slot;
    }
    ImGui::SameLine(right - coverageWidth);
    ImGui::TextDisabled("%s", coverage);
    ImGui::PopID();
  }
  if (const LsLightRig* rig = canvas->GetRig(currentRig)) {
    char name[64];
    snprintf(name, sizeof(name), "%s", rig->name.c_str());
    if (ImGui::InputText("Name##Rig", name, sizeof(name))) {
      LsLightRig edited = *rig;
      edited.name = name;
      canvas->SetRig(currentRig, edited);
    }
  }
  ImGui::PushTextWrapPos(0.0f);
  if (currentRig == 0) {
    ImGui::TextDisabled("No rig selected, make one with New. Light strokes (L) then paint it.");
  }
  else {
    ImGui::TextDisabled("Light strokes (L) paint the selected rig, its lights and occluders are checked below. "
                        "Shift paints no rig, Alt+click picks the rig under the brush.");
  }
  ImGui::PopTextWrapPos();
}

// The normal of the patch as a tilt away from facing the camera and the way it
// leans, and which strokes paint it
void LsLightingPanel::DrawNormal() {
  ImGui::SeparatorText("Normal");
  glm::vec3 normal = painter->GetNormal();
  float tilt = glm::degrees(std::acos(glm::clamp(normal.z, -1.0f, 1.0f)));
  float turn = glm::degrees(std::atan2(normal.y, normal.x));
  if (turn < 0.0f) {
    turn += 360.0f;
  }
  bool changed = ImGui::SliderFloat("Tilt", &tilt, 0.0f, 85.0f, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
  ImGui::SetItemTooltip("How far the surface leans away from facing along the camera's view axis");
  // Which way it leans means nothing while it faces the camera
  ImGui::BeginDisabled(tilt < 0.5f);
  changed |= ImGui::SliderFloat("Turn", &turn, 0.0f, 360.0f, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
  ImGui::EndDisabled();
  ImGui::SetItemTooltip("Which way the surface leans, as seen from the camera:\n0 right, 90 up, 180 left, 270 down");
  if (changed) {
    float t = glm::radians(tilt), r = glm::radians(turn);
    painter->SetNormal(glm::vec3(std::sin(t)*std::cos(r), std::sin(t)*std::sin(r), std::cos(t)));
  }
  if (ImGui::Button("Face camera")) {
    painter->SetNormal(glm::vec3(0.0f, 0.0f, 1.0f));
  }
  ImGui::SetItemTooltip("Back to a surface facing the camera");
  ImGui::SameLine();
  bool armed = painter->IsPickArmed();
  if (armed) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
  }
  if (ImGui::Button("Pick")) {
    painter->ArmPick();
  }
  if (armed) {
    ImGui::PopStyleColor();
  }
  ImGui::SetItemTooltip("Click the canvas to take the normal under the brush, averaged over it.\n"
                        "Alt+click or Alt+drag on the canvas does the same at any time.");

  bool rigNormal = painter->GetRigNormal();
  if (ImGui::Checkbox("Light brush paints it", &rigNormal)) {
    painter->SetRigNormal(rigNormal);
  }
  ImGui::SetItemTooltip("Light strokes paint this normal with the rig, blended like a normal stroke");
  bool fixedNormal = painter->GetFixedNormal();
  if (ImGui::Checkbox("Normals brush paints it", &fixedNormal)) {
    painter->SetFixedNormal(fixedNormal);
  }
  ImGui::SetItemTooltip("Normal strokes paint this normal instead of the construction geometry's");
}

glm::vec3 LsLightingPanel::DirectionToCamera(const LsCamera* camera) const {
  glm::vec2 point;
  if (!camera || !painter->GetPickPoint(point)) {
    return glm::vec3(0.0f, 0.0f, 1.0f);
  }
  // The ray through the pixel as canvas.slang's pixelRay has it, in view space
  glm::vec2 size(canvas->GetSize());
  glm::vec2 ndc = glm::clamp(point/size, 0.0f, 1.0f)*2.0f - 1.0f;
  ndc.y = -ndc.y;
  float tanY = camera->GetTanHalfFovY();
  float tanX = tanY*camera->GetAspect();
  glm::vec2 shift = camera->GetLensShift();
  glm::vec3 ray(ndc.x*tanX + shift.x, ndc.y*tanY + shift.y, -1.0f);
  return -glm::normalize(ray);
}

void LsLightingPanel::AddLight(glm::vec3 direction) {
  LsLocalLight light;
  char name[32];
  snprintf(name, sizeof(name), "Light %d", nextLightNumber);
  light.name = name;
  light.direction = direction;
  int slot = canvas->AddLocalLight(light);
  if (slot >= 0) {
    SelectLight(slot);
    nextLightNumber++;
    // Into the current rig, a first rig for a first light
    if (currentRig == 0 && canvas->GetLocalLightSlots() == (1u << slot)) {
      NewRig(nullptr);
    }
    SetInRig(static_cast<uint32_t>(slot), false, true);
  }
}

void LsLightingPanel::AddOccluder(glm::vec3 direction) {
  LsOccluder occluder;
  char name[32];
  snprintf(name, sizeof(name), "Occluder %d", nextOccluderNumber);
  occluder.name = name;
  occluder.direction = direction;
  int slot = canvas->AddOccluder(occluder);
  if (slot >= 0) {
    SelectOccluder(slot);
    nextOccluderNumber++;
    SetInRig(static_cast<uint32_t>(slot), true, true);
  }
}

void LsLightingPanel::DrawSphere(const LsCamera* camera) {
  float size = std::min(ImGui::GetContentRegionAvail().x, 280.0f*ImGui::GetFontSize()/16.0f);
  ImVec2 origin = ImGui::GetCursorScreenPos();
  ImGui::InvisibleButton("##Sphere", ImVec2(size, size), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
  bool hovered = ImGui::IsItemHovered();
  bool active = ImGui::IsItemActive();
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  ImVec2 center(origin.x + size*0.5f, origin.y + size*0.5f);
  float radius = size*0.33f;

  // The widget's own view of the canvas camera's view space, turned so that the
  // patch and the hemisphere read as 3D. Widget space has x right, y up and z
  // towards the viewer, like view space.
  glm::mat3 yaw = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(viewYaw), glm::vec3(0.0f, 1.0f, 0.0f)));
  glm::mat3 pitch = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(viewPitch), glm::vec3(1.0f, 0.0f, 0.0f)));
  glm::mat3 toWidget = pitch*yaw;
  auto toScreen = [&](glm::vec3 v) {
    glm::vec3 w = toWidget*v;
    return ImVec2(center.x + w.x*radius, center.y - w.y*radius);
  };
  auto depth = [&](glm::vec3 v) { return (toWidget*v).z; };
  // The point on the side of the sphere facing the viewer under the mouse, on
  // its rim outside it, in view space
  auto fromScreen = [&](ImVec2 p) {
    glm::vec2 v((p.x - center.x)/radius, (center.y - p.y)/radius);
    float length = glm::length(v);
    if (length > 1.0f) {
      v /= length;
    }
    glm::vec3 w(v, std::sqrt(std::max(0.0f, 1.0f - glm::dot(v, v))));
    return glm::normalize(glm::transpose(toWidget)*w);
  };
  // Lines are fainter behind the centre of the sphere
  auto lineColor = [&](glm::vec3 v, ImU32 front, ImU32 back) { return depth(v) >= 0.0f ? front : back; };

  // Tangent frame of the patch, the surface with the brush normal
  glm::vec3 normal = painter->GetNormal();
  glm::vec3 tangent = glm::normalize(glm::cross(std::abs(normal.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0), normal));
  glm::vec3 bitangent = glm::cross(normal, tangent);
  auto hemisphere = [&](float theta, float phi) {
    return normal*std::cos(theta) + (tangent*std::cos(phi) + bitangent*std::sin(phi))*std::sin(theta);
  };

  // The patch, a semi-transparent disk a little wider than the hemisphere,
  // with a cross through it and rings
  const float patchRadius = 1.3f;
  const int patchSegments = 64;
  ImVec2 rim[patchSegments];
  for (int i = 0; i < patchSegments; i++) {
    float phi = glm::two_pi<float>()*i/patchSegments;
    rim[i] = toScreen((tangent*std::cos(phi) + bitangent*std::sin(phi))*patchRadius);
  }
  drawList->AddConvexPolyFilled(rim, patchSegments, IM_COL32(140, 140, 150, 70));
  drawList->AddPolyline(rim, patchSegments, IM_COL32(190, 190, 200, 150), ImDrawFlags_Closed, 1.0f);
  drawList->AddLine(toScreen(-tangent*patchRadius), toScreen(tangent*patchRadius), IM_COL32(180, 180, 190, 60));
  drawList->AddLine(toScreen(-bitangent*patchRadius), toScreen(bitangent*patchRadius), IM_COL32(180, 180, 190, 60));
  for (float ring : { 0.5f, 1.0f }) {
    for (int i = 0; i < patchSegments; i++) {
      float a = glm::two_pi<float>()*i/patchSegments;
      float b = glm::two_pi<float>()*(i + 1)/patchSegments;
      drawList->AddLine(toScreen((tangent*std::cos(a) + bitangent*std::sin(a))*ring),
                        toScreen((tangent*std::cos(b) + bitangent*std::sin(b))*ring), IM_COL32(180, 180, 190, 40));
    }
  }

  // The hemisphere over the patch, a faint fill under latitude and longitude lines
  const float halfPi = glm::half_pi<float>();
  const int rings = 8;
  const int segments = 32;
  ImVec2 white = ImGui::GetFontTexUvWhitePixel();
  ImU32 fill = IM_COL32(200, 210, 230, 18);
  unsigned int base = drawList->_VtxCurrentIdx;
  drawList->PrimReserve(segments*3 + (rings - 1)*segments*6, 1 + rings*segments);
  drawList->PrimWriteVtx(toScreen(normal), white, fill);
  for (int ring = 1; ring <= rings; ring++) {
    for (int segment = 0; segment < segments; segment++) {
      drawList->PrimWriteVtx(toScreen(hemisphere(halfPi*ring/rings, glm::two_pi<float>()*segment/segments)), white, fill);
    }
  }
  auto vertex = [&](int ring, int segment) {
    return static_cast<ImDrawIdx>(base + 1 + (ring - 1)*segments + segment % segments);
  };
  for (int segment = 0; segment < segments; segment++) {
    drawList->PrimWriteIdx(static_cast<ImDrawIdx>(base));
    drawList->PrimWriteIdx(vertex(1, segment));
    drawList->PrimWriteIdx(vertex(1, segment + 1));
  }
  for (int ring = 1; ring < rings; ring++) {
    for (int segment = 0; segment < segments; segment++) {
      ImDrawIdx a = vertex(ring, segment), b = vertex(ring, segment + 1);
      ImDrawIdx c = vertex(ring + 1, segment), d = vertex(ring + 1, segment + 1);
      drawList->PrimWriteIdx(a); drawList->PrimWriteIdx(c); drawList->PrimWriteIdx(b);
      drawList->PrimWriteIdx(b); drawList->PrimWriteIdx(c); drawList->PrimWriteIdx(d);
    }
  }
  const ImU32 gridFront = IM_COL32(210, 215, 230, 150);
  const ImU32 gridBack = IM_COL32(210, 215, 230, 45);
  const int arcSteps = 48;
  for (int latitude = 1; latitude <= 3; latitude++) {
    float theta = halfPi*latitude/3.0f;
    for (int i = 0; i < arcSteps; i++) {
      glm::vec3 a = hemisphere(theta, glm::two_pi<float>()*i/arcSteps);
      glm::vec3 b = hemisphere(theta, glm::two_pi<float>()*(i + 1)/arcSteps);
      drawList->AddLine(toScreen(a), toScreen(b), lineColor(a + b, gridFront, gridBack), latitude == 3 ? 1.5f : 1.0f);
    }
  }
  for (int longitude = 0; longitude < 8; longitude++) {
    float phi = glm::two_pi<float>()*longitude/8.0f;
    for (int i = 0; i < arcSteps/4; i++) {
      glm::vec3 a = hemisphere(halfPi*i/(arcSteps/4), phi);
      glm::vec3 b = hemisphere(halfPi*(i + 1)/(arcSteps/4), phi);
      drawList->AddLine(toScreen(a), toScreen(b), lineColor(a + b, gridFront, gridBack), 1.0f);
    }
  }

  // Which way the canvas camera is from the point picked last, and the
  // world axes as it sees them, in the lower left corner
  ImU32 cameraColor = IM_COL32(200, 200, 200, 160);
  glm::vec3 toCamera = DirectionToCamera(camera);
  drawList->AddLine(toScreen(toCamera), toScreen(toCamera*1.3f), cameraColor, 1.0f);
  ImVec2 cameraEnd = toScreen(toCamera*1.42f);
  drawList->AddText(ImVec2(cameraEnd.x - ImGui::CalcTextSize("Camera").x*0.5f, cameraEnd.y - ImGui::GetFontSize()*0.5f),
                    cameraColor, "Camera");
  if (camera) {
    ImVec2 frameCenter(origin.x + size*0.1f, origin.y + size*0.9f);
    float frameLength = size*0.07f;
    const struct { glm::vec3 axis; ImU32 color; const char* label; } axes[3] = {
      { glm::vec3(1, 0, 0), IM_COL32(220, 80, 80, 255), "X" },
      { glm::vec3(0, 1, 0), IM_COL32(90, 200, 90, 255), "Y" },
      { glm::vec3(0, 0, 1), IM_COL32(90, 130, 240, 255), "Z" },
    };
    for (const auto& axis : axes) {
      glm::vec3 v(glm::dot(axis.axis, camera->GetRight()), glm::dot(axis.axis, camera->GetUp()),
                  -glm::dot(axis.axis, camera->GetForward()));
      glm::vec3 w = toWidget*v;
      ImVec2 end(frameCenter.x + w.x*frameLength, frameCenter.y - w.y*frameLength);
      drawList->AddLine(frameCenter, end, axis.color, 2.0f);
      drawList->AddText(ImVec2(end.x + 2.0f, end.y - ImGui::GetFontSize()*0.5f), axis.color, axis.label);
    }
  }

  // The normal as a labelled arrow from the centre to a square handle, bright
  // while a brush paints it, outlined while the mouse is over it
  float handle = std::max(5.0f, size*0.022f);
  ImVec2 normalEnd = toScreen(normal);
  ImVec2 mousePosition = ImGui::GetIO().MousePos;
  bool overNormal = hovered && std::hypot(normalEnd.x - mousePosition.x, normalEnd.y - mousePosition.y) <= handle + 4.0f;
  bool painted = painter->GetFixedNormal() || painter->GetRigNormal();
  ImU32 normalColor = painted ? IM_COL32(80, 220, 230, 255) : IM_COL32(80, 220, 230, 150);
  drawList->AddLine(center, normalEnd, normalColor, 2.0f);
  float normalHandle = overNormal || drag == Drag::Normal ? handle + 2.0f : handle;
  drawList->AddRectFilled(ImVec2(normalEnd.x - normalHandle, normalEnd.y - normalHandle),
                          ImVec2(normalEnd.x + normalHandle, normalEnd.y + normalHandle), normalColor);
  if (overNormal || drag == Drag::Normal) {
    drawList->AddRect(ImVec2(normalEnd.x - normalHandle - 2.0f, normalEnd.y - normalHandle - 2.0f),
                      ImVec2(normalEnd.x + normalHandle + 2.0f, normalEnd.y + normalHandle + 2.0f), IM_COL32(255, 255, 255, 255));
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  }
  drawList->AddText(ImVec2(normalEnd.x + normalHandle + 4.0f, normalEnd.y - ImGui::GetFontSize()*0.5f), normalColor, "Normal");

  // Occluders as dark caps over the directions they shadow, faint when out of
  // the rig, a line where they start to fade, a diamond handle at the centre
  auto capPoint = [&](glm::vec3 axis, float angle, float phi) {
    glm::vec3 t = glm::normalize(glm::cross(std::abs(axis.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0), axis));
    glm::vec3 b = glm::cross(axis, t);
    return axis*std::cos(angle) + (t*std::cos(phi) + b*std::sin(phi))*std::sin(angle);
  };
  std::vector<ImVec2> occluderHandles(kLsMaxOccluders);
  for (uint32_t slot = 0; slot < kLsMaxOccluders; slot++) {
    const LsOccluder* occluder = canvas->GetOccluder(slot);
    if (!occluder) {
      continue;
    }
    const int capSegments = 48;
    float radius = glm::radians(glm::clamp(occluder->size, 0.5f, 90.0f));
    float inner = radius*(1.0f - glm::clamp(occluder->softness, 0.0f, 1.0f));
    ImVec2 rimPoints[capSegments];
    for (int i = 0; i < capSegments; i++) {
      rimPoints[i] = toScreen(capPoint(occluder->direction, radius, glm::two_pi<float>()*i/capSegments));
    }
    int alpha = static_cast<int>((InRig(slot, true) ? 150.0f : 60.0f)*glm::clamp(occluder->strength, 0.2f, 1.0f));
    drawList->AddConvexPolyFilled(rimPoints, capSegments, IM_COL32(10, 10, 18, alpha));
    bool isSelected = static_cast<int>(slot) == selectedOccluder;
    drawList->AddPolyline(rimPoints, capSegments, isSelected ? IM_COL32(255, 255, 255, 220) : IM_COL32(120, 120, 140, 200),
                          ImDrawFlags_Closed, isSelected ? 1.5f : 1.0f);
    if (inner < radius - 1.0e-3f) {
      for (int i = 0; i < capSegments; i += 2) {
        drawList->AddLine(toScreen(capPoint(occluder->direction, inner, glm::two_pi<float>()*i/capSegments)),
                          toScreen(capPoint(occluder->direction, inner, glm::two_pi<float>()*(i + 1)/capSegments)),
                          IM_COL32(120, 120, 140, 160));
      }
    }
    ImVec2 c = toScreen(occluder->direction);
    occluderHandles[slot] = c;
    float d = handle + 1.0f;
    drawList->AddQuadFilled(ImVec2(c.x, c.y - d), ImVec2(c.x + d, c.y), ImVec2(c.x, c.y + d), ImVec2(c.x - d, c.y),
                            IM_COL32(25, 25, 35, 255));
    drawList->AddQuad(ImVec2(c.x, c.y - d), ImVec2(c.x + d, c.y), ImVec2(c.x, c.y + d), ImVec2(c.x - d, c.y),
                      isSelected ? IM_COL32(255, 255, 255, 255) : IM_COL32(170, 170, 190, 255), 1.5f);
  }

  // Lights as dots on the sphere with a line to the centre, the light they shine
  // along. Faint when out of the rig or below the patch, where they can't light
  // it, hollow behind the sphere, the selected one ringed.
  for (uint32_t slot = 0; slot < kLsMaxLocalLights; slot++) {
    const LsLocalLight* light = canvas->GetLocalLight(slot);
    if (!light) {
      continue;
    }
    bool lighting = InRig(slot, false) && glm::dot(light->direction, normal) > 0.0f;
    ImVec2 p = toScreen(light->direction);
    ImU32 color = HueColor(light->color, lighting ? 1.0f : 0.35f);
    drawList->AddLine(center, p, HueColor(light->color, lighting ? 0.6f : 0.2f), 1.0f);
    if (depth(light->direction) >= 0.0f) {
      drawList->AddCircleFilled(p, handle, color);
      drawList->AddCircle(p, handle, IM_COL32(20, 20, 20, 255), 0, 1.0f);
    }
    else {
      drawList->AddCircle(p, handle, color, 0, 2.5f);
    }
    if (static_cast<int>(slot) == selected) {
      drawList->AddCircle(p, handle + 4.0f, IM_COL32(255, 255, 255, 255), 0, 1.5f);
    }
  }

  // The left button picks the light, occluder or normal handle under the mouse, a double
  // click elsewhere adds a light. The right button turns the view, a double
  // click with it puts the view back.
  ImVec2 mouse = ImGui::GetIO().MousePos;
  auto near = [&](ImVec2 p) { return std::hypot(p.x - mouse.x, p.y - mouse.y) <= handle + 4.0f; };
  if (ImGui::IsItemActivated()) {
    drag = Drag::None;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      drag = Drag::View;
    }
    else {
      // Nearest to the viewer first, so a light in front of the normal handle is picked
      float nearest = -1.0e30f;
      int light = -1, occluder = -1;
      for (uint32_t slot = 0; slot < kLsMaxLocalLights; slot++) {
        const LsLocalLight* candidate = canvas->GetLocalLight(slot);
        if (candidate && near(toScreen(candidate->direction)) && depth(candidate->direction) > nearest) {
          light = static_cast<int>(slot);
          drag = Drag::Light;
          nearest = depth(candidate->direction);
        }
      }
      for (uint32_t slot = 0; slot < kLsMaxOccluders; slot++) {
        const LsOccluder* candidate = canvas->GetOccluder(slot);
        if (candidate && near(occluderHandles[slot]) && depth(candidate->direction) > nearest) {
          occluder = static_cast<int>(slot);
          drag = Drag::Occluder;
          nearest = depth(candidate->direction);
        }
      }
      if (near(normalEnd) && depth(normal) > nearest) {
        drag = Drag::Normal;
      }
      else if (drag == Drag::Light) {
        SelectLight(light);
      }
      else if (drag == Drag::Occluder) {
        SelectOccluder(occluder);
      }
    }
  }
  if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right)) {
    viewYaw = kDefaultViewYaw;
    viewPitch = kDefaultViewPitch;
  }
  if (hovered && drag == Drag::None && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    AddLight(fromScreen(mouse));
  }
  if (active && drag == Drag::View) {
    ImVec2 delta = ImGui::GetIO().MouseDelta;
    viewYaw += delta.x*0.5f;
    viewPitch = glm::clamp(viewPitch + delta.y*0.5f, -89.0f, 89.0f);
  }
  else if (active && drag == Drag::Light && selected >= 0) {
    LsLocalLight light = *canvas->GetLocalLight(static_cast<uint32_t>(selected));
    glm::vec3 direction = fromScreen(mouse);
    if (direction != light.direction) {
      light.direction = direction;
      canvas->SetLocalLight(static_cast<uint32_t>(selected), light);
    }
  }
  else if (active && drag == Drag::Occluder && selectedOccluder >= 0) {
    LsOccluder occluder = *canvas->GetOccluder(static_cast<uint32_t>(selectedOccluder));
    glm::vec3 direction = fromScreen(mouse);
    if (direction != occluder.direction) {
      occluder.direction = direction;
      canvas->SetOccluder(static_cast<uint32_t>(selectedOccluder), occluder);
    }
  }
  else if (active && drag == Drag::Normal) {
    glm::vec3 direction = fromScreen(mouse);
    // The picked surface faces the camera, it is seen from there
    glm::vec3 toCamera = DirectionToCamera(camera);
    float facing = glm::dot(direction, toCamera);
    if (facing < 0.05f) {
      direction += toCamera*(0.05f - facing);
    }
    painter->SetNormal(glm::normalize(direction));
  }
  if (!active) {
    drag = Drag::None;
  }
  if (hovered && !active) {
    ImGui::SetItemTooltip("Drag a light, an occluder or the normal handle around the sphere,\n"
                          "double click to add a light. Right drag to turn the view,\n"
                          "double right click to put it back.");
  }
}

void LsLightingPanel::DrawLightList() {
  ImGui::SeparatorText("Lights");
  if (ImGui::Button("Add##Light")) {
    AddLight(glm::normalize(glm::vec3(-0.5f, 0.6f, 0.6f)));
  }
  ImGui::SameLine();
  ImGui::BeginDisabled(selected < 0);
  if (ImGui::Button("Delete##Light")) {
    canvas->RemoveLocalLight(static_cast<uint32_t>(selected));
    selected = -1;
  }
  ImGui::EndDisabled();
  ImGui::SetItemTooltip("Removes the light, also from where it is painted");

  for (uint32_t slot = 0; slot < kLsMaxLocalLights; slot++) {
    const LsLocalLight* light = canvas->GetLocalLight(slot);
    if (!light) {
      continue;
    }
    ImGui::PushID(static_cast<int>(slot));
    ImU32 hue = HueColor(light->color);
    ImGui::ColorButton("##Color", ImGui::ColorConvertU32ToFloat4(hue), ImGuiColorEditFlags_NoTooltip,
                       ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
    ImGui::SameLine();
    char label[96];
    // In the current rig, which changes every pixel the rig is painted on
    bool inRig = InRig(slot, false);
    ImGui::BeginDisabled(currentRig == 0);
    if (ImGui::Checkbox("##InRig", &inRig)) {
      SetInRig(slot, false, inRig);
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("In the selected rig, also where it is already painted");
    ImGui::SameLine();
    if (ImGui::Selectable(light->name.c_str(), static_cast<int>(slot) == selected)) {
      SelectLight(static_cast<int>(slot));
    }
    ImGui::PopID();
  }
  if (canvas->GetLocalLightSlots() == 0) {
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("No lights, add one or double click the sphere");
    ImGui::PopTextWrapPos();
  }
}

void LsLightingPanel::DrawSelectedLight() {
  if (selected < 0) {
    return;
  }
  LsLocalLight light = *canvas->GetLocalLight(static_cast<uint32_t>(selected));
  ImGui::SeparatorText(light.name.c_str());
  bool changed = false;
  char name[64];
  snprintf(name, sizeof(name), "%s", light.name.c_str());
  if (ImGui::InputText("Name", name, sizeof(name))) {
    light.name = name;
    changed = true;
  }
  float color[3] = { LinearToSrgb(light.color.r), LinearToSrgb(light.color.g), LinearToSrgb(light.color.b) };
  if (ImGui::ColorEdit3("Color", color)) {
    light.color = glm::vec3(SrgbToLinear(color[0]), SrgbToLinear(color[1]), SrgbToLinear(color[2]));
    changed = true;
  }
  changed |= ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 50.0f, "%.2f",
                                ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
  ImGui::SetItemTooltip("Irradiance on a surface facing the light");
  changed |= ImGui::SliderFloat("Size", &light.size, 0.1f, 45.0f, "%.1f deg",
                                ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
  ImGui::SetItemTooltip("Angular radius of the light, larger ones give softer shadows and highlights");
  bool behind = light.direction.z < 0.0f;
  if (ImGui::Checkbox("From behind", &behind)) {
    light.direction.z = -light.direction.z;
    // A light on the rim has no side, push it off slightly
    if (std::abs(light.direction.z) < 1.0e-3f) {
      light.direction = glm::normalize(glm::vec3(light.direction.x, light.direction.y, behind ? -0.05f : 0.05f));
    }
    changed = true;
  }
  ImGui::SetItemTooltip("Light from behind the painting, for rim lights");
  changed |= ImGui::Checkbox("Shadows", &light.shadows);
  ImGui::SetItemTooltip("The scene's shadow casters block the light");
  if (changed) {
    canvas->SetLocalLight(static_cast<uint32_t>(selected), light);
  }
}

void LsLightingPanel::DrawOccluderList() {
  ImGui::SeparatorText("Occluders");
  if (ImGui::Button("Add##Occluder")) {
    AddOccluder(glm::normalize(glm::vec3(0.6f, 0.4f, 0.5f)));
  }
  ImGui::SetItemTooltip("Shadows the pixels a light stroke paints it on from a cap of directions");
  ImGui::SameLine();
  ImGui::BeginDisabled(selectedOccluder < 0);
  if (ImGui::Button("Delete##Occluder")) {
    canvas->RemoveOccluder(static_cast<uint32_t>(selectedOccluder));
    selectedOccluder = -1;
  }
  ImGui::EndDisabled();
  ImGui::SetItemTooltip("Removes the occluder, also from where it is painted");

  for (uint32_t slot = 0; slot < kLsMaxOccluders; slot++) {
    const LsOccluder* occluder = canvas->GetOccluder(slot);
    if (!occluder) {
      continue;
    }
    ImGui::PushID(static_cast<int>(kLsMaxLocalLights + slot));
    // In the current rig, which changes every pixel the rig is painted on
    bool inRig = InRig(slot, true);
    ImGui::BeginDisabled(currentRig == 0);
    if (ImGui::Checkbox("##InRig", &inRig)) {
      SetInRig(slot, true, inRig);
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("In the selected rig, also where it is already painted");
    ImGui::SameLine();
    if (ImGui::Selectable(occluder->name.c_str(), static_cast<int>(slot) == selectedOccluder)) {
      SelectOccluder(static_cast<int>(slot));
    }
    ImGui::PopID();
  }
}

void LsLightingPanel::DrawSelectedOccluder() {
  if (selectedOccluder < 0) {
    return;
  }
  LsOccluder occluder = *canvas->GetOccluder(static_cast<uint32_t>(selectedOccluder));
  ImGui::SeparatorText(occluder.name.c_str());
  bool changed = false;
  char name[64];
  snprintf(name, sizeof(name), "%s", occluder.name.c_str());
  if (ImGui::InputText("Name##Occluder", name, sizeof(name))) {
    occluder.name = name;
    changed = true;
  }
  changed |= ImGui::SliderFloat("Size##Occluder", &occluder.size, 1.0f, 90.0f, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
  ImGui::SetItemTooltip("Angular radius of the directions it shadows");
  changed |= ImGui::SliderFloat("Softness", &occluder.softness, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
  ImGui::SetItemTooltip("How much of its radius it fades out over, soft shadow edges on area lights");
  changed |= ImGui::SliderFloat("Strength", &occluder.strength, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
  ImGui::SetItemTooltip("How much of the light from within it is blocked");
  bool behind = occluder.direction.z < 0.0f;
  if (ImGui::Checkbox("From behind##Occluder", &behind)) {
    occluder.direction.z = -occluder.direction.z;
    if (std::abs(occluder.direction.z) < 1.0e-3f) {
      occluder.direction = glm::normalize(glm::vec3(occluder.direction.x, occluder.direction.y, behind ? -0.05f : 0.05f));
    }
    changed = true;
  }
  ImGui::SetItemTooltip("Centred behind the painting");
  if (changed) {
    canvas->SetOccluder(static_cast<uint32_t>(selectedOccluder), occluder);
  }
}
