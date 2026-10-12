#include <algorithm>
#include <cmath>
#include <cstring>
#include <imgui.h>
#include <LsRenderer.h>
#include <LsCanvas.h>
#include <LsTransform.h>
#include <LsCanvasTool.h>

// A press that hardly moves places the tool at this radius on screen, in UI points
static const float kClickRadius = 40.0f;
// Smaller drags are clicks
static const float kMinDragRadius = 4.0f;
// Without painted depth, a previous tool or the scene under the press
static const float kDefaultDistance = 5.0f;
// A placement whose depth pick doesn't come back in this many frames uses the fallback
static const int kMaxPickFrames = 10;
// Up and Down move the tool by this fraction of its size, with Shift by the smaller one
static const float kDepthStep = 0.1f;
static const float kFineDepthStep = 0.02f;
// Cells of the silhouette grid along the longer side of the tool on screen
static const int kOutlineCells = 96;
static const float kMinOutlineCell = 1.5f; // in UI points
static const int kOutlineSteps = 256;
// Halvings of a grid cell's edge where the outline crosses it
static const int kOutlineBisections = 8;

// Radius of the shape's object space bounding box from its centre, along its widest axis
static float GetBoundingExtent(const LsShape& shape) {
  glm::vec3 min, max;
  if (!shape.GetBounds(min, max)) {
    return 1.0f;
  }
  float extent = std::max(std::max(max.x, max.y), max.z);
  return extent > 0.0f ? extent : 1.0f;
}

// Largest scale along any of the matrix's axes
static float GetMaxScale(const glm::mat4& m) {
  return std::max(std::max(glm::length(glm::vec3(m[0])), glm::length(glm::vec3(m[1]))), glm::length(glm::vec3(m[2])));
}

LsCanvasTool::LsCanvasTool(std::shared_ptr<LsRenderer> renderer, std::shared_ptr<LsScene> scene,
                           std::shared_ptr<LsCanvas> canvas) :
  renderer(renderer), scene(scene), canvas(canvas) {}

void LsCanvasTool::Arm(LsShapeType type) {
  armed = true;
  armedType = type;
  placing = Placing::None;
  hoverPicked = false;
}

void LsCanvasTool::Cancel() {
  placed = false;
  armed = false;
  placing = Placing::None;
  outline = Outline();
  previewOutline = Outline();
}

bool LsCanvasTool::GetPrimitive(LsScenePrimitive& primitive) const {
  if (!placed) {
    return false;
  }
  primitive = {};
  LsTransform objectToWorld(world);
  glm::mat4 worldToObject = glm::transpose(objectToWorld.GetInverseMatrix());
  for (int row = 0; row < 3; row++) {
    primitive.worldToObject[row] = worldToObject[row];
  }
  primitive.shapeParams = shape.params;
  // Lights nothing, the radius only keeps the field meaningful
  primitive.emission = glm::vec4(0.0f, 0.0f, 0.0f, GetBoundingExtent(shape)*std::sqrt(3.0f)*GetMaxScale(world));
  primitive.medium = glm::vec4(std::max(blendRadius, 0.0f), 0.0f, 0.0f, 0.0f);
  primitive.shapeType = static_cast<uint32_t>(shape.type);
  primitive.roles = eRoleTool | (carve ? eRoleToolCarve : ignoreScene ? eRoleToolOnly : 0u);
  primitive.distanceScale = objectToWorld.GetMinScale();
  primitive.nodeId = kLsInvalidNode;
  return true;
}

bool LsCanvasTool::WantsMouse() const {
  return armed || placing != Placing::None ||
         (placed && (gizmo.IsHovered() || gizmo.IsActive() || keys.ClaimsMouse()));
}

