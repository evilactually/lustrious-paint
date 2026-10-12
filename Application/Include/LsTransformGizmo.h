#pragma once

#include <glm/glm.hpp>
#include <imgui.h>
#include <LsViewportProjection.h>

enum class LsGizmoMode { Translate, Rotate, Scale };

//-------------------------------------------------------------------------------
// @ LsTransformGizmo
//-------------------------------------------------------------------------------
// Move, rotate and scale handles drawn over a 3D view with the ImGui draw list.
// It edits a world space matrix: dragging an axis, a plane square or the centre
// moves the object, dragging a ring rotates it about that axis, and dragging an
// axis, a plane square or the centre in scale mode scales it along its own axes. Holding ctrl
// snaps to steps.
//
// Changes are always computed from the matrix the drag started with, so
// repeated updates don't accumulate rounding errors.
//-------------------------------------------------------------------------------
class LsTransformGizmo
{
  enum Handle
  {
    eNone,
    eAxisX, eAxisY, eAxisZ,    // translate or scale along, rotate about
    ePlaneYZ, ePlaneZX, ePlaneXY, // translate or scale in the plane, indexed by its normal
    eCenter,                   // translate in the view plane or scale uniformly
  };

  Handle hovered = eNone;
  Handle active = eNone;

  // State captured when a drag starts
  glm::mat4 startWorld = glm::mat4(1.0f);
  glm::vec3 startOrigin = glm::vec3(0.0f);
  glm::vec3 axes[3];
  float handleLength = 1.0f;
  glm::vec2 startMouse = glm::vec2(0.0f);
  glm::vec3 startPoint = glm::vec3(0.0f); // where the mouse ray met the handle
  float startParameter = 0.0f;            // along the axis for axis drags
  float angle = 0.0f;                     // accumulated, so it can go past a full turn
  float previousScreenAngle = 0.0f;

  // What the current drag shows next to the cursor
  char readout[64] = {};

  void ComputeFrame(const glm::mat4& world, bool local, LsGizmoMode mode, glm::vec3 frame[3]) const;
  Handle HitTest(const LsViewportProjection& projection, glm::vec2 mouse, LsGizmoMode mode,
                 glm::vec3 origin, const glm::vec3 frame[3], float length) const;
  void Draw(ImDrawList* drawList, const LsViewportProjection& projection, LsGizmoMode mode,
            glm::vec3 origin, const glm::vec3 frame[3], float length) const;
  bool Drag(const LsViewportProjection& projection, glm::vec2 mouse, LsGizmoMode mode, bool snap, glm::mat4& world);
public:
  // Size of the handles on screen
  float sizeInPixels = 90.0f;

  // Draws the gizmo for an object at world and lets the mouse drag it when
  // mouseAvailable (the view is hovered and nothing else uses the mouse).
  // Returns true on frames where it changed world.
  bool Manipulate(ImDrawList* drawList, const LsViewportProjection& projection, LsGizmoMode mode,
                  bool local, bool mouseAvailable, glm::mat4& world);

  // A handle is under the mouse, a click should go to the gizmo
  bool IsHovered() const { return hovered != eNone; }
  bool IsActive() const { return active != eNone; }
  // Ends a drag, restoring where it started from
  void Cancel(glm::mat4& world);
};

//-------------------------------------------------------------------------------
// @ LsTransformKeys
//-------------------------------------------------------------------------------
// The keys that go with the gizmo, the same wherever something is transformed.
// W, E and R pick moving, scaling and rotating. Held instead of tapped, they
// move in screen space, scale uniformly and rotate about the view axis with
// the mouse until released, without a click. X, Y and Z lock the mode to one
// of three axes the caller gives, in place of the gizmo: a drag anywhere then
// moves along it, scales along the object's own axis closest to it, or rotates
// about it as the mouse goes round the centre. The mode's key again, or the
// axis key again, goes back to the gizmo. Escape cancels a hold or a drag.
//
// Like the gizmo, every change is computed from the matrix the hold or drag
// started with.
//-------------------------------------------------------------------------------
class LsTransformKeys
{
  LsGizmoMode mode = LsGizmoMode::Translate;
  // Index of the locked axis, -1 for the gizmo
  int axisLock = -1;
  bool axisDragging = false;
  glm::vec2 dragStart = glm::vec2(0.0f);
  glm::mat4 dragStartWorld = glm::mat4(1.0f);
  // A mode key held down: tapped it picks the mode, held it transforms
  struct {
    ImGuiKey key = ImGuiKey_None;
    LsGizmoMode mode = LsGizmoMode::Translate;
    double pressTime = 0.0;
    glm::vec2 startMouse = glm::vec2(0.0f);
    glm::mat4 startWorld = glm::mat4(1.0f);
    bool holding = false;
  } keyHold;
public:
  LsGizmoMode GetMode() const { return mode; }
  // Also goes back to the gizmo
  void SetMode(LsGizmoMode newMode) { mode = newMode; axisLock = -1; }
  int GetAxisLock() const { return axisLock; }
  void SetAxisLock(int axis) { axisLock = axis; }
  // The gizmo is shown, rather than a locked axis or nothing during a hold
  bool ShowsGizmo() const { return axisLock < 0 && !keyHold.holding; }
  bool IsTransforming() const { return keyHold.holding || axisDragging; }
  // A press in the view goes to the keys rather than selecting or navigating
  bool ClaimsMouse() const { return axisLock >= 0 || IsTransforming(); }

  // Reads the mode and axis keys, call while the keys belong to the view.
  // world is what a hold starts from, canStart false while the gizmo drags.
  void HandleKeys(const glm::mat4& world, bool canStart);
  // Ends a hold or a drag, restoring where it started from. False when there
  // was none, so Escape can do something else.
  bool Cancel(glm::mat4& world);
  // Applies a hold, or a drag along the locked axis, which a left press starts
  // when mouseAvailable. Returns true on frames where it changed world.
  bool Update(const LsViewportProjection& projection, const glm::vec3 axes[3], bool mouseAvailable, glm::mat4& world);
  // The locked axis through the object's centre, and what a drag does
  void DrawAxisLock(ImDrawList* drawList, const LsViewportProjection& projection, const glm::mat4& world,
                    const glm::vec3 axes[3]) const;

  // world transformed from start by the mouse moving from startMouse to mouse,
  // along axis, or in screen space (uniformly for scaling) without one
  static glm::mat4 Transform(const LsViewportProjection& projection, LsGizmoMode mode, const glm::vec3* axis,
                             const glm::mat4& start, glm::vec2 startMouse, glm::vec2 mouse);
};
