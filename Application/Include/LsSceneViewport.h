#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <LsScene.h>
#include <LsSceneView.h>
#include <LsTransformGizmo.h>
#include <LsViewportProjection.h>

class LsRenderer;
class LsSceneEditor;
struct ImDrawList;

//-------------------------------------------------------------------------------
// @ LsSceneViewport
//-------------------------------------------------------------------------------
// The "3D View" window: shows the scene from a free editor camera, selects
// objects by clicking them and moves them with a transform gizmo. Camera nodes
// are drawn as frustums ending in the canvas they paint, and the view can look
// through the canvas camera, optionally moving it along when navigating, which
// is the quickest way to frame a painting.
//
// Navigation follows the common DCC layout: alt+left drag orbits, middle drag
// pans, alt+right drag and the wheel zoom, right drag looks around with WASD/QE
// flying, F frames the selection.
//-------------------------------------------------------------------------------
class LsSceneViewport
{
  enum class Navigation { None, Orbit, Pan, Zoom, Fly };

  std::shared_ptr<LsRenderer> renderer;
  std::shared_ptr<LsScene> scene;
  LsSceneEditor* editor;
  std::unique_ptr<LsSceneView> view;
  LsSceneViewSettings settings;

  LsTransformGizmo gizmo;
  LsGizmoMode gizmoMode = LsGizmoMode::Translate;
  bool gizmoLocal = false;

  // Orbit camera: looks at target from distance, yaw about +Y and pitch above the horizon
  glm::vec3 target = glm::vec3(0.0f, 0.6f, 1.5f);
  float distance = 9.0f;
  float yaw = 55.0f;   // in degrees
  float pitch = 22.0f; // in degrees
  float fov = 50.0f;   // vertical, in degrees
  Navigation navigation = Navigation::None;
  float flySpeedScale = 1.0f;

  // The orbit camera from before looking through, to return to
  struct {
    glm::vec3 target;
    float distance, yaw, pitch;
  } savedOrbit = {};

  // Looking through the camera that paints the canvas
  bool lookThrough = false;
  // While looking through, navigating moves that camera
  bool lockCameraToView = false;

  // Scene as last uploaded, with the selection and hover flags set
  std::vector<LsScenePrimitive> primitives;
  uint64_t primitivesRevision = UINT64_MAX;
  LsNodeId drawnSelection = kLsInvalidNode;
  LsNodeId drawnHover = kLsInvalidNode;
  LsNodeId hovered = kLsInvalidNode;

  LsViewportProjection projection;

  glm::vec3 GetOrbitPosition() const;
  LsCamera GetOrbitCamera(float aspect) const;
  void SetOrbitFromCamera(const LsCamera& camera);
  // The orbit camera takes the canvas camera's pose, pivoting on what it looks at
  void SyncOrbitToCamera();
  float GetCanvasAspect() const;
  // What the view shows: the orbit camera, or the canvas camera fitted inside the view
  LsCamera GetViewCamera(float viewAspect, glm::vec2& frameNdc) const;

  // Returns true when the view moved
  bool Navigate(bool viewHovered, glm::vec2 viewSize);
  void HandleShortcuts(bool viewHovered);
  void UpdatePrimitives();
  // Camera nodes that are not hidden, apart from the one being looked through
  std::vector<LsNodeId> GetDrawnCameras() const;
  LsNodeId PickCamera(glm::vec2 mouse) const;
  LsNodeId Pick(glm::vec2 mouse) const;

  void DrawToolbar();
  void DrawAddMenu();
  void DrawViewMenu();
  void DrawCameras(ImDrawList* drawList) const;
  void DrawCameraFrame(ImDrawList* drawList, glm::vec2 frameNdc) const;
  // Returns true when a click on it changed the view
  bool DrawAxisWidget(ImDrawList* drawList, glm::vec2 corner, bool viewHovered);
  void DrawStatus(ImDrawList* drawList) const;

  // Places a node so it ends up at world, whatever its parents are
  void SetWorldMatrix(LsNodeId id, const glm::mat4& world);
  void FrameSelection();
  void AlignCameraToView();
  // Leaving either returns to the view from before, or carries on from the camera
  void SetLookThrough(bool enabled, bool restoreView = true);
  void ViewAlongAxis(int axis, bool negative);
public:
  LsSceneViewport(std::shared_ptr<LsRenderer> renderer, std::shared_ptr<LsScene> scene, LsSceneEditor* editor);
  ~LsSceneViewport();
  // Draws the "3D View" window, call between BeginFrame and EndFrame
  void Draw();
  // Frames must keep coming even without input, such as while flying with keys held
  bool IsAnimating() const { return navigation == Navigation::Fly; }
};
