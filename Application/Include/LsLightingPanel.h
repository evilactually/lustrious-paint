#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <LsCamera.h>
#include <LsCanvas.h>

class LsCanvasPainter;

//-------------------------------------------------------------------------------
// @ LsLightingPanel
//-------------------------------------------------------------------------------
// Places the canvas's local lights and the normal that normal strokes can paint.
// A semi-transparent surface patch with the brush normal and the hemisphere
// over it, in a view of its own the right mouse button turns, shows each light
// as a dot where its direction meets the unit sphere and the normal as a
// handle, and occluders as dark caps of the directions they shadow. All of them
// are dragged around the sphere, and a double click adds a light there. The
// direction of the canvas camera and the world axes show how the view relates
// to the canvas. Below are the light rigs, with how much of the canvas each is
// painted on, and the lists of lights and occluders, with the ones in the
// current rig, which light strokes paint, checked, and the settings of the
// selected one.
//-------------------------------------------------------------------------------
class LsLightingPanel
{
  std::shared_ptr<LsCanvas> canvas;
  std::shared_ptr<LsCanvasPainter> painter;
  int selected = -1; // slot of the selected light
  int selectedOccluder = -1; // slot of the selected occluder, only one of them is selected
  enum class Drag { None, Light, Occluder, Normal, View };
  Drag drag = Drag::None;
  // The widget's view, turned from the canvas camera's, in degrees. By default
  // it looks down on a patch facing the camera, which lies flat with the
  // hemisphere over it and the camera up.
  static constexpr float kDefaultViewYaw = -25.0f;
  static constexpr float kDefaultViewPitch = -60.0f;
  float viewYaw = kDefaultViewYaw;
  float viewPitch = kDefaultViewPitch;
  int nextLightNumber = 1;
  int nextOccluderNumber = 1;
  // Slot of the light rig light strokes paint, whose lights and occluders the
  // lists check, 0 for none
  uint32_t currentRig = 0;
  int nextRigNumber = 1;

  void DrawRigs();
  int NewRig(const LsLightRig* copy);
  // Whether a light or occluder slot is in the current rig
  bool InRig(uint32_t slot, bool occluder) const;
  void SetInRig(uint32_t slot, bool occluder, bool in);
  void DrawSphere(const LsCamera* camera);
  void DrawNormal();
  void DrawLightList();
  void DrawSelectedLight();
  void DrawOccluderList();
  void DrawSelectedOccluder();
  void AddLight(glm::vec3 direction);
  void AddOccluder(glm::vec3 direction);
  void SelectLight(int slot) { selected = slot; selectedOccluder = -1; }
  void SelectOccluder(int slot) { selectedOccluder = slot; selected = -1; }
  // From the point the normal was last picked at towards the canvas camera, in
  // its view space, along the view axis before any pick. Perspective turns it
  // away from the view axis towards the edges of the canvas.
  glm::vec3 DirectionToCamera(const LsCamera* camera) const;
public:
  // Makes a rig the one light strokes paint, as picking it from the canvas does
  void SelectRig(uint32_t slot) { currentRig = canvas->GetRig(slot) ? slot : 0u; }
  LsLightingPanel(std::shared_ptr<LsCanvas> canvas, std::shared_ptr<LsCanvasPainter> painter);
  // Draws the contents of the panel, between ImGui::Begin and End. The camera is
  // the canvas camera, null when there is none.
  void Draw(const LsCamera* camera);
};
