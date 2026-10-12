#include <LsTransformGizmo.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
#include <LsTransform.h>

static const float kPickRadius = 6.0f;      // in pixels
static const float kMinAxisPixels = 12.0f;  // shorter axes point at the viewer and are hidden
static const float kTranslateSnap = 0.25f;
static const float kRotateSnap = 15.0f;     // in degrees
static const float kScaleSnap = 0.1f;
static const int kRingSegments = 64;
static const float kPlaneHandleNear = 0.2f; // plane squares span this range of the handle length
static const float kPlaneHandleFar = 0.45f;

static const ImU32 kAxisColors[3] = { IM_COL32(230, 75, 75, 255), IM_COL32(110, 190, 60, 255), IM_COL32(70, 130, 230, 255) };
static const ImU32 kHighlightColor = IM_COL32(255, 210, 60, 255);
static const char* kAxisNames[3] = { "X", "Y", "Z" };
// A mode key released sooner than this without moving the mouse is a tap
static const double kTapSeconds = 0.25;
static const float kTapMovement = 3.0f; // in UI points
// Scaling compares distances from the centre on screen, at least this far, in UI points
static const float kMinScaleRadius = 10.0f;
// An axis shorter than this on screen per hundred points of its length points
// at the viewer, dragging up and down moves along it then
static const float kMinAxisScreenFraction = 0.15f;

static float DistanceToSegment(glm::vec2 p, glm::vec2 a, glm::vec2 b) {
  glm::vec2 ab = b - a;
  float lengthSquared = glm::dot(ab, ab);
  float t = lengthSquared > 0.0f ? glm::clamp(glm::dot(p - a, ab)/lengthSquared, 0.0f, 1.0f) : 0.0f;
  return glm::length(p - (a + ab*t));
}

static bool InsideConvexQuad(glm::vec2 p, const glm::vec2 quad[4]) {
  float sign = 0.0f;
  for (int i = 0; i < 4; i++) {
    glm::vec2 edge = quad[(i + 1) % 4] - quad[i];
    glm::vec2 toPoint = p - quad[i];
    float cross = edge.x*toPoint.y - edge.y*toPoint.x;
    if (cross != 0.0f) {
      if (sign != 0.0f && (cross > 0.0f) != (sign > 0.0f)) {
        return false;
      }
      sign = cross;
    }
  }
  return true;
}

// Parameter along the line origin + axis*s closest to the ray
static float ClosestParameterOnAxis(const LsRay& ray, glm::vec3 origin, glm::vec3 axis) {
  glm::vec3 w = origin - ray.origin;
  float b = glm::dot(axis, ray.direction);
  float denominator = 1.0f - b*b;
  if (denominator < 1.0e-6f) {
    return 0.0f;
  }
  return (b*glm::dot(ray.direction, w) - glm::dot(axis, w))/denominator;
}

static bool IntersectPlane(const LsRay& ray, glm::vec3 point, glm::vec3 normal, glm::vec3& hit) {
  float denominator = glm::dot(ray.direction, normal);
  if (std::abs(denominator) < 1.0e-6f) {
    return false;
  }
  float t = glm::dot(point - ray.origin, normal)/denominator;
  if (t < 0.0f) {
    return false;
  }
  hit = ray.At(t);
  return true;
}

static float Snap(float value, float step) {
  return std::round(value/step)*step;
}

static float ScreenAngle(glm::vec2 point, glm::vec2 center) {
  return std::atan2(point.y - center.y, point.x - center.x);
}

void LsTransformGizmo::ComputeFrame(const glm::mat4& world, bool local, LsGizmoMode mode, glm::vec3 frame[3]) const {
  for (int i = 0; i < 3; i++) {
    glm::vec3 axis(0.0f);
    axis[i] = 1.0f;
    // Scaling is along the object's own axes, anything else would shear it
    if (local || mode == LsGizmoMode::Scale) {
      glm::vec3 column(world[i]);
      float length = glm::length(column);
      if (length > 1.0e-8f) {
        axis = column/length;
      }
    }
    frame[i] = axis;
  }
}