bool LsCanvasTool::UpdateProjection() {
  LsNodeId camera = scene->GetActiveCamera();
  glm::vec2 canvasSize(canvas->GetSize());
  if (camera == kLsInvalidNode || canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
    return false;
  }
  // The canvas is placed in window pixels, the UI draws in points
  glm::vec2 pixelsPerPoint = LsToVec2(ImGui::GetIO().DisplayFramebufferScale);
  projection.origin = renderer->GetCanvasOffset()/pixelsPerPoint;
  projection.size = glm::max(canvasSize*renderer->GetCanvasZoom()/pixelsPerPoint, glm::vec2(1.0e-3f));
  projection.camera = scene->GetCamera(camera, canvasSize.x/canvasSize.y);
  return true;
}

glm::vec3 LsCanvasTool::GetFallbackCenter(const LsRay& ray) const {
  const LsCamera& camera = projection.camera;
  float along = glm::dot(ray.direction, camera.GetForward());
  if (lastViewDepth > 0.0f && along > 1.0e-3f) {
    return ray.At(lastViewDepth/along);
  }
  LsSceneHit hit;
  if (scene->Raycast(ray, hit, eRoleConstruction)) {
    return hit.position;
  }
  return ray.At(kDefaultDistance);
}

glm::mat4 LsCanvasTool::GetPlacement(const LsShape& shape, glm::vec3 center, float radius) const {
  float scale = radius*projection.GetPixelSize(center)/GetBoundingExtent(shape);
  // Facing the camera: its object +Y, the way a plane faces and a torus opens,
  // points back at the camera, and its X runs along the camera's right
  const LsCamera& camera = projection.camera;
  glm::mat4 placement(1.0f);
  placement[0] = glm::vec4(camera.GetRight()*scale, 0.0f);
  placement[1] = glm::vec4(-camera.GetForward()*scale, 0.0f);
  placement[2] = glm::vec4(-camera.GetUp()*scale, 0.0f);
  placement[3] = glm::vec4(center, 1.0f);
  return placement;
}

bool LsCanvasTool::GetHoverCenter(glm::vec2 mouse, glm::vec3& center) const {
  if (!hoverPicked) {
    return false;
  }
  LsRay ray = projection.GetRay(mouse);
  center = hoverDistance > 0.0f ? ray.At(hoverDistance) : GetFallbackCenter(ray);
  return true;
}

void LsCanvasTool::PlaceAt(glm::vec3 center, glm::vec2 dragEnd) {
  shape = LsShape::Default(armedType);
  float radius = glm::length(dragEnd - pressPoint);
  world = GetPlacement(shape, center, radius < kMinDragRadius ? kClickRadius : radius);
  placed = true;
}

void LsCanvasTool::UpdatePlacing(bool mouseAvailable) {
  glm::vec2 mouse = LsToVec2(ImGui::GetIO().MousePos);
  if (placing == Placing::None) {
    if (!armed || !mouseAvailable) {
      hoverPicked = false;
      return;
    }
    float distance;
    if (canvas->TakePickedDistance(distance)) {
      hoverPicked = true;
      hoverDistance = distance;
    }
    glm::vec3 center;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      pressPoint = mouse;
      released = false;
      pickFrames = 0;
      // The distance under the mouse a frame ago is close enough to start from
      if (GetHoverCenter(mouse, placeCenter)) {
        placing = Placing::Dragging;
      }
      else {
        placing = Placing::WaitingForDepth;
      }
    }
    else if (GetHoverCenter(mouse, center)) {
      // What a click would place, before the press
      LsShape preview = LsShape::Default(armedType);
      UpdateOutline(previewOutline, preview, GetPlacement(preview, center, kClickRadius));
      DrawOutline(ImGui::GetBackgroundDrawList(), previewOutline, IM_COL32(160, 160, 160, 160));
    }
    // The distance under the mouse for the next frame
    glm::vec2 pixel = (mouse - projection.origin)/projection.size*glm::vec2(canvas->GetSize());
    canvas->RequestDistancePick(pixel);
    if (placing == Placing::None) {
      return;
    }
  }

  // The press may be over before the pick comes back
  if (!released && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    released = true;
    releasePoint = mouse;
  }
  if (placing == Placing::WaitingForDepth) {
    float distance = 0.0f;
    bool picked = canvas->TakePickedDistance(distance);
    if (!picked && ++pickFrames < kMaxPickFrames) {
      return;
    }
    LsRay ray = projection.GetRay(pressPoint);
    placeCenter = picked && distance > 0.0f ? ray.At(distance) : GetFallbackCenter(ray);
    placing = Placing::Dragging;
    hoverPicked = false;
  }
  PlaceAt(placeCenter, released ? releasePoint : mouse);
  if (released) {
    placing = Placing::None;
    armed = false;
    hoverPicked = false;
  }
}

