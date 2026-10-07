#include <LsSceneViewport.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <LsRenderer.h>
#include <LsSceneEditor.h>

static const float kOrbitDegreesPerPixel = 0.3f;
static const float kLookDegreesPerPixel = 0.2f;
static const float kZoomPerWheelNotch = 0.85f;
static const float kMinPitch = -89.5f;
static const float kMaxPitch = 89.5f;
static const float kMinDistance = 0.01f;
static const float kMaxDistance = 10000.0f;
// Camera frustums are drawn this deep, the far end stands for the canvas
static const float kCameraDisplayDepth = 0.6f;
// The canvas frame fills this much of the view when looking through its camera
static const float kCameraFrameFill = 0.9f;
static const float kPickPixels = 6.0f;
// Flying between the canvas and the 3D view
static const double kTransitionSeconds = 0.6;

static const ImU32 kSelectionColor = IM_COL32(255, 140, 40, 255);

static glm::vec3 OrbitDirection(float yawDegrees, float pitchDegrees) {
  float yaw = glm::radians(yawDegrees);
  float pitch = glm::radians(pitchDegrees);
  return glm::vec3(std::cos(pitch)*std::sin(yaw), std::sin(pitch), std::cos(pitch)*std::cos(yaw));
}

using LsSegment = std::pair<glm::vec3, glm::vec3>;

// Pyramid from the eye to the canvas rectangle, with a triangle on top showing which way is up
static std::vector<LsSegment> CameraWireframe(const LsCamera& camera) {
  glm::vec3 eye = camera.GetPosition();
  glm::vec3 corners[4] = {
    camera.GetImagePlanePoint(glm::vec2(-1.0f, -1.0f), kCameraDisplayDepth),
    camera.GetImagePlanePoint(glm::vec2( 1.0f, -1.0f), kCameraDisplayDepth),
    camera.GetImagePlanePoint(glm::vec2( 1.0f,  1.0f), kCameraDisplayDepth),
    camera.GetImagePlanePoint(glm::vec2(-1.0f,  1.0f), kCameraDisplayDepth),
  };
  std::vector<LsSegment> segments;
  for (int i = 0; i < 4; i++) {
    segments.push_back({ eye, corners[i] });
    segments.push_back({ corners[i], corners[(i + 1) % 4] });
  }
  glm::vec3 up[3] = {
    camera.GetImagePlanePoint(glm::vec2(-0.5f, 1.15f), kCameraDisplayDepth),
    camera.GetImagePlanePoint(glm::vec2( 0.5f, 1.15f), kCameraDisplayDepth),
    camera.GetImagePlanePoint(glm::vec2( 0.0f, 1.6f), kCameraDisplayDepth),
  };
  for (int i = 0; i < 3; i++) {
    segments.push_back({ up[i], up[(i + 1) % 3] });
  }
  return segments;
}

// Where a camera's name goes, under the bottom left corner of its canvas
static bool CameraLabelPosition(const LsViewportProjection& projection, const LsCamera& camera, glm::vec2& position) {
  if (!projection.ToScreen(camera.GetImagePlanePoint(glm::vec2(-1.0f, -1.0f), kCameraDisplayDepth), position)) {
    return false;
  }
  position += glm::vec2(2.0f, 2.0f);
  return true;
}

static float DistanceToSegment(glm::vec2 p, glm::vec2 a, glm::vec2 b) {
  glm::vec2 ab = b - a;
  float lengthSquared = glm::dot(ab, ab);
  float t = lengthSquared > 0.0f ? glm::clamp(glm::dot(p - a, ab)/lengthSquared, 0.0f, 1.0f) : 0.0f;
  return glm::length(p - (a + ab*t));
}

// Highlighted while on, like the other tool strips
static bool ToggleButton(const char* label, bool on) {
  if (on) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive));
  }
  bool pressed = ImGui::Button(label);
  if (on) {
    ImGui::PopStyleColor(2);
  }
  return pressed;
}

static void AddTextWithShadow(ImDrawList* drawList, glm::vec2 position, ImU32 color, const char* text) {
  drawList->AddText(ImVec2(position.x + 1.0f, position.y + 1.0f), IM_COL32(0, 0, 0, 160), text);
  drawList->AddText(LsToImVec2(position), color, text);
}

LsSceneViewport::LsSceneViewport(std::shared_ptr<LsRenderer> renderer, std::shared_ptr<LsScene> scene, LsSceneEditor* editor) :
  renderer(renderer), scene(scene), editor(editor), view(std::make_unique<LsSceneView>(renderer)) { }

LsSceneViewport::~LsSceneViewport() = default;

glm::vec3 LsSceneViewport::GetOrbitPosition() const {
  return target + OrbitDirection(yaw, pitch)*distance;
}

LsCamera LsSceneViewport::GetOrbitCamera(float aspect) const {
  return LsCamera::LookAt(GetOrbitPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f), fov, aspect);
}

void LsSceneViewport::SetOrbitFromCamera(const LsCamera& camera) {
  // The orbit direction points from the target back to the eye
  glm::vec3 forward = camera.GetForward();
  pitch = glm::clamp(glm::degrees(std::asin(glm::clamp(-forward.y, -1.0f, 1.0f))), kMinPitch, kMaxPitch);
  yaw = glm::degrees(std::atan2(-forward.x, -forward.z));
  target = camera.GetPosition() + forward*distance;
}

float LsSceneViewport::GetCanvasAspect() const {
  glm::vec2 size = renderer->GetCanvasSize();
  return size.y > 0.0f ? size.x/size.y : 1.0f;
}

LsCamera LsSceneViewport::GetViewCamera(float viewAspect, glm::vec2& frameNdc) const {
  LsNodeId active = scene->GetActiveCamera();
  if (!lookThrough || active == kLsInvalidNode) {
    frameNdc = glm::vec2(0.0f);
    return GetOrbitCamera(viewAspect);
  }

  // Widen the view until the canvas frame fits inside it with a margin
  float canvasAspect = GetCanvasAspect();
  LsCamera camera = scene->GetCamera(active, canvasAspect);
  float tanY = camera.GetTanHalfFovY();
  float viewTanY = std::max(tanY, tanY*canvasAspect/viewAspect)/kCameraFrameFill;
  frameNdc = glm::vec2(tanY*canvasAspect/(viewTanY*viewAspect), tanY/viewTanY);
  return LsCamera(camera.GetCameraToWorld(), glm::degrees(2.0f*std::atan(viewTanY)), viewAspect);
}