LsTransformGizmo::Handle LsTransformGizmo::HitTest(const LsViewportProjection& projection, glm::vec2 mouse, LsGizmoMode mode,
                                                   glm::vec3 origin, const glm::vec3 frame[3], float length) const {
  glm::vec2 center;
  if (!projection.ToScreen(origin, center)) {
    return eNone;
  }

  Handle best = eNone;
  float bestDistance = kPickRadius;
  auto consider = [&](Handle handle, float distance) {
    if (distance < bestDistance) {
      best = handle;
      bestDistance = distance;
    }
  };

  if (mode == LsGizmoMode::Rotate) {
    for (int i = 0; i < 3; i++) {
      glm::vec3 u = frame[(i + 1) % 3];
      glm::vec3 v = frame[(i + 2) % 3];
      glm::vec2 previous;
      bool previousVisible = false;
      for (int segment = 0; segment <= kRingSegments; segment++) {
        float a = 2.0f*3.14159265f*segment/kRingSegments;
        glm::vec2 point;
        bool visible = projection.ToScreen(origin + (u*std::cos(a) + v*std::sin(a))*length, point);
        if (visible && previousVisible) {
          consider(static_cast<Handle>(eAxisX + i), DistanceToSegment(mouse, previous, point));
        }
        previous = point;
        previousVisible = visible;
      }
    }
    return best;
  }

  // The centre wins over everything that overlaps it
  if (glm::length(mouse - center) < kPickRadius + 2.0f) {
    return eCenter;
  }

  for (int i = 0; i < 3; i++) {
    glm::vec2 tip;
    if (projection.ToScreen(origin + frame[i]*length, tip) && glm::length(tip - center) >= kMinAxisPixels) {
      consider(static_cast<Handle>(eAxisX + i), DistanceToSegment(mouse, center, tip));
    }
  }

  // Plane squares move or scale in their plane
  if (best == eNone) {
    for (int normal = 0; normal < 3; normal++) {
      glm::vec3 u = frame[(normal + 1) % 3];
      glm::vec3 v = frame[(normal + 2) % 3];
      const float corners[4][2] = { { kPlaneHandleNear, kPlaneHandleNear }, { kPlaneHandleFar, kPlaneHandleNear },
                                    { kPlaneHandleFar, kPlaneHandleFar }, { kPlaneHandleNear, kPlaneHandleFar } };
      glm::vec2 quad[4];
      bool visible = true;
      for (int c = 0; c < 4 && visible; c++) {
        visible = projection.ToScreen(origin + (u*corners[c][0] + v*corners[c][1])*length, quad[c]);
      }
      if (visible && InsideConvexQuad(mouse, quad)) {
        return static_cast<Handle>(ePlaneYZ + normal);
      }
    }
  }
  return best;
}

