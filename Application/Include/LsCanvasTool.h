#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <LsScene.h>
#include <LsTransformGizmo.h>
#include <LsViewportProjection.h>

class LsRenderer;
class LsCanvas;
struct ImDrawList;

//-------------------------------------------------------------------------------
// @ LsCanvasTool
//-------------------------------------------------------------------------------
// A temporary SDF primitive placed over the canvas for construction, see TOOL
// in SPECIFICATION.md. It lives in world space like a scene shape, but only
// depth and normal strokes see it: they transfer from it in a smooth union
// with the construction geometry, or from it alone.
//
// Picking a primitive in the Tool panel arms placement, and the next drag on
// the canvas places it the ZBrush way: the press is the centre, on the painted
// depth under it (or the last tool's depth, else the scene, on an empty
// pixel), and the drag length sets the size. It faces the camera. A transform
// gizmo then moves (W), scales (E) and rotates (R) it, and Up and Down push it
// into or pull it out of the painting along the camera ray through its centre.
//
// It is drawn as a grey outline of its silhouette, and the canvas draws where
// it passes through the painted positions, see LsCanvas::SetTool.
//-------------------------------------------------------------------------------
class LsCanvasTool
{
  enum class Placing { None, WaitingForDepth, Dragging };

  std::shared_ptr<LsRenderer> renderer;
  std::shared_ptr<LsScene> scene;
  std::shared_ptr<LsCanvas> canvas;

  bool placed = false;
  LsShape shape;
  glm::mat4 world = glm::mat4(1.0f);
  // How far the union with the construction geometry blends, in world units
  float blendRadius = 0.05f;
  // Strokes transfer from the tool alone
  bool ignoreScene = false;

  // The next drag on the canvas places a shape of this type
  bool armed = false;
  LsShapeType armedType = LsShapeType::Sphere;
  Placing placing = Placing::None;
  glm::vec2 pressPoint = glm::vec2(0.0f); // in UI points
  // Frames the placement has waited for its depth pick
  int pickFrames = 0;
  bool released = false;
  glm::vec2 releasePoint = glm::vec2(0.0f);
  glm::vec3 placeCenter = glm::vec3(0.0f);
  // View depth of the last placed tool's centre, 0 before any
  float lastViewDepth = 0.0f;

  LsTransformGizmo gizmo;
  LsGizmoMode gizmoMode = LsGizmoMode::Translate;

  // The canvas as it is on screen, through the canvas camera
  LsViewportProjection projection;
  bool hasProjection = false;

  // Silhouette segments in UI points, recomputed when the tool or the view changes
  std::vector<std::pair<glm::vec2, glm::vec2>> outline;
  struct OutlineKey {
    glm::mat4 world;
    glm::mat4 cameraToWorld;
    glm::vec4 view; // origin and size
    glm::vec4 lens; // tan of the half field of view, aspect and lens shift
    glm::vec4 shapeParams;
    uint32_t shapeType;
  } outlineKey = {};
  bool outlineValid = false;

  bool UpdateProjection();
  // Signed world space distance to the tool
  float Distance(glm::vec3 p, const glm::mat4& worldToObject, float distanceScale) const;
  void UpdateOutline();
  void DrawOutline(ImDrawList* drawList) const;
  // Where a press on an empty pixel puts the tool's centre
  glm::vec3 GetFallbackCenter(const LsRay& ray) const;
  // The tool at center, facing the camera, as large as the drag from the press
  void PlaceAt(glm::vec3 center, glm::vec2 dragEnd);
  void UpdatePlacing(bool mouseAvailable);
  // Moves the tool along the camera ray through its centre, positive away from the camera
  void MoveInDepth(float distance);
public:
  LsCanvasTool(std::shared_ptr<LsRenderer> renderer, std::shared_ptr<LsScene> scene, std::shared_ptr<LsCanvas> canvas);

  bool IsPlaced() const { return placed; }
  // The next drag on the canvas places a primitive of this type
  void Arm(LsShapeType type);
  // Removes the tool, or stops placing one
  void Cancel();
  // The tool for LsCanvas::SetTool, false when none is placed
  bool GetPrimitive(LsScenePrimitive& primitive) const;
  // Placing, or a gizmo handle is under the mouse or dragged: a press on the
  // canvas goes to the tool rather than the brush
  bool WantsMouse() const;

  // Places the tool, drives its gizmo and draws its outline over the canvas.
  // mouseAvailable when the canvas has the mouse and the brush isn't painting.
  // Call every frame while the canvas shows, before LsCanvas::Render, so a
  // placement's depth pick is recorded in the same frame.
  void Update(bool mouseAvailable);
  // W, E and R pick the gizmo mode, Up and Down move the tool in depth, Escape
  // stops placing or a gizmo drag
  void HandleShortcuts();
  // Draws the "Tool" panel's contents into the current window
  void DrawPanel();
};