void LsSceneViewport::SyncOrbitToCamera() {
  LsNodeId active = scene->GetActiveCamera();
  if (active == kLsInvalidNode) {
    return;
  }
  LsCamera camera = scene->GetCamera(active, GetCanvasAspect());
  // Orbiting a locked camera should turn it about its subject, not a point behind it
  LsSceneHit hit;
  LsRay ray = { camera.GetPosition(), camera.GetForward() };
  if (LsRaycastPrimitives(primitives, ray, kLsAllRoles, kMaxDistance, hit)) {
    distance = glm::clamp(hit.distance, kMinDistance, kMaxDistance);
  }
  SetOrbitFromCamera(camera);
}

void LsSceneViewport::SetLookThrough(bool enabled, bool restoreView) {
  if (scene->GetActiveCamera() == kLsInvalidNode) {
    enabled = false;
  }
  if (enabled && !lookThrough) {
    savedOrbit = { target, distance, yaw, pitch };
  }
  if (!enabled && lookThrough && restoreView) {
    target = savedOrbit.target;
    distance = savedOrbit.distance;
    yaw = savedOrbit.yaw;
    pitch = savedOrbit.pitch;
  }
  else if (lookThrough || enabled) {
    SyncOrbitToCamera();
  }
  lookThrough = enabled;
}

void LsSceneViewport::SetWorldMatrix(LsNodeId id, const glm::mat4& world) {
  const LsSceneNode* node = scene->Find(id);
  if (!node) {
    return;
  }
  glm::mat4 parentToWorld = scene->GetObjectToWorld(node->GetParent()).GetMatrix();
  const LsPlacement& current = node->GetProperties().placement;
  scene->SetPlacement(id, LsPlacement::FromMatrix(glm::inverse(parentToWorld)*world, current));
}

void LsSceneViewport::AlignCameraToView() {
  LsNodeId active = scene->GetActiveCamera();
  if (active == kLsInvalidNode) {
    active = scene->AddCamera("Camera");
  }
  if (!lookThrough) {
    SetWorldMatrix(active, GetOrbitCamera(1.0f).GetCameraToWorld());
  }
  SetLookThrough(true);
}

void LsSceneViewport::FrameSelection() {
  LsNodeId selected = editor->GetSelected();
  if (!scene->Find(selected)) {
    selected = scene->GetRoot();
  }
  glm::vec3 min, max;
  if (!scene->GetWorldBounds(selected, min, max)) {
    min = max = scene->GetObjectToWorld(selected).ApplyToPoint(glm::vec3(0.0f));
  }

  if (lookThrough && !lockCameraToView) {
    SetLookThrough(false, false);
  }
  target = (min + max)*0.5f;
  float radius = glm::length(max - min)*0.5f;
  // A point has no size to fit, cameras show their frustum and anything else
  // (such as a plane) keeps the zoom
  if (radius < 1.0e-4f) {
    const LsSceneNode* node = scene->Find(selected);
    radius = node->GetProperties().type == LsNodeType::Camera ? kCameraDisplayDepth*2.0f : 0.0f;
  }
  if (radius > 0.0f) {
    distance = glm::clamp(radius/std::sin(glm::radians(fov)*0.5f)*1.1f, kMinDistance, kMaxDistance);
  }
  if (lookThrough) {
    SetWorldMatrix(scene->GetActiveCamera(), GetOrbitCamera(1.0f).GetCameraToWorld());
  }
}

void LsSceneViewport::CenterViewOnCursor() {
  if (lookThrough && !lockCameraToView) {
    SetLookThrough(false, false);
  }
  target = cursor;
  if (lookThrough) {
    SetWorldMatrix(scene->GetActiveCamera(), GetOrbitCamera(1.0f).GetCameraToWorld());
  }
}

void LsSceneViewport::ViewAlongAxis(int axis, bool negative) {
  if (lookThrough && !lockCameraToView) {
    SetLookThrough(false, false);
  }
  // Top and bottom views have X to the right, like the front view
  const float yaws[3] = { 90.0f, 0.0f, 0.0f };
  const float pitches[3] = { 0.0f, kMaxPitch, 0.0f };
  yaw = negative ? yaws[axis] + (axis == 1 ? 0.0f : 180.0f) : yaws[axis];
  pitch = negative ? -pitches[axis] : pitches[axis];
  if (lookThrough) {
    SetWorldMatrix(scene->GetActiveCamera(), GetOrbitCamera(1.0f).GetCameraToWorld());
  }
}

void LsSceneViewport::PlaceCursor(glm::vec2 mouse) {
  LsRay ray = projection.GetRay(mouse);
  LsSceneHit hit;
  if (LsRaycastPrimitives(primitives, ray, kLsAllRoles, kMaxDistance, hit)) {
    cursor = hit.position;
  }
  else if (std::abs(ray.direction.y) > 1.0e-4f && ray.origin.y*ray.direction.y < 0.0f) {
    // Nothing there, the ground plane through the origin stands in
    cursor = ray.At(-ray.origin.y/ray.direction.y);
  }
  // The Scene panel's Add menu puts new objects there as well
  editor->SetCreationPoint(cursor);
}

void LsSceneViewport::DrawCursor(ImDrawList* drawList) const {
  glm::vec2 center;
  if (!projection.ToScreen(cursor, center)) {
    return;
  }
  // Blender's look: a ring of alternating red and white with a crosshair through a gap
  const float radius = 10.0f;
  const int segments = 8;
  for (int i = 0; i < segments; i++) {
    float a0 = 2.0f*3.14159265f*i/segments;
    float a1 = 2.0f*3.14159265f*(i + 1)/segments;
    ImU32 color = (i % 2) ? IM_COL32(255, 255, 255, 255) : IM_COL32(230, 60, 60, 255);
    drawList->PathArcTo(LsToImVec2(center), radius, a0, a1, 6);
    drawList->PathStroke(color, 0, 2.0f);
  }
  const ImU32 cross = IM_COL32(20, 20, 20, 220);
  const float inner = 4.0f;
  const float outer = radius + 6.0f;
  glm::vec2 directions[4] = { { 1.0f, 0.0f }, { -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f } };
  for (glm::vec2 d : directions) {
    drawList->AddLine(LsToImVec2(center + d*inner), LsToImVec2(center + d*outer), cross, 1.0f);
  }
}