void LsTransformGizmo::Draw(ImDrawList* drawList, const LsViewportProjection& projection, LsGizmoMode mode,
                            glm::vec3 origin, const glm::vec3 frame[3], float length) const {
  glm::vec2 center;
  if (!projection.ToScreen(origin, center)) {
    return;
  }
  Handle shown = active != eNone ? active : hovered;
  auto colorOf = [&](Handle handle, int axis) {
    return handle == shown ? kHighlightColor : kAxisColors[axis];
  };

  if (mode == LsGizmoMode::Rotate) {
    glm::vec3 toCamera = projection.camera.GetPosition() - origin;
    for (int i = 0; i < 3; i++) {
      glm::vec3 u = frame[(i + 1) % 3];
      glm::vec3 v = frame[(i + 2) % 3];
      ImU32 color = colorOf(static_cast<Handle>(eAxisX + i), i);
      // The half of the ring facing away is drawn faint, so the rings read as a sphere
      ImU32 backColor = (color & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 70);
      glm::vec2 previous;
      bool previousVisible = false;
      for (int segment = 0; segment <= kRingSegments; segment++) {
        float a = 2.0f*3.14159265f*segment/kRingSegments;
        glm::vec3 offset = u*std::cos(a) + v*std::sin(a);
        glm::vec2 point;
        bool visible = projection.ToScreen(origin + offset*length, point);
        if (visible && previousVisible) {
          bool front = glm::dot(offset, toCamera) >= 0.0f;
          drawList->AddLine(LsToImVec2(previous), LsToImVec2(point), front ? color : backColor, front ? 2.5f : 1.5f);
        }
        previous = point;
        previousVisible = visible;
      }
    }
    if (active != eNone) {
      drawList->AddLine(LsToImVec2(center), ImGui::GetIO().MousePos, kHighlightColor, 1.0f);
    }
    drawList->AddCircleFilled(LsToImVec2(center), 3.0f, IM_COL32(255, 255, 255, 200));
    return;
  }

  {
    for (int normal = 0; normal < 3; normal++) {
      glm::vec3 u = frame[(normal + 1) % 3];
      glm::vec3 v = frame[(normal + 2) % 3];
      const float corners[4][2] = { { kPlaneHandleNear, kPlaneHandleNear }, { kPlaneHandleFar, kPlaneHandleNear },
                                    { kPlaneHandleFar, kPlaneHandleFar }, { kPlaneHandleNear, kPlaneHandleFar } };
      ImVec2 quad[4];
      bool visible = true;
      for (int c = 0; c < 4 && visible; c++) {
        glm::vec2 point;
        visible = projection.ToScreen(origin + (u*corners[c][0] + v*corners[c][1])*length, point);
        quad[c] = LsToImVec2(point);
      }
      if (visible) {
        ImU32 color = colorOf(static_cast<Handle>(ePlaneYZ + normal), normal);
        drawList->AddConvexPolyFilled(quad, 4, (color & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 90));
        drawList->AddPolyline(quad, 4, color, ImDrawFlags_Closed, 1.0f);
      }
    }
  }

  for (int i = 0; i < 3; i++) {
    glm::vec2 tip;
    if (!projection.ToScreen(origin + frame[i]*length, tip) || glm::length(tip - center) < kMinAxisPixels) {
      continue;
    }
    ImU32 color = colorOf(static_cast<Handle>(eAxisX + i), i);
    drawList->AddLine(LsToImVec2(center), LsToImVec2(tip), color, 2.5f);
    glm::vec2 direction = glm::normalize(tip - center);
    glm::vec2 side(-direction.y, direction.x);
    if (mode == LsGizmoMode::Translate) {
      drawList->AddTriangleFilled(LsToImVec2(tip + direction*12.0f), LsToImVec2(tip + side*5.0f), LsToImVec2(tip - side*5.0f), color);
    }
    else {
      drawList->AddRectFilled(LsToImVec2(tip - glm::vec2(5.0f)), LsToImVec2(tip + glm::vec2(5.0f)), color);
    }
  }

  ImU32 centerColor = shown == eCenter ? kHighlightColor : IM_COL32(230, 230, 230, 220);
  if (mode == LsGizmoMode::Scale) {
    drawList->AddRectFilled(LsToImVec2(center - glm::vec2(6.0f)), LsToImVec2(center + glm::vec2(6.0f)), centerColor);
  }
  else {
    drawList->AddCircle(LsToImVec2(center), kPickRadius + 2.0f, centerColor, 0, 2.0f);
  }
}

