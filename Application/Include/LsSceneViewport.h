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

// How the scene is shown over the canvas
enum class LsSceneOverlay
{
  Shaded,
  Outlines,
  ShadedWithOutlines,
};

//-------------------------------------------------------------------------------
// @ LsSceneViewport
//-------------------------------------------------------------------------------
// The 3D view of the scene, which shares the canvas area with the canvas.
// Switching flies the view out of the canvas camera so the canvas becomes the
// plane at the end of its frustum, or back in until that plane covers the
// canvas exactly where it was on screen. While painting, the scene can also be
// shown over the canvas through the canvas camera, without leaving the canvas.
//
// It shows the scene from a free editor camera, selects objects by clicking
// them and moves them with a transform gizmo. Camera nodes are drawn as
// frustums ending in the canvas they paint, and the view can look through the
// canvas camera, optionally moving it along when navigating, which is the
// quickest way to frame a painting.
//
// Navigation follows the common DCC layout: left drag (or alt+left drag) orbits,
// middle drag pans, alt+right drag and the wheel zoom, right drag looks around
// with WASD/QE flying, F frames the selection. As in Blender a double click puts
// the 3D cursor on the surface under the mouse, new objects are created there,
// and Alt+Home centres the view on it. W, E and R transform the selection as
// they do the canvas tool, see LsTransformKeys.
//-------------------------------------------------------------------------------
class LsSceneViewport
{
  enum class Navigation { None, Orbit, Pan, Zoom, Fly };
  enum class Mode { Canvas, ToScene, Scene, ToCanvas };

  std::shared_ptr<LsRenderer> renderer;
  std::shared_ptr<LsScene> scene;
  LsSceneEditor* editor;
  std::unique_ptr<LsSceneView> view;
  LsSceneViewSettings settings;

  LsTransformGizmo gizmo;
  // W, E and R, and X, Y and Z locking the world axes, or the selection's own
  // when the gizmo is local
  LsTransformKeys transformKeys;
  bool gizmoLocal = false;

  // Orbit camera: looks at target from distance, yaw about +Y and pitch above the horizon
  glm::vec3 target = glm::vec3(0.0f, 1.0f, 0.5f);
  float distance = 8.0f;
  float yaw = 35.0f;   // in degrees
  float pitch = 20.0f; // in degrees
  float fov = 50.0f;   // vertical, in degrees
  Navigation navigation = Navigation::None;
  float flySpeedScale = 1.0f;

  // Where new objects go, placed with a double click
  glm::vec3 cursor = glm::vec3(0.0f);
  // A left press that is neither a drag yet nor a click
  bool leftPressPending = false;
  // A left press was released without dragging, which selects
  bool leftClicked = false;

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

  // Surfaces are picked by reading back what the view rendered at a pixel, the
  // results arrive a frame later. 0 when nothing is waiting.
  LsPickTicket hoverTicket = 0;
  LsNodeId hoveredSurface = kLsInvalidNode;
  // The last hover pick, for the debug readout
  LsSceneHit hoverHit;
  bool hoverHitValid = false;
  LsPickTicket selectTicket = 0;
  LsPickTicket cursorTicket = 0;
  LsRay cursorRay = {};
  LsPickTicket orbitTicket = 0;

  LsViewportProjection projection;
  bool viewHovered = false;
  bool viewPressed = false;

  Mode mode = Mode::Canvas;
  double transitionStart = 0.0;
  // What the canvas area shows in canvas mode around the canvas
  glm::vec3 canvasBackground = glm::vec3(0.0f);
  // The scene over the canvas while painting, so it shows what is painted over
  bool sceneOverCanvas = false;
  float sceneOverCanvasOpacity = 0.5f;
  LsSceneOverlay sceneOverlay = LsSceneOverlay::Outlines;
  // The canvas area as last drawn in canvas mode, in UI points
  glm::vec2 canvasAreaOrigin = glm::vec2(0.0f);
  glm::vec2 canvasAreaSize = glm::vec2(1.0f);
  // The selection's gizmo is shown over the canvas this frame
  bool transformOverCanvas = false;

  glm::vec3 GetOrbitPosition() const;
  LsCamera GetOrbitCamera(float aspect) const;
  void SetOrbitFromCamera(const LsCamera& camera);
  // The orbit camera takes the canvas camera's pose, pivoting on what it looks at
  void SyncOrbitToCamera();
  float GetCanvasAspect() const;
  // What the view shows: the orbit camera, or the canvas camera fitted inside the view
  LsCamera GetViewCamera(float viewAspect, glm::vec2& frameNdc) const;