bool LsSceneViewport::Navigate(bool viewHovered, glm::vec2 viewSize) {
  ImGuiIO& io = ImGui::GetIO();

  leftClicked = false;
  if (navigation == Navigation::None && viewHovered && !gizmo.IsActive() && !gizmo.IsHovered()) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      navigation = io.KeyAlt ? Navigation::Zoom : Navigation::Fly;
    }
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
      navigation = Navigation::Pan;
    }
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && viewPressed) {
      // With alt it orbits right away, otherwise only once the press turns into a drag
      if (io.KeyAlt) {
        navigation = Navigation::Orbit;
      }
      else {
        leftPressPending = true;
      }
    }
  }
  if (leftPressPending) {
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      leftPressPending = false;
      leftClicked = true;
    }
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      leftPressPending = false;
      navigation = Navigation::Orbit;
    }
  }
  ImGuiMouseButton button = navigation == Navigation::Orbit ? ImGuiMouseButton_Left :
                            navigation == Navigation::Pan ? ImGuiMouseButton_Middle : ImGuiMouseButton_Right;
  if (navigation != Navigation::None && !ImGui::IsMouseDown(button)) {
    navigation = Navigation::None;
  }

  float wheel = viewHovered ? io.MouseWheel : 0.0f;
  if (navigation == Navigation::None && wheel == 0.0f) {
    return false;
  }

  // Looking through the canvas camera, navigating either takes the camera along or leaves it behind
  if (lookThrough && !lockCameraToView) {
    SetLookThrough(false, false);
  }

  glm::vec2 delta = ImGui::IsMousePosValid() ? LsToVec2(io.MouseDelta) : glm::vec2(0.0f);
  bool moved = delta != glm::vec2(0.0f) || wheel != 0.0f;
  LsCamera camera = GetOrbitCamera(viewSize.x/viewSize.y);
  float pixelSize = distance*2.0f*camera.GetTanHalfFovY()/viewSize.y;

  switch (navigation) {
    case Navigation::Orbit:
      yaw -= delta.x*kOrbitDegreesPerPixel;
      pitch = glm::clamp(pitch + delta.y*kOrbitDegreesPerPixel, kMinPitch, kMaxPitch);
      break;
    case Navigation::Pan:
      // The point under the cursor follows it
      target += (-camera.GetRight()*delta.x + camera.GetUp()*delta.y)*pixelSize;
      break;
    case Navigation::Zoom:
      distance = glm::clamp(distance*std::exp(-(delta.x - delta.y)*0.01f), kMinDistance, kMaxDistance);
      break;
    case Navigation::Fly: {
      // Turning around the eye instead of the target
      glm::vec3 eye = GetOrbitPosition();
      yaw -= delta.x*kLookDegreesPerPixel;
      pitch = glm::clamp(pitch + delta.y*kLookDegreesPerPixel, kMinPitch, kMaxPitch);
      target = eye - OrbitDirection(yaw, pitch)*distance;

      if (wheel != 0.0f) {
        flySpeedScale = glm::clamp(flySpeedScale*std::pow(1.25f, wheel), 1.0f/64.0f, 64.0f);
        wheel = 0.0f;
      }
      camera = GetOrbitCamera(1.0f);
      glm::vec3 move(0.0f);
      if (!io.WantTextInput) {
        move += camera.GetForward()*(float(ImGui::IsKeyDown(ImGuiKey_W)) - float(ImGui::IsKeyDown(ImGuiKey_S)));
        move += camera.GetRight()*(float(ImGui::IsKeyDown(ImGuiKey_D)) - float(ImGui::IsKeyDown(ImGuiKey_A)));
        move += glm::vec3(0.0f, 1.0f, 0.0f)*(float(ImGui::IsKeyDown(ImGuiKey_E)) - float(ImGui::IsKeyDown(ImGuiKey_Q)));
      }
      if (move != glm::vec3(0.0f)) {
        // Long pauses between frames would make a jump
        float dt = std::min(io.DeltaTime, 0.05f);
        // Crossing the orbit distance takes a second, so speed suits the scale being worked at
        float speed = std::max(distance, 0.5f)*flySpeedScale*(io.KeyShift ? 4.0f : 1.0f);
        target += glm::normalize(move)*speed*dt;
        moved = true;
      }
    }
    break;
    default:
      break;
  }

  if (wheel != 0.0f) {
    distance = glm::clamp(distance*std::pow(kZoomPerWheelNotch, wheel), kMinDistance, kMaxDistance);
  }

  if (moved && lookThrough) {
    SetWorldMatrix(scene->GetActiveCamera(), GetOrbitCamera(1.0f).GetCameraToWorld());
  }
  return moved;
}