bool LsTransformGizmo::Drag(const LsViewportProjection& projection, glm::vec2 mouse, LsGizmoMode mode, bool snap, glm::mat4& world) {
  LsRay ray = projection.GetRay(mouse);
  glm::mat4 change(1.0f);
  // Applied in object space instead of world space
  bool objectSpace = false;

  if (mode == LsGizmoMode::Translate) {
    glm::vec3 delta(0.0f);
    if (active <= eAxisZ) {
      int axis = active - eAxisX;
      float distance = ClosestParameterOnAxis(ray, startOrigin, axes[axis]) - startParameter;
      if (snap) {
        distance = Snap(distance, kTranslateSnap);
      }
      delta = axes[axis]*distance;
      snprintf(readout, sizeof(readout), "%s %+.3f", kAxisNames[axis], distance);
    }
    else {
      glm::vec3 normal = active == eCenter ? projection.camera.GetForward() : axes[active - ePlaneYZ];
      glm::vec3 point;
      if (!IntersectPlane(ray, startOrigin, normal, point)) {
        return false;
      }
      delta = point - startPoint;
      if (snap) {
        // Along the gizmo's axes, which are the world axes unless it is in local mode
        glm::vec3 snapped(0.0f);
        for (int i = 0; i < 3; i++) {
          snapped += axes[i]*Snap(glm::dot(delta, axes[i]), kTranslateSnap);
        }
        delta = snapped;
      }
      snprintf(readout, sizeof(readout), "%+.3f %+.3f %+.3f", delta.x, delta.y, delta.z);
    }
    change = glm::translate(glm::mat4(1.0f), delta);
  }
  else if (mode == LsGizmoMode::Rotate) {
    glm::vec2 center;
    if (!projection.ToScreen(startOrigin, center)) {
      return false;
    }
    // Unwrapped, so dragging round several times keeps turning
    float screenAngle = ScreenAngle(mouse, center);
    float step = screenAngle - previousScreenAngle;
    step -= 2.0f*3.14159265f*std::round(step/(2.0f*3.14159265f));
    angle += step;
    previousScreenAngle = screenAngle;

    glm::vec3 axis = axes[active - eAxisX];
    // Screen y points down, so a positive (counter-clockwise) turn about an axis
    // pointing at the viewer decreases the screen angle
    float degrees = glm::degrees(glm::dot(axis, projection.camera.GetForward()) < 0.0f ? -angle : angle);
    if (snap) {
      degrees = Snap(degrees, kRotateSnap);
    }
    snprintf(readout, sizeof(readout), "%s %+.1f\xC2\xB0", kAxisNames[active - eAxisX], degrees);
    change = glm::translate(glm::mat4(1.0f), startOrigin)*
             glm::rotate(glm::mat4(1.0f), glm::radians(degrees), axis)*
             glm::translate(glm::mat4(1.0f), -startOrigin);
  }
  else {
    glm::vec3 scale(1.0f);
    if (active == eCenter) {
      float factor = 1.0f + ((mouse.x - startMouse.x) - (mouse.y - startMouse.y))/sizeInPixels;
      if (snap) {
        factor = Snap(factor, kScaleSnap);
      }
      scale = glm::vec3(factor);
      snprintf(readout, sizeof(readout), "x%.3f", factor);
    }
    else if (active >= ePlaneYZ) {
      // Both axes of the plane by how far along its diagonal the mouse is
      // against where the drag started
      int normal = active - ePlaneYZ;
      int u = (normal + 1) % 3;
      int v = (normal + 2) % 3;
      glm::vec3 point;
      if (!IntersectPlane(ray, startOrigin, axes[normal], point)) {
        return false;
      }
      glm::vec3 diagonal = glm::normalize(axes[u] + axes[v]);
      float from = glm::dot(startPoint - startOrigin, diagonal);
      if (std::abs(from) < 1.0e-6f) {
        return false;
      }
      float factor = glm::dot(point - startOrigin, diagonal)/from;
      if (snap) {
        factor = Snap(factor, kScaleSnap);
      }
      scale[u] = factor;
      scale[v] = factor;
      snprintf(readout, sizeof(readout), "%s%s x%.3f", kAxisNames[u], kAxisNames[v], factor);
    }
    else {
      int axis = active - eAxisX;
      float factor = 1.0f + (ClosestParameterOnAxis(ray, startOrigin, axes[axis]) - startParameter)/handleLength;
      if (snap) {
        factor = Snap(factor, kScaleSnap);
      }
      scale[axis] = factor;
      snprintf(readout, sizeof(readout), "%s x%.3f", kAxisNames[axis], factor);
    }
    // Collapsing an axis completely would make the transform singular
    for (int i = 0; i < 3; i++) {
      if (std::abs(scale[i]) < 1.0e-3f) {
        scale[i] = 1.0e-3f;
      }
    }
    change = glm::scale(glm::mat4(1.0f), scale);
    objectSpace = true;
  }

  world = objectSpace ? startWorld*change : change*startWorld;
  return true;
}