void LsCanvasTool::MoveInDepth(float distance) {
  glm::vec3 center(world[3]);
  glm::vec3 away = center - projection.camera.GetPosition();
  float length = glm::length(away);
  if (length < 1.0e-6f) {
    return;
  }
  // Not through the camera
  distance = std::max(distance, 1.0e-3f - length);
  world[3] = glm::vec4(center + away/length*distance, 1.0f);
}

void LsCanvasTool::Update(bool mouseAvailable) {
  hasProjection = UpdateProjection();
  if (!hasProjection) {
    return;
  }
  // Under the panels, over the canvas
  ImDrawList* drawList = ImGui::GetBackgroundDrawList();
  if (armed || placing != Placing::None) {
    UpdatePlacing(mouseAvailable);
  }
  if (!placed) {
    return;
  }
  // Not while placing, a press then places a new tool
  bool toolMouse = mouseAvailable && !armed && placing == Placing::None;
  glm::vec3 axes[3];
  GetViewAxes(axes);
  keys.Update(projection, axes, toolMouse, world);
  UpdateOutline(outline, shape, world);
  DrawOutline(drawList, outline, IM_COL32(160, 160, 160, 255));
  if (keys.ShowsGizmo()) {
    gizmo.Manipulate(drawList, projection, keys.GetMode(), true, toolMouse, world);
    if (gizmo.IsActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      gizmo.Cancel(world);
    }
  }
  keys.DrawAxisLock(drawList, projection, world, axes);
  lastViewDepth = projection.camera.GetDepth(glm::vec3(world[3]));
}