void LsSceneViewport::HandleShortcuts(bool viewHovered) {
  ImGuiIO& io = ImGui::GetIO();
  bool focused = viewHovered || ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
  // Keys belong to flying while it lasts
  if (!focused || io.WantTextInput || navigation == Navigation::Fly) {
    return;
  }

  // Escape and right clicks cancel a drag where the gizmo is drawn
  if (gizmo.IsActive()) {
    return;
  }

  if (!io.KeyCtrl && !io.KeyAlt) {
    if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
      gizmoMode = LsGizmoMode::Translate;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
      gizmoMode = LsGizmoMode::Rotate;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
      gizmoMode = LsGizmoMode::Scale;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
      FrameSelection();
    }
  }
  if (ImGui::IsKeyChordPressed(ImGuiMod_Alt | ImGuiKey_Home)) {
    CenterViewOnCursor();
  }

  // Only with focus, the outliner has the same keys and hovering here mustn't double them
  LsNodeId selected = editor->GetSelected();
  bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
  if (windowFocused && scene->Find(selected) && selected != scene->GetRoot()) {
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
      scene->Remove(selected);
      editor->SetSelected(kLsInvalidNode);
    }
    else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_D)) {
      editor->SetSelected(scene->Duplicate(selected));
    }
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Keypad0, false)) {
    if (io.KeyCtrl && io.KeyAlt) {
      AlignCameraToView();
    }
    else {
      SetLookThrough(!lookThrough);
    }
  }
  // Front, right and top views, ctrl looks from the opposite side
  if (ImGui::IsKeyPressed(ImGuiKey_Keypad1, false)) {
    ViewAlongAxis(2, io.KeyCtrl);
  }
  if (ImGui::IsKeyPressed(ImGuiKey_Keypad3, false)) {
    ViewAlongAxis(0, io.KeyCtrl);
  }
  if (ImGui::IsKeyPressed(ImGuiKey_Keypad7, false)) {
    ViewAlongAxis(1, io.KeyCtrl);
  }
}

void LsSceneViewport::UpdatePrimitives() {
  LsNodeId selected = editor->GetSelected();
  if (scene->GetRevision() == primitivesRevision && selected == drawnSelection && hovered == drawnHover) {
    return;
  }

  primitives = scene->Flatten();
  bool hasSelection = scene->Find(selected) != nullptr;
  for (LsScenePrimitive& primitive : primitives) {
    if (primitive.roles == 0) {
      primitive.roles |= kLsViewUnassigned;
    }
    // Selecting a group selects everything in it
    if (hasSelection && scene->IsAncestor(selected, primitive.nodeId)) {
      primitive.roles |= kLsViewSelected;
    }
    if (primitive.nodeId == hovered) {
      primitive.roles |= kLsViewHovered;
    }
  }
  view->SetPrimitives(primitives);

  primitivesRevision = scene->GetRevision();
  drawnSelection = selected;
  drawnHover = hovered;
}

std::vector<LsNodeId> LsSceneViewport::GetDrawnCameras() const {
  LsNodeId lookedThrough = lookThrough ? scene->GetActiveCamera() : kLsInvalidNode;
  std::vector<LsNodeId> cameras;
  std::vector<LsNodeId> pending = { scene->GetRoot() };
  while (!pending.empty()) {
    const LsSceneNode* node = scene->Find(pending.back());
    pending.pop_back();
    // Hiding a node hides its subtree
    if (!node->GetProperties().visible) {
      continue;
    }
    if (node->GetProperties().type == LsNodeType::Camera && node->GetId() != lookedThrough) {
      cameras.push_back(node->GetId());
    }
    const std::vector<LsNodeId>& children = node->GetChildren();
    pending.insert(pending.end(), children.rbegin(), children.rend());
  }
  return cameras;
}

LsNodeId LsSceneViewport::PickCamera(glm::vec2 mouse) const {
  LsNodeId best = kLsInvalidNode;
  float bestDistance = kPickPixels;
  float aspect = GetCanvasAspect();
  for (LsNodeId id : GetDrawnCameras()) {
    LsCamera camera = scene->GetCamera(id, aspect);
    // The name is an easier target than the thin lines
    glm::vec2 label;
    if (CameraLabelPosition(projection, camera, label)) {
      glm::vec2 size = LsToVec2(ImGui::CalcTextSize(scene->Find(id)->GetProperties().name.c_str()));
      if (mouse.x >= label.x && mouse.y >= label.y && mouse.x <= label.x + size.x && mouse.y <= label.y + size.y) {
        return id;
      }
    }
    for (const LsSegment& segment : CameraWireframe(camera)) {
      glm::vec2 a, b;
      if (projection.SegmentToScreen(segment.first, segment.second, a, b)) {
        float d = DistanceToSegment(mouse, a, b);
        if (d < bestDistance) {
          bestDistance = d;
          best = id;
        }
      }
    }
  }
  return best;
}

LsNodeId LsSceneViewport::Pick(glm::vec2 mouse) const {
  // Wireframes are thin, they win over the surfaces behind them
  LsNodeId camera = PickCamera(mouse);
  if (camera != kLsInvalidNode) {
    return camera;
  }
  LsSceneHit hit;
  if (LsRaycastPrimitives(primitives, projection.GetRay(mouse), kLsAllRoles, 1000.0f, hit)) {
    return hit.node;
  }
  return kLsInvalidNode;
}

void LsSceneViewport::DrawCameras(ImDrawList* drawList) const {
  LsNodeId selected = editor->GetSelected();
  LsNodeId active = scene->GetActiveCamera();
  float aspect = GetCanvasAspect();
  for (LsNodeId id : GetDrawnCameras()) {
    LsCamera camera = scene->GetCamera(id, aspect);
    // Seen from its own eye it would only be lines across the whole view
    if (glm::length(camera.GetPosition() - projection.camera.GetPosition()) < kCameraDisplayDepth) {
      continue;
    }
    bool isSelected = scene->IsAncestor(selected, id);
    ImU32 color = isSelected ? kSelectionColor :
                  id == hovered ? IM_COL32(255, 255, 255, 255) :
                  id == active ? IM_COL32(215, 215, 215, 255) : IM_COL32(140, 140, 140, 255);
    for (const LsSegment& segment : CameraWireframe(camera)) {
      projection.DrawLine(drawList, segment.first, segment.second, color, isSelected ? 2.0f : 1.5f);
    }

    // The camera painting the canvas has a filled up marker
    if (id == active) {
      glm::vec2 up[3];
      if (projection.ToScreen(camera.GetImagePlanePoint(glm::vec2(-0.5f, 1.15f), kCameraDisplayDepth), up[0]) &&
          projection.ToScreen(camera.GetImagePlanePoint(glm::vec2( 0.5f, 1.15f), kCameraDisplayDepth), up[1]) &&
          projection.ToScreen(camera.GetImagePlanePoint(glm::vec2( 0.0f, 1.6f), kCameraDisplayDepth), up[2])) {
        drawList->AddTriangleFilled(LsToImVec2(up[0]), LsToImVec2(up[1]), LsToImVec2(up[2]), color);
      }
    }

    glm::vec2 label;
    if (CameraLabelPosition(projection, camera, label)) {
      AddTextWithShadow(drawList, label, color, scene->Find(id)->GetProperties().name.c_str());
    }
  }
}