bool LsTransformGizmo::Manipulate(ImDrawList* drawList, const LsViewportProjection& projection, LsGizmoMode mode,
                                  bool local, bool mouseAvailable, glm::mat4& world) {
  glm::vec3 origin(world[3]);
  glm::vec3 frame[3];
  float length;
  if (active != eNone) {
    // The handles stay as they were when the drag started
    for (int i = 0; i < 3; i++) {
      frame[i] = axes[i];
    }
    length = handleLength;
  }
  else {
    ComputeFrame(world, local, mode, frame);
    length = projection.GetPixelSize(origin)*sizeInPixels;
  }

  if (projection.camera.GetDepth(origin) <= 0.0f) {
    hovered = eNone;
    active = eNone;
    return false;
  }

  ImGuiIO& io = ImGui::GetIO();
  glm::vec2 mouse = LsToVec2(io.MousePos);
  bool changed = false;
  if (active == eNone) {
    hovered = mouseAvailable ? HitTest(projection, mouse, mode, origin, frame, length) : eNone;
    if (hovered != eNone && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      active = hovered;
      startWorld = world;
      startOrigin = origin;
      for (int i = 0; i < 3; i++) {
        axes[i] = frame[i];
      }
      handleLength = length;
      startMouse = mouse;
      readout[0] = '\0';

      LsRay ray = projection.GetRay(mouse);
      if (active <= eAxisZ) {
        startParameter = ClosestParameterOnAxis(ray, origin, axes[active - eAxisX]);
      }
      glm::vec3 normal = active == eCenter ? projection.camera.GetForward() :
                         active >= ePlaneYZ ? axes[active - ePlaneYZ] : glm::vec3(0.0f, 1.0f, 0.0f);
      if (!IntersectPlane(ray, origin, normal, startPoint)) {
        startPoint = origin;
      }
      glm::vec2 center;
      projection.ToScreen(origin, center);
      previousScreenAngle = ScreenAngle(mouse, center);
      angle = 0.0f;
    }
  }
  else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    changed = Drag(projection, mouse, mode, io.KeyCtrl, world);
  }
  else {
    active = eNone;
  }

  Draw(drawList, projection, mode, glm::vec3(world[3]), frame, length);
  if (active != eNone && readout[0]) {
    drawList->AddText(ImVec2(io.MousePos.x + 16.0f, io.MousePos.y + 8.0f), IM_COL32(255, 255, 255, 230), readout);
  }
  return changed;
}

void LsTransformGizmo::Cancel(glm::mat4& world) {
  if (active != eNone) {
    world = startWorld;
    active = eNone;
  }
}

// Where a world direction from the centre goes on screen, per hundred UI points
// of its length at the centre's depth, and the world length that is
static glm::vec2 ScreenDirection(const LsViewportProjection& projection, glm::vec3 center, glm::vec3 direction,
                                 float& worldLength) {
  worldLength = projection.GetPixelSize(center)*100.0f;
  glm::vec2 a, b;
  if (!projection.SegmentToScreen(center, center + direction*worldLength, a, b)) {
    return glm::vec2(0.0f);
  }
  return b - a;
}

