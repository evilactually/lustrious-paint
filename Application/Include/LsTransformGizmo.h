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
// axis or the centre in scale mode scales it along its own axes. Holding ctrl
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
    ePlaneYZ, ePlaneZX, ePlaneXY, // translate in the plane, indexed by its normal
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
  float startAngle = 0.0f;
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