// Marching squares over the smallest distance to the tool along the camera
// rays through a grid of points on screen: positive outside the silhouette and
// negative where the ray hits, so the zero crossing is the outline
void LsCanvasTool::UpdateOutline(Outline& outline, const LsShape& shape, const glm::mat4& world) const {
  const LsCamera& camera = projection.camera;
  OutlineKey key = {};
  key.world = world;
  key.cameraToWorld = camera.GetCameraToWorld();
  key.view = glm::vec4(projection.origin, projection.size);
  key.lens = glm::vec4(camera.GetTanHalfFovY(), camera.GetAspect(), camera.GetLensShift());
  key.shapeParams = shape.params;
  key.shapeType = static_cast<uint32_t>(shape.type);
  if (outline.valid && std::memcmp(&key, &outline.key, sizeof(key)) == 0) {
    return;
  }
  outline.key = key;
  outline.valid = true;
  outline.segments.clear();

  // The tool's bounding box on screen, or the whole canvas when part of it is behind the camera
  glm::vec3 boundsMin, boundsMax;
  if (!shape.GetBounds(boundsMin, boundsMax)) {
    return;
  }
  glm::vec2 screenMin(1.0e30f), screenMax(-1.0e30f);
  bool projected = true;
  for (int corner = 0; corner < 8 && projected; corner++) {
    glm::vec3 p((corner & 1) ? boundsMax.x : boundsMin.x, (corner & 2) ? boundsMax.y : boundsMin.y,
                (corner & 4) ? boundsMax.z : boundsMin.z);
    glm::vec2 screen;
    projected = projection.ToScreen(glm::vec3(world*glm::vec4(p, 1.0f)), screen);
    screenMin = glm::min(screenMin, screen);
    screenMax = glm::max(screenMax, screen);
  }
  glm::vec2 canvasMin = projection.origin;
  glm::vec2 canvasMax = projection.origin + projection.size;
  if (!projected) {
    screenMin = canvasMin;
    screenMax = canvasMax;
  }
  float cell = std::max(std::max(screenMax.x - screenMin.x, screenMax.y - screenMin.y)/kOutlineCells, kMinOutlineCell);
  // A cell of margin keeps the outline inside the grid, and only the canvas is drawn
  screenMin = glm::max(screenMin - cell, canvasMin);
  screenMax = glm::min(screenMax + cell, canvasMax);
  if (screenMax.x <= screenMin.x || screenMax.y <= screenMin.y) {
    return;
  }
  int columns = std::min(static_cast<int>(std::ceil((screenMax.x - screenMin.x)/cell)), 2*kOutlineCells);
  int rows = std::min(static_cast<int>(std::ceil((screenMax.y - screenMin.y)/cell)), 2*kOutlineCells);
  glm::vec2 step = (screenMax - screenMin)/glm::vec2(columns, rows);

  LsTransform objectToWorld(world);
  glm::mat4 worldToObject = objectToWorld.GetInverseMatrix();
  float distanceScale = objectToWorld.GetMinScale();
  glm::vec3 center(world[3]);
  float reach = glm::length(center - camera.GetPosition()) + 2.0f*GetBoundingExtent(shape)*std::sqrt(3.0f)*GetMaxScale(world);
  // Within a tenth of a UI point of the surface counts as a hit, near the
  // silhouette rays pass close to the surface for many steps
  float hitTolerance = 0.1f*projection.GetPixelSize(center)/std::max(glm::length(center - camera.GetPosition()), 1.0e-6f);
  auto hits = [&](glm::vec2 screen) {
    LsRay ray = projection.GetRay(screen);
    float t = 0.0f;
    for (int i = 0; i < kOutlineSteps && t < reach; i++) {
      float d = shape.Distance(glm::vec3(worldToObject*glm::vec4(ray.At(t), 1.0f)))*distanceScale;
      if (d < hitTolerance*std::max(t, 1.0e-3f)) {
        return true;
      }
      t += d;
    }
    return false;
  };

  std::vector<char> inside((columns + 1)*(rows + 1));
  for (int row = 0; row <= rows; row++) {
    for (int column = 0; column <= columns; column++) {
      inside[row*(columns + 1) + column] = hits(screenMin + glm::vec2(column, row)*step);
    }
  }

  auto isInside = [&](int column, int row) { return inside[row*(columns + 1) + column] != 0; };
  auto point = [&](int column, int row) { return screenMin + glm::vec2(column, row)*step; };
  // The silhouette between a point inside and one outside, found by bisection,
  // so the outline is as straight as the shape's edges
  auto cross = [&](glm::vec2 in, glm::vec2 out) {
    for (int i = 0; i < kOutlineBisections; i++) {
      glm::vec2 middle = (in + out)*0.5f;
      if (hits(middle)) {
        in = middle;
      }
      else {
        out = middle;
      }
    }
    return (in + out)*0.5f;
  };
  auto edge = [&](glm::vec2 p, glm::vec2 q, bool pInside, bool qInside) {
    return pInside == qInside ? p : pInside ? cross(p, q) : cross(q, p);
  };
  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < columns; column++) {
      // Corners a, b, c, d go round the cell from its top left
      bool ia = isInside(column, row), ib = isInside(column + 1, row);
      bool ic = isInside(column + 1, row + 1), id = isInside(column, row + 1);
      int index = (ia ? 1 : 0) | (ib ? 2 : 0) | (ic ? 4 : 0) | (id ? 8 : 0);
      if (index == 0 || index == 15) {
        continue;
      }
      glm::vec2 a = point(column, row), b = point(column + 1, row);
      glm::vec2 c = point(column + 1, row + 1), d = point(column, row + 1);
      // Crossings on the edges ab, bc, cd and da
      glm::vec2 e[4] = { edge(a, b, ia, ib), edge(b, c, ib, ic), edge(c, d, ic, id), edge(d, a, id, ia) };
      auto add = [&](int from, int to) { outline.segments.push_back({ e[from], e[to] }); };
      switch (index) {
        case 1: case 14: add(3, 0); break;
        case 2: case 13: add(0, 1); break;
        case 3: case 12: add(3, 1); break;
        case 4: case 11: add(1, 2); break;
        case 6: case 9:  add(0, 2); break;
        case 7: case 8:  add(3, 2); break;
        // Saddles, a and c inside or b and d: joined through the centre or not
        case 5:
          if (hits((a + c)*0.5f)) { add(0, 1); add(2, 3); } else { add(3, 0); add(1, 2); }
          break;
        case 10:
          if (hits((a + c)*0.5f)) { add(3, 0); add(1, 2); } else { add(0, 1); add(2, 3); }
          break;
      }
    }
  }
}