void LsSceneViewport::DrawCameraFrame(ImDrawList* drawList, glm::vec2 frameNdc) const {
  glm::vec2 min = projection.NdcToScreen(glm::vec2(-frameNdc.x, frameNdc.y));
  glm::vec2 max = projection.NdcToScreen(glm::vec2(frameNdc.x, -frameNdc.y));
  glm::vec2 viewMin = projection.origin;
  glm::vec2 viewMax = projection.origin + projection.size;

  // Dim what the canvas won't show
  ImU32 shade = IM_COL32(0, 0, 0, 120);
  drawList->AddRectFilled(LsToImVec2(viewMin), ImVec2(viewMax.x, min.y), shade);
  drawList->AddRectFilled(ImVec2(viewMin.x, max.y), LsToImVec2(viewMax), shade);
  drawList->AddRectFilled(ImVec2(viewMin.x, min.y), ImVec2(min.x, max.y), shade);
  drawList->AddRectFilled(ImVec2(max.x, min.y), ImVec2(viewMax.x, max.y), shade);
  ImU32 color = lockCameraToView ? kSelectionColor : IM_COL32(220, 220, 220, 255);
  drawList->AddRect(LsToImVec2(min), LsToImVec2(max), color, 0.0f, 0, 1.5f);

  char label[160];
  const LsSceneNode* camera = scene->Find(scene->GetActiveCamera());
  snprintf(label, sizeof(label), "%s%s", camera ? camera->GetProperties().name.c_str() : "",
           lockCameraToView ? "  (moves with the view)" : "");
  AddTextWithShadow(drawList, glm::vec2(min.x, min.y - ImGui::GetTextLineHeight() - 2.0f), color, label);
}

bool LsSceneViewport::DrawAxisWidget(ImDrawList* drawList, glm::vec2 corner, bool viewHovered) {
  const float radius = 30.0f;
  const float knob = 8.0f;
  const ImU32 colors[3] = { IM_COL32(230, 75, 75, 255), IM_COL32(110, 190, 60, 255), IM_COL32(70, 130, 230, 255) };
  const char* names[3] = { "X", "Y", "Z" };
  glm::vec2 center = corner + glm::vec2(-radius - knob - 6.0f, radius + knob + 6.0f);
  const LsCamera& camera = projection.camera;

  struct Knob { int axis; bool negative; glm::vec2 position; float depth; };
  std::vector<Knob> knobs;
  for (int axis = 0; axis < 3; axis++) {
    for (int side = 0; side < 2; side++) {
      glm::vec3 direction(0.0f);
      direction[axis] = side ? -1.0f : 1.0f;
      glm::vec2 offset(glm::dot(direction, camera.GetRight()), -glm::dot(direction, camera.GetUp()));
      knobs.push_back({ axis, side == 1, center + offset*radius, glm::dot(direction, camera.GetForward()) });
    }
  }
  // Far knobs first, so near ones are drawn over them
  std::sort(knobs.begin(), knobs.end(), [](const Knob& a, const Knob& b) { return a.depth > b.depth; });

  glm::vec2 mouse = LsToVec2(ImGui::GetIO().MousePos);
  const Knob* hoveredKnob = nullptr;
  if (viewHovered) {
    for (const Knob& k : knobs) {
      if (glm::length(mouse - k.position) <= knob) {
        hoveredKnob = &k; // the nearest one, being last
      }
    }
  }

  drawList->AddCircleFilled(LsToImVec2(center), radius + knob + 2.0f,
                            hoveredKnob ? IM_COL32(255, 255, 255, 30) : IM_COL32(255, 255, 255, 12));
  for (const Knob& k : knobs) {
    ImU32 color = colors[k.axis];
    if (!k.negative) {
      drawList->AddLine(LsToImVec2(center), LsToImVec2(k.position), color, 2.0f);
    }
    ImU32 fill = k.negative ? ((color & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 90)) : color;
    drawList->AddCircleFilled(LsToImVec2(k.position), knob, fill);
    if (&k == hoveredKnob) {
      drawList->AddCircle(LsToImVec2(k.position), knob, IM_COL32(255, 255, 255, 255), 0, 1.5f);
    }
    if (!k.negative) {
      ImVec2 size = ImGui::CalcTextSize(names[k.axis]);
      drawList->AddText(ImVec2(k.position.x - size.x*0.5f, k.position.y - size.y*0.5f), IM_COL32(20, 20, 20, 255), names[k.axis]);
    }
  }

  if (hoveredKnob) {
    ImGui::SetTooltip("View from %s%s", hoveredKnob->negative ? "-" : "+", names[hoveredKnob->axis]);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      ViewAlongAxis(hoveredKnob->axis, hoveredKnob->negative);
      return true;
    }
  }
  return false;
}

void LsSceneViewport::DrawStatus(ImDrawList* drawList) const {
  char text[160];
  if (navigation == Navigation::Fly) {
    snprintf(text, sizeof(text), "W A S D to fly, Q E down and up, shift faster, wheel sets speed (x%.2f)", flySpeedScale);
  }
  else if (gizmo.IsActive()) {
    snprintf(text, sizeof(text), "Ctrl snaps, Esc or right click cancels");
  }
  else {
    return;
  }
  float y = projection.origin.y + projection.size.y - ImGui::GetTextLineHeight() - 6.0f;
  AddTextWithShadow(drawList, glm::vec2(projection.origin.x + 8.0f, y), IM_COL32(230, 230, 230, 255), text);
}