glm::mat4 LsTransformKeys::Transform(const LsViewportProjection& projection, LsGizmoMode mode, const glm::vec3* axis,
                                     const glm::mat4& start, glm::vec2 startMouse, glm::vec2 mouse) {
  const LsCamera& camera = projection.camera;
  glm::vec3 center(start[3]);
  glm::vec2 screenCenter;
  if (!projection.ToScreen(center, screenCenter)) {
    return start;
  }
  glm::vec3 toCamera = camera.GetPosition() - center;
  float cameraDistance = glm::length(toCamera);
  toCamera = cameraDistance > 1.0e-6f ? toCamera/cameraDistance : -camera.GetForward();
  // Screen y goes down
  glm::vec2 delta = mouse - startMouse;
  glm::mat4 result = start;

  if (mode == LsGizmoMode::Translate) {
    float pixelSize = projection.GetPixelSize(center);
    glm::vec3 offset;
    if (!axis) {
      offset = (camera.GetRight()*delta.x - camera.GetUp()*delta.y)*pixelSize;
    }
    else {
      float worldLength;
      glm::vec2 direction = ScreenDirection(projection, center, *axis, worldLength);
      if (glm::length(direction) < 100.0f*kMinAxisScreenFraction) {
        // Seen end on: dragging up moves towards the camera
        float towards = glm::dot(*axis, toCamera) >= 0.0f ? 1.0f : -1.0f;
        offset = *axis*towards*-delta.y*pixelSize;
      }
      else {
        offset = *axis*glm::dot(delta, direction)/glm::dot(direction, direction)*worldLength;
      }
    }
    // Not through the camera
    if (glm::dot(center + offset - camera.GetPosition(), camera.GetForward()) < 1.0e-3f) {
      return start;
    }
    result[3] = glm::vec4(center + offset, 1.0f);
    return result;
  }

  if (mode == LsGizmoMode::Scale) {
    // As far from the centre as the mouse is now against where it started
    float from = std::max(glm::length(startMouse - screenCenter), kMinScaleRadius);
    float factor = std::max(glm::length(mouse - screenCenter)/from, 0.01f);
    if (!axis) {
      for (int column = 0; column < 3; column++) {
        result[column] *= factor;
      }
      return result;
    }
    // Along the object's own axis closest to the given one, which keeps it free
    // of shear however it is turned
    int closest = 0;
    float best = -1.0f;
    for (int column = 0; column < 3; column++) {
      glm::vec3 own(start[column]);
      float length = glm::length(own);
      float alignment = length > 1.0e-8f ? std::abs(glm::dot(own/length, *axis)) : 0.0f;
      if (alignment > best) {
        best = alignment;
        closest = column;
      }
    }
    result[closest] *= factor;
    return result;
  }

  // Turns as the mouse goes round the centre, about the view axis or the given
  // one, the way it faces deciding which way round
  glm::vec2 a = startMouse - screenCenter;
  glm::vec2 b = mouse - screenCenter;
  if (glm::length(a) < 1.0f || glm::length(b) < 1.0f) {
    return start;
  }
  // Screen y goes down, so a clockwise turn on screen is a positive screen angle,
  // a clockwise turn about an axis towards the viewer a negative one
  float angle = -std::atan2(a.x*b.y - a.y*b.x, glm::dot(a, b));
  glm::vec3 rotationAxis = toCamera;
  if (axis) {
    rotationAxis = glm::dot(*axis, toCamera) >= 0.0f ? *axis : -*axis;
  }
  result = LsTransform::Rotate(angle, rotationAxis).GetMatrix()*start;
  result[3] = glm::vec4(center, 1.0f);
  return result;
}