void LsCanvasTool::DrawOutline(ImDrawList* drawList, const Outline& outline, uint32_t color) {
  for (const auto& segment : outline.segments) {
    drawList->AddLine(LsToImVec2(segment.first), LsToImVec2(segment.second), color, 1.5f);
  }
}

void LsCanvasTool::GetViewAxes(glm::vec3 axes[3]) const {
  const LsCamera& camera = projection.camera;
  glm::vec3 toCamera = camera.GetPosition() - glm::vec3(world[3]);
  float length = glm::length(toCamera);
  axes[0] = camera.GetRight();
  axes[1] = camera.GetUp();
  // Along the ray through the centre, so moving along it keeps the tool where it is on the canvas
  axes[2] = length > 1.0e-6f ? toCamera/length : -camera.GetForward();
}

void LsCanvasTool::HandleShortcuts() {
  if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
    // A hold or a drag along an axis first, then the gizmo's drag
    if (keys.Cancel(world)) {
      return;
    }
    if (gizmo.IsActive()) {
      gizmo.Cancel(world);
    }
    else if (armed || placing != Placing::None) {
      armed = false;
      placing = Placing::None;
    }
    else {
      keys.SetAxisLock(-1);
    }
  }
  if (!placed || ImGui::GetIO().KeyCtrl) {
    return;
  }
  if (hasProjection) {
    keys.HandleKeys(world, !gizmo.IsActive());
  }
  // Up pulls the tool out of the painting towards the camera, Down pushes it in
  float direction = 0.0f;
  if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
    direction = -1.0f;
  }
  else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
    direction = 1.0f;
  }
  if (direction != 0.0f && hasProjection && !gizmo.IsActive() && !IsTransforming()) {
    float size = GetBoundingExtent(shape)*GetMaxScale(world);
    MoveInDepth(direction*size*(ImGui::GetIO().KeyShift ? kFineDepthStep : kDepthStep));
  }
}