void LsSceneViewport::DrawAddMenu() {
  if (!ImGui::BeginPopup("ViewportAdd")) {
    return;
  }
  LsNodeId added = kLsInvalidNode;
  // New things go to the 3D cursor
  LsPlacement placement;
  placement.translation = cursor;
  if (ImGui::MenuItem("Group")) {
    added = scene->AddGroup("Group");
    scene->SetPlacement(added, placement);
  }
  if (ImGui::MenuItem("Camera")) {
    added = scene->AddCamera("Camera");
    // Seeing what the view sees, so it frames the current shot
    SetWorldMatrix(added, GetOrbitCamera(1.0f).GetCameraToWorld());
  }
  ImGui::SetItemTooltip("Placed at the current view, so it frames what the view shows");
  ImGui::Separator();
  for (uint32_t i = 0; i < static_cast<uint32_t>(LsShapeType::Count); i++) {
    LsShapeType type = static_cast<LsShapeType>(i);
    if (ImGui::MenuItem(LsShapeTypeName(type))) {
      added = scene->AddShape(LsShapeTypeName(type), LsShape::Default(type), eRoleConstruction);
      scene->SetPlacement(added, placement);
    }
  }
  if (added != kLsInvalidNode) {
    editor->SetSelected(added);
  }
  ImGui::EndPopup();
}

void LsSceneViewport::DrawViewMenu() {
  if (!ImGui::BeginPopup("ViewportView")) {
    return;
  }
  ImGui::Checkbox("Lit by the scene", &settings.lit);
  ImGui::SetItemTooltip("Preview the light rig: emitters as point lights, with shadows from shadow casters");
  ImGui::BeginDisabled(!settings.lit);
  ImGui::SetNextItemWidth(ImGui::GetFontSize()*10.0f);
  ImGui::SliderFloat("Exposure", &settings.exposure, -10.0f, 10.0f, "%+.1f EV");
  ImGui::EndDisabled();
  ImGui::Checkbox("Grid", &settings.grid);
  if (ImGui::MenuItem("Centre view on cursor", "Alt+Home")) {
    CenterViewOnCursor();
  }
  ImGui::SetNextItemWidth(ImGui::GetFontSize()*10.0f);
  ImGui::SliderFloat("Field of view", &fov, 10.0f, 120.0f, "%.0f\xC2\xB0");
  ImGui::SetItemTooltip("Of the editor view, the canvas camera has its own");

  ImGui::SeparatorText("Navigation");
  static const char* kControls[][2] = {
    { "Left drag", "Orbit" },
    { "Double click", "Place the 3D cursor, where new objects go" },
    { "Alt + Home", "Centre the view on the 3D cursor" },
    { "Middle drag", "Pan" },
    { "Wheel, alt + right drag", "Zoom" },
    { "Right drag + WASD QE", "Fly" },
    { "F", "Frame selection" },
    { "W  E  R", "Move, rotate, scale" },
    { "Numpad 1  3  7", "Front, right, top" },
    { "Numpad 0", "Look through camera" },
    { "Ctrl + Alt + Numpad 0", "Align camera to view" },
  };
  if (ImGui::BeginTable("Controls", 2, ImGuiTableFlags_SizingFixedFit)) {
    for (const auto& control : kControls) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextDisabled("%s", control[0]);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(control[1]);
    }
    ImGui::EndTable();
  }
  ImGui::EndPopup();
}

void LsSceneViewport::DrawToolbar() {
  // The window has no padding so the view can reach its edges, the toolbar gets its own
  ImVec2 start = ImGui::GetCursorPos();
  ImGui::SetCursorPos(ImVec2(start.x + 6.0f, start.y + 4.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));

  if (ImGui::Button("Canvas")) {
    ToggleSceneView();
  }
  ImGui::SetItemTooltip("Back to painting on the canvas (Tab)");
  ImGui::SameLine(0.0f, 16.0f);

  struct { const char* label; LsGizmoMode mode; const char* tooltip; } tools[] = {
    { "Move", LsGizmoMode::Translate, "Move the selection (W)" },
    { "Rotate", LsGizmoMode::Rotate, "Rotate the selection (E)" },
    { "Scale", LsGizmoMode::Scale, "Scale the selection along its own axes (R)" },
  };
  for (const auto& tool : tools) {
    if (ToggleButton(tool.label, gizmoMode == tool.mode)) {
      gizmoMode = tool.mode;
    }
    ImGui::SetItemTooltip("%s", tool.tooltip);
    ImGui::SameLine(0.0f, 2.0f);
  }
  ImGui::SameLine();
  if (ImGui::Button(gizmoLocal ? "Local" : "World")) {
    gizmoLocal = !gizmoLocal;
  }
  ImGui::SetItemTooltip("Move and rotate along the world axes or the object's own");

  ImGui::SameLine(0.0f, 16.0f);
  if (ImGui::Button("Add")) {
    ImGui::OpenPopup("ViewportAdd");
  }
  DrawAddMenu();
  ImGui::SameLine();
  if (ImGui::Button("View")) {
    ImGui::OpenPopup("ViewportView");
  }
  DrawViewMenu();

  ImGui::SameLine(0.0f, 16.0f);
  bool hasCamera = scene->GetActiveCamera() != kLsInvalidNode;
  ImGui::BeginDisabled(!hasCamera);
  if (ToggleButton("Camera view", lookThrough)) {
    SetLookThrough(!lookThrough);
  }
  ImGui::SetItemTooltip("Look through the camera that paints the canvas (Numpad 0)");
  ImGui::SameLine(0.0f, 2.0f);
  if (ToggleButton("Lock", lockCameraToView)) {
    lockCameraToView = !lockCameraToView;
  }
  ImGui::SetItemTooltip("While looking through the camera, navigating moves it");
  ImGui::EndDisabled();
  ImGui::SameLine(0.0f, 2.0f);
  if (ImGui::Button("Align")) {
    AlignCameraToView();
  }
  ImGui::SetItemTooltip("Move the canvas camera to the current view (Ctrl+Alt+Numpad 0)");

  ImGui::PopStyleVar();
  ImGui::SetCursorPosX(start.x);
  ImGui::Dummy(ImVec2(0.0f, 2.0f));
}