  // The canvas camera seen so its canvas lands exactly where the canvas is drawn
  // in canvas mode. False without a canvas camera.
  bool GetCanvasAlignedCamera(LsCamera& camera) const;
  // How far a transition has got, 0 at its start and 1 at its end
  float GetTransitionProgress() const;
  void DrawCanvasInScene(ImDrawList* drawList, float alpha) const;
  // Renders the scene through the canvas camera, lined up with the canvas, under
  // the UI but over the canvas, leaving input to the canvas
  void DrawSceneOverCanvas(glm::vec2 origin, glm::vec2 size);

  // Puts the 3D cursor on the surface under the mouse, or the ground plane
  void PlaceCursor(glm::vec2 mouse);
  void DrawCursor(ImDrawList* drawList) const;

  // Returns true when the view moved
  bool Navigate(bool viewHovered, glm::vec2 viewSize);
  void HandleShortcuts(bool viewHovered);
  void UpdatePrimitives();
  // Camera nodes that are not hidden, apart from the one being looked through
  std::vector<LsNodeId> GetDrawnCameras() const;
  LsNodeId PickCamera(glm::vec2 mouse) const;
  // Pixel of the view's image under a point in UI points
  glm::uvec2 ToViewPixel(glm::vec2 point) const;
  // Applies the picks that have come back from the GPU
  void UpdatePicks();
  // Forgets the picks still waiting, their results no longer apply
  void CancelPicks();
  // With a debug view on, marks the picked pixels and prints the hover pick
  void DrawPickDebug(ImDrawList* drawList) const;

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
  // A selection the gizmo and the transform keys move, not the root nor the
  // camera being looked through
  bool HasTransformableSelection() const;
  // The axes X, Y and Z lock, see transformKeys
  void GetTransformAxes(const glm::mat4& world, glm::vec3 axes[3]) const;
  void FrameSelection();
  // Orbits around the 3D cursor from now on, keeping the zoom
  void CenterViewOnCursor();
  void AlignCameraToView();
  // Leaving either returns to the view from before, or carries on from the camera
  void SetLookThrough(bool enabled, bool restoreView = true);
  void ViewAlongAxis(int axis, bool negative);
public:
  LsSceneViewport(std::shared_ptr<LsRenderer> renderer, std::shared_ptr<LsScene> scene, LsSceneEditor* editor);
  ~LsSceneViewport();
  // Draws the 3D view over the canvas area, origin and size in UI points, unless
  // the canvas is showing. Call between BeginFrame and EndFrame.
  void Draw(glm::vec2 origin, glm::vec2 size);
  // In canvas mode, shows the selection's gizmo over the canvas through the
  // canvas camera and lets the transform keys move it there, unless it is the
  // canvas camera itself. Call every frame after Draw, enabled when nothing
  // else over the canvas, such as the canvas tool, has the transform keys.
  void UpdateOverCanvas(bool enabled, bool keysAvailable, bool mouseAvailable);
  // The selection's gizmo over the canvas has the transform keys
  bool IsTransformingOverCanvas() const { return transformOverCanvas; }
  // A press on the canvas goes to the selection's gizmo or transform keys
  bool WantsCanvasMouse() const {
    return transformOverCanvas && (gizmo.IsHovered() || gizmo.IsActive() || transformKeys.ClaimsMouse());
  }
  // Switches between the canvas and the 3D view
  void ToggleSceneView();
  // The canvas area is taken by the 3D view, including during the transitions
  bool IsShowingScene() const { return mode != Mode::Canvas; }
  bool IsHovered() const { return mode != Mode::Canvas && viewHovered; }
  void SetCanvasBackground(glm::vec3 color) { canvasBackground = color; }
  bool IsSceneOverCanvas() const { return sceneOverCanvas; }
  void SetSceneOverCanvas(bool shown) { sceneOverCanvas = shown; }
  float& SceneOverCanvasOpacity() { return sceneOverCanvasOpacity; }
  LsSceneOverlay& SceneOverlay() { return sceneOverlay; }
  // Frames must keep coming even without input, such as while flying with keys held
  // and while a click waits for its ray to come back
  bool IsAnimating() const {
    return navigation == Navigation::Fly || mode == Mode::ToScene || mode == Mode::ToCanvas || selectTicket || cursorTicket;
  }
  // An edit made through the view is still going on, such as a drag, the scene
  // changes are not finished yet
  bool IsEditing() const { return gizmo.IsActive() || transformKeys.IsTransforming() || navigation != Navigation::None; }
};