void LsCanvasTool::DrawPanel() {
  // One button per primitive, the one being placed highlighted
  for (uint32_t type = 0; type < static_cast<uint32_t>(LsShapeType::Count); type++) {
    LsShapeType shapeType = static_cast<LsShapeType>(type);
    if (type % 3 != 0) {
      ImGui::SameLine();
    }
    bool current = armed && armedType == shapeType;
    if (current) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    if (ImGui::Button(LsShapeTypeName(shapeType), ImVec2(ImGui::GetFontSize()*5.0f, 0.0f))) {
      Arm(shapeType);
    }
    if (current) {
      ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip("Drag on the canvas to place a %s: the press is its centre, on the painted\n"
                          "depth there, and the drag sets its size", LsShapeTypeName(shapeType));
  }
  if (armed) {
    ImGui::TextDisabled("Drag on the canvas to place it (Esc stops)");
  }
  else if (!placed) {
    ImGui::TextDisabled("Pick a primitive, then drag on the canvas");
  }

  if (placed) {
    ImGui::SeparatorText("Transform");
    struct { const char* label; LsGizmoMode mode; const char* tooltip; } modes[] = {
      { "Move", LsGizmoMode::Translate, "Move the tool (W), hold W to move it in screen space" },
      { "Scale", LsGizmoMode::Scale, "Scale the tool (E), hold E to scale it uniformly" },
      { "Rotate", LsGizmoMode::Rotate, "Rotate the tool (R), hold R to rotate it in screen space" },
    };
    for (const auto& mode : modes) {
      if (&mode != modes) {
        ImGui::SameLine();
      }
      if (ImGui::RadioButton(mode.label, keys.GetMode() == mode.mode)) {
        keys.SetMode(mode.mode);
      }
      ImGui::SetItemTooltip("%s", mode.tooltip);
    }
    // The gizmo, or a view axis a drag anywhere on the canvas goes along
    const char* axisLabels[4] = { "Gizmo", "X", "Y", "Z" };
    const char* axisTooltips[4] = {
      "Drag the gizmo's handles, the mode's key again comes back here",
      "Drag anywhere along the screen's horizontal (X)",
      "Drag anywhere along the screen's vertical (Y)",
      "Drag up and down to move towards or away from the camera, or about the view axis (Z)",
    };
    for (int axis = -1; axis < 3; axis++) {
      if (axis >= 0) {
        ImGui::SameLine();
      }
      if (ImGui::RadioButton(axisLabels[axis + 1], keys.GetAxisLock() == axis)) {
        keys.SetAxisLock(axis);
      }
      ImGui::SetItemTooltip("%s", axisTooltips[axis + 1]);
    }
    if (hasProjection) {
      const LsCamera& camera = projection.camera;
      glm::vec3 center(world[3]);
      float depth = camera.GetDepth(center);
      if (ImGui::DragFloat("Depth", &depth, std::max(depth, 0.1f)*0.002f, 1.0e-3f, 1.0e4f, "%.3f")) {
        // Along the ray through the centre, so the tool stays where it is on the canvas
        float along = glm::dot(glm::normalize(center - camera.GetPosition()), camera.GetForward());
        if (along > 1.0e-3f) {
          MoveInDepth((depth - camera.GetDepth(center))/along);
        }
      }
      ImGui::SetItemTooltip("How far the tool's centre is in front of the canvas camera.\n"
                            "Up pulls it out of the painting, Down pushes it in, Shift finely.");
    }
    ImGui::SeparatorText("Mode");
    if (ImGui::RadioButton("Union", !carve)) {
      carve = false;
    }
    ImGui::SetItemTooltip("Depth and normal strokes transfer from the tool joined to the scene's construction geometry");
    ImGui::SameLine();
    if (ImGui::RadioButton("Carve", carve)) {
      carve = true;
    }
    ImGui::SetItemTooltip("Depth and normal strokes carve the painted surface with the tool: positions\n"
                          "inside it go back to its far side, normals take the cavity's");
    if (!carve) {
      ImGui::SliderFloat("Blend", &blendRadius, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
      ImGui::SetItemTooltip("How far the tool blends into the scene's construction geometry, in scene units");
      ImGui::Checkbox("Ignore scene", &ignoreScene);
      ImGui::SetItemTooltip("Depth and normal strokes transfer from the tool alone");
    }
  }
  if (placed || armed) {
    if (ImGui::Button("Cancel")) {
      Cancel();
    }
    ImGui::SetItemTooltip("Remove the tool");
  }
}