// In between two views: position and the frustum's shape blend, orientation turns along the shorter way
static LsCamera InterpolateCamera(const LsCamera& a, const LsCamera& b, float t) {
  glm::quat qa = glm::quat_cast(glm::mat3(a.GetCameraToWorld()));
  glm::quat qb = glm::quat_cast(glm::mat3(b.GetCameraToWorld()));
  if (glm::dot(qa, qb) < 0.0f) {
    qb = -qb;
  }
  glm::mat4 cameraToWorld = glm::mat4_cast(glm::normalize(glm::slerp(qa, qb, t)));
  cameraToWorld[3] = glm::vec4(glm::mix(a.GetPosition(), b.GetPosition(), t), 1.0f);
  // Zooming looks even when the field of view changes geometrically
  float tanY = std::exp(glm::mix(std::log(a.GetTanHalfFovY()), std::log(b.GetTanHalfFovY()), t));
  LsCamera camera(cameraToWorld, glm::degrees(2.0f*std::atan(tanY)), b.GetAspect());
  camera.SetLensShift(glm::mix(a.GetLensShift(), b.GetLensShift(), t));
  return camera;
}

bool LsSceneViewport::GetCanvasAlignedCamera(LsCamera& camera) const {
  LsNodeId active = scene->GetActiveCamera();
  if (active == kLsInvalidNode) {
    return false;
  }
  // The canvas is placed in window pixels, the view in UI points
  glm::vec2 pixelsPerPoint = LsToVec2(ImGui::GetIO().DisplayFramebufferScale);
  glm::vec2 canvasMin = renderer->GetCanvasOffset()/pixelsPerPoint;
  glm::vec2 canvasSize = renderer->GetCanvasSize()*renderer->GetCanvasZoom()/pixelsPerPoint;
  if (canvasSize.y <= 0.0f) {
    return false;
  }

  // Each UI point covers the same angle as on the canvas, and the lens shift
  // moves the image so the canvas centre is where it is drawn
  LsCamera canvasCamera = scene->GetCamera(active, GetCanvasAspect());
  float tanPerPoint = canvasCamera.GetTanHalfFovY()/(canvasSize.y*0.5f);
  glm::vec2 canvasCenter = canvasMin + canvasSize*0.5f;
  glm::vec2 viewCenter = projection.origin + projection.size*0.5f;
  float viewTanY = tanPerPoint*projection.size.y*0.5f;
  camera = LsCamera(canvasCamera.GetCameraToWorld(), glm::degrees(2.0f*std::atan(viewTanY)), projection.size.x/projection.size.y);
  camera.SetLensShift(glm::vec2(viewCenter.x - canvasCenter.x, canvasCenter.y - viewCenter.y)*tanPerPoint);
  return true;
}

float LsSceneViewport::GetTransitionProgress() const {
  return glm::clamp(static_cast<float>((ImGui::GetTime() - transitionStart)/kTransitionSeconds), 0.0f, 1.0f);
}

void LsSceneViewport::ToggleSceneView() {
  bool toScene = mode == Mode::Canvas || mode == Mode::ToCanvas;
  bool canFly = scene->GetActiveCamera() != kLsInvalidNode;
  // Turning round halfway carries on from where the view is
  double now = ImGui::GetTime();
  if (mode == Mode::ToScene || mode == Mode::ToCanvas) {
    transitionStart = now - (1.0f - GetTransitionProgress())*kTransitionSeconds;
  }
  else {
    transitionStart = now;
  }

  if (toScene) {
    mode = canFly ? Mode::ToScene : Mode::Scene;
    return;
  }
  // Whatever was going on in the view ends with it
  navigation = Navigation::None;
  mode = canFly ? Mode::ToCanvas : Mode::Canvas;
}

void LsSceneViewport::DrawCanvasInScene(ImDrawList* drawList, float alpha) const {
  LsNodeId active = scene->GetActiveCamera();
  ImTextureID texture = renderer->GetCanvasTexture();
  if (active == kLsInvalidNode || !texture || alpha <= 0.0f) {
    return;
  }
  LsCamera camera = scene->GetCamera(active, GetCanvasAspect());
  ImU32 tint = IM_COL32(255, 255, 255, static_cast<int>(alpha*255.0f + 0.5f));

  // The texture is mapped linearly across each quad, small ones keep that close to perspective
  const int cells = 16;
  glm::vec2 points[cells + 1][cells + 1];
  bool visible[cells + 1][cells + 1];
  for (int row = 0; row <= cells; row++) {
    for (int column = 0; column <= cells; column++) {
      glm::vec2 ndc(-1.0f + 2.0f*column/cells, 1.0f - 2.0f*row/cells);
      visible[row][column] = projection.ToScreen(camera.GetImagePlanePoint(ndc, kCameraDisplayDepth), points[row][column]);
    }
  }
  for (int row = 0; row < cells; row++) {
    for (int column = 0; column < cells; column++) {
      if (!visible[row][column] || !visible[row][column + 1] || !visible[row + 1][column + 1] || !visible[row + 1][column]) {
        continue;
      }
      ImVec2 uv0(float(column)/cells, float(row)/cells);
      ImVec2 uv1(float(column + 1)/cells, float(row + 1)/cells);
      drawList->AddImageQuad(texture,
                             LsToImVec2(points[row][column]), LsToImVec2(points[row][column + 1]),
                             LsToImVec2(points[row + 1][column + 1]), LsToImVec2(points[row + 1][column]),
                             uv0, ImVec2(uv1.x, uv0.y), uv1, ImVec2(uv0.x, uv1.y), tint);
    }
  }
}

void LsSceneViewport::DrawSceneOverCanvas(glm::vec2 origin, glm::vec2 size) {
  projection.origin = origin;
  projection.size = glm::max(size, glm::vec2(1.0f));
  LsCamera camera;
  if (!GetCanvasAlignedCamera(camera)) {
    return;
  }
  projection.camera = camera;
  hovered = kLsInvalidNode;
  UpdatePrimitives();
  glm::vec2 pixels = glm::max(glm::floor(projection.size*LsToVec2(ImGui::GetIO().DisplayFramebufferScale)), glm::vec2(1.0f));
  view->Render(static_cast<uint32_t>(pixels.x), static_cast<uint32_t>(pixels.y), camera, settings);

  // The background list goes under every window but over what the renderer drew, the canvas
  ImDrawList* drawList = ImGui::GetBackgroundDrawList();
  drawList->PushClipRect(LsToImVec2(origin), LsToImVec2(origin + projection.size), true);
  int alpha = static_cast<int>(glm::clamp(sceneOverCanvasOpacity, 0.0f, 1.0f)*255.0f + 0.5f);
  drawList->AddImage(view->GetTexture(), LsToImVec2(origin), LsToImVec2(origin + projection.size),
                     ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32(255, 255, 255, alpha));
  drawList->PopClipRect();
}