void LsTransformKeys::HandleKeys(const glm::mat4& world, bool canStart) {
  ImGuiIO& io = ImGui::GetIO();
  glm::vec2 mouse = LsToVec2(io.MousePos);
  const struct { ImGuiKey key; LsGizmoMode mode; } keys[] = {
    { ImGuiKey_W, LsGizmoMode::Translate }, { ImGuiKey_E, LsGizmoMode::Scale }, { ImGuiKey_R, LsGizmoMode::Rotate },
  };
  bool modifiers = io.KeyCtrl || io.KeyAlt;
  if (keyHold.key == ImGuiKey_None && !axisDragging && canStart && !modifiers) {
    for (const auto& entry : keys) {
      if (ImGui::IsKeyPressed(entry.key, false)) {
        keyHold.key = entry.key;
        keyHold.mode = entry.mode;
        keyHold.pressTime = ImGui::GetTime();
        keyHold.startMouse = mouse;
        keyHold.startWorld = world;
        keyHold.holding = false;
        break;
      }
    }
  }
  // Locks the mode to an axis, or the same one again back to the gizmo
  const ImGuiKey axisKeys[3] = { ImGuiKey_X, ImGuiKey_Y, ImGuiKey_Z };
  for (int axis = 0; axis < 3; axis++) {
    if (ImGui::IsKeyPressed(axisKeys[axis], false) && !axisDragging && !keyHold.holding && canStart && !modifiers) {
      axisLock = axisLock == axis ? -1 : axis;
    }
  }
  if (keyHold.key == ImGuiKey_None) {
    return;
  }
  if (ImGui::IsKeyDown(keyHold.key)) {
    // Held long enough, or the mouse moved while it was down
    if (!keyHold.holding && (ImGui::GetTime() - keyHold.pressTime > kTapSeconds ||
                             glm::length(mouse - keyHold.startMouse) > kTapMovement)) {
      keyHold.holding = true;
    }
    return;
  }
  // Released: a tap picks the mode, its key again goes back to the gizmo
  if (!keyHold.holding) {
    SetMode(keyHold.mode);
  }
  keyHold.key = ImGuiKey_None;
  keyHold.holding = false;
}

bool LsTransformKeys::Cancel(glm::mat4& world) {
  if (keyHold.holding) {
    world = keyHold.startWorld;
    keyHold.holding = false;
    keyHold.key = ImGuiKey_None;
    return true;
  }
  if (axisDragging) {
    world = dragStartWorld;
    axisDragging = false;
    return true;
  }
  return false;
}

bool LsTransformKeys::Update(const LsViewportProjection& projection, const glm::vec3 axes[3], bool mouseAvailable,
                             glm::mat4& world) {
  glm::vec2 mouse = LsToVec2(ImGui::GetIO().MousePos);
  glm::mat4 before = world;
  if (keyHold.holding) {
    world = Transform(projection, keyHold.mode, nullptr, keyHold.startWorld, keyHold.startMouse, mouse);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      Cancel(world);
    }
  }
  else if (axisLock >= 0) {
    if (!axisDragging && mouseAvailable && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      axisDragging = true;
      dragStart = mouse;
      dragStartWorld = world;
    }
    if (axisDragging) {
      world = Transform(projection, mode, &axes[axisLock], dragStartWorld, dragStart, mouse);
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        Cancel(world);
      }
      else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        axisDragging = false;
      }
    }
  }
  else {
    axisDragging = false;
  }
  return world != before;
}

void LsTransformKeys::DrawAxisLock(ImDrawList* drawList, const LsViewportProjection& projection, const glm::mat4& world,
                                   const glm::vec3 axes[3]) const {
  if (axisLock < 0 || keyHold.holding) {
    return;
  }
  glm::vec3 center(world[3]);
  glm::vec2 screenCenter;
  if (!projection.ToScreen(center, screenCenter)) {
    return;
  }
  ImU32 color = kAxisColors[axisLock];
  float worldLength;
  glm::vec2 direction = ScreenDirection(projection, center, axes[axisLock], worldLength);
  if (glm::length(direction) < 100.0f*kMinAxisScreenFraction) {
    // Seen end on, the axis is a ring round the centre
    drawList->AddCircle(LsToImVec2(screenCenter), 8.0f, color, 0, 2.0f);
    drawList->AddCircleFilled(LsToImVec2(screenCenter), 2.5f, color);
  }
  else {
    // A line through the centre, far enough to cross the view
    float reach = glm::length(projection.size)/glm::length(direction)*worldLength;
    projection.DrawLine(drawList, center - axes[axisLock]*reach, center + axes[axisLock]*reach, color, 1.5f);
  }
  const char* modeName = mode == LsGizmoMode::Translate ? "Move" : mode == LsGizmoMode::Scale ? "Scale" : "Rotate";
  char label[32];
  snprintf(label, sizeof(label), "%s %s", modeName, kAxisNames[axisLock]);
  drawList->AddText(LsToImVec2(screenCenter + glm::vec2(12.0f, 10.0f)), color, label);
}