void LsSceneViewport::Draw(glm::vec2 origin, glm::vec2 size) {
  if (mode == Mode::Canvas) {
    viewHovered = false;
    if (sceneOverCanvas) {
      DrawSceneOverCanvas(origin, size);
    }
    return;
  }

  // 0 shows the canvas, 1 the scene
  bool transitioning = mode == Mode::ToScene || mode == Mode::ToCanvas;
  float blend = 1.0f;
  if (transitioning) {
    float progress = GetTransitionProgress();
    float eased = progress*progress*(3.0f - 2.0f*progress);
    blend = mode == Mode::ToScene ? eased : 1.0f - eased;
    if (progress >= 1.0f) {
      if (mode == Mode::ToCanvas) {
        mode = Mode::Canvas;
        viewHovered = false;
        return;
      }
      mode = Mode::Scene;
      transitioning = false;
    }
  }
  bool interactive = mode == Mode::Scene;

  size = glm::max(size, glm::vec2(1.0f));
  ImGui::SetNextWindowPos(LsToImVec2(origin));
  ImGui::SetNextWindowSize(LsToImVec2(size));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  // Stays behind floating panels, and Tab belongs to switching back
  ImGui::Begin("##SceneView", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
                                       ImGuiWindowFlags_NoScrollWithMouse);
  ImGui::PopStyleVar(2);

  // One item covers the whole view, so it keeps the mouse while dragging with any button.
  // The toolbar goes on top of it.
  ImGui::SetNextItemAllowOverlap();
  ImGui::InvisibleButton("##View", LsToImVec2(size), ImGuiButtonFlags_MouseButtonLeft |
                                                      ImGuiButtonFlags_MouseButtonRight |
                                                      ImGuiButtonFlags_MouseButtonMiddle);
  viewHovered = ImGui::IsItemHovered();
  // The toolbar overlaps the view, a press counts only when the view itself took it
  viewPressed = ImGui::IsItemActivated();
  bool viewInput = viewHovered && interactive;
  ImGuiIO& io = ImGui::GetIO();
  glm::vec2 mouse = LsToVec2(io.MousePos);

  projection.origin = origin;
  projection.size = size;
  if (interactive) {
    // The camera may have been moved elsewhere, such as in the properties panel
    if (lookThrough && navigation == Navigation::None) {
      SyncOrbitToCamera();
    }
    Navigate(viewInput, size);
    HandleShortcuts(viewInput);
  }

  glm::vec2 frameNdc;
  projection.camera = GetViewCamera(size.x/size.y, frameNdc);
  LsCamera canvasCamera;
  if (transitioning && GetCanvasAlignedCamera(canvasCamera)) {
    projection.camera = InterpolateCamera(canvasCamera, projection.camera, blend);
  }

  bool mouseFree = viewInput && navigation == Navigation::None && !io.KeyAlt && ImGui::IsMousePosValid();
  hovered = mouseFree && !gizmo.IsHovered() && !gizmo.IsActive() ? Pick(mouse) : kLsInvalidNode;

  // Render at the display's pixel density rather than in UI points
  UpdatePrimitives();
  glm::vec2 pixels = glm::max(glm::floor(size*LsToVec2(io.DisplayFramebufferScale)), glm::vec2(1.0f));
  view->Render(static_cast<uint32_t>(pixels.x), static_cast<uint32_t>(pixels.y), projection.camera, settings);

  ImDrawList* drawList = ImGui::GetWindowDrawList();
  drawList->PushClipRect(LsToImVec2(origin), LsToImVec2(origin + size), true);
  drawList->AddImage(view->GetTexture(), LsToImVec2(origin), LsToImVec2(origin + size));

  // The scene fades in from the canvas background while the canvas stays as it was
  if (blend < 1.0f) {
    float cover = 1.0f - glm::smoothstep(0.0f, 0.6f, blend);
    glm::vec3 background = glm::clamp(canvasBackground, 0.0f, 1.0f)*255.0f;
    drawList->AddRectFilled(LsToImVec2(origin), LsToImVec2(origin + size),
                            IM_COL32(int(background.r), int(background.g), int(background.b), int(cover*255.0f)));
  }
  // Looking through the camera the scene is what matters, the canvas would cover all of it
  DrawCanvasInScene(drawList, lookThrough ? 1.0f - blend : 1.0f);
  if (blend > 0.5f) {
    DrawCameras(drawList);
  }

  if (interactive) {
    if (lookThrough) {
      DrawCameraFrame(drawList, frameNdc);
    }

    LsNodeId selected = editor->GetSelected();
    bool lookingThroughSelection = lookThrough && selected == scene->GetActiveCamera();
    if (scene->Find(selected) && selected != scene->GetRoot() && !lookingThroughSelection) {
      glm::mat4 world = scene->GetObjectToWorld(selected).GetMatrix();
      if (gizmo.Manipulate(drawList, projection, gizmoMode, gizmoLocal, mouseFree, world)) {
        SetWorldMatrix(selected, world);
      }
      if (gizmo.IsActive() && (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
        gizmo.Cancel(world);
        SetWorldMatrix(selected, world);
      }
    }

    bool axisClicked = DrawAxisWidget(drawList, glm::vec2(origin.x + size.x, origin.y), viewInput);
    if (axisClicked) {
      leftPressPending = false;
    }

    // A double click moves the 3D cursor, a click that didn't drag selects
    // (clicking empty space deselects)
    if (mouseFree && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !gizmo.IsActive() && !axisClicked) {
      PlaceCursor(mouse);
    }
    if (mouseFree && leftClicked && !gizmo.IsActive()) {
      editor->SetSelected(Pick(mouse));
    }
    DrawCursor(drawList);

    DrawStatus(drawList);
  }
  drawList->PopClipRect();

  if (interactive) {
    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    DrawToolbar();
  }
  ImGui::End();
}
