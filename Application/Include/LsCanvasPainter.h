#pragma once

#include <memory>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <LsFSdlEventHandler.h>
#include <LsCanvasNavigator.h>
#include <LsCanvas.h>

class LsCanvasPainter: public LsFSdlEventHandler
{
  std::shared_ptr<LsRenderer> renderer;
  std::shared_ptr<LsCanvasNavigator> navigator;
  std::shared_ptr<LsCanvas> canvas;
  SDL_FPoint grabOffset;
  bool dragging = false;
  bool painting = false;
  // Something over the canvas, such as the tool's gizmo, takes presses instead
  bool mouseClaimed = false;
  float x = 0.0f;
  float y = 0.0f;
  float width = 64.0f;
  float height = 64.0f;
  float brushRadius = 10.0f; // in pixels
  float brushOpacity = 1.0f;
  float brushFalloff = 0.5f;
  bool blendKeepsEdges = true;
  float blendEdgeThreshold = 0.1f;
  bool fixedNormal = false;
  bool rigNormal = true;
  // Picking the normal under the brush instead of painting, while Alt is held
  // or once after ArmPick
  bool picking = false;
  bool pickArmed = false;
  // Where the normal was last picked, in canvas pixels
  glm::vec2 pickPoint = glm::vec2(0.0f);
  bool hasPickPoint = false;
  void Pick(glm::vec2 canvasPoint);
  glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
  uint32_t rig = 0;
  bool sculptPush = false;
  bool sculptAlongNormal = true;
  LsCanvasDabMode mode = LsCanvasDabMode::Depth;
  // Taken from the brush when the stroke starts, so changing it doesn't change a
  // stroke, with the radius in canvas pixels
  LsCanvasBrush strokeBrush;
  // Stroke state in canvas pixels, dabs are spaced evenly along the path regardless of the input rate
  glm::vec2 lastStrokeSample = glm::vec2(0.0f);
  float distanceToNextDab = 0.0f;
  float GetDabSpacing();
  // The brush as set, for a stroke started with or without Shift held
  LsCanvasBrush MakeBrush(bool shift) const;
  void BeginStroke(glm::vec2 canvasPoint);
  void ContinueStroke(glm::vec2 canvasPoint);
public:
  LsCanvasPainter(std::shared_ptr<LsRenderer> renderer, std::shared_ptr<LsCanvasNavigator> navigator,
                  std::shared_ptr<LsCanvas> canvas);
  ~LsCanvasPainter();
  void Render();
  bool HitTest(SDL_FPoint point);
  float GetBrushRadius();
  void SetBrushRadius(float radius);
  // How much a stroke covers at most, see LsCanvasBrush
  float GetBrushOpacity() const { return brushOpacity; }
  void SetBrushOpacity(float opacity) { brushOpacity = opacity; }
  // Fraction of the radius over which dabs fade out, see LsCanvasBrush
  float GetBrushFalloff() const { return brushFalloff; }
  void SetBrushFalloff(float falloff) { brushFalloff = falloff; }
  // Whether blending leaves out neighbours on other surfaces, and how far in
  // front or behind they are as a fraction of the view depth, see LsCanvasBrush
  bool GetBlendKeepsEdges() const { return blendKeepsEdges; }
  void SetBlendKeepsEdges(bool keeps) { blendKeepsEdges = keeps; }
  float GetBlendEdgeThreshold() const { return blendEdgeThreshold; }
  void SetBlendEdgeThreshold(float threshold) { blendEdgeThreshold = threshold; }
  // Normal strokes paint this view space normal instead of the construction
  // geometry's while fixed, see LsCanvasBrush
  bool GetFixedNormal() const { return fixedNormal; }
  void SetFixedNormal(bool fixed) { fixedNormal = fixed; }
  // Where the normal was last picked, in canvas pixels, false before any pick
  bool GetPickPoint(glm::vec2& point) const { point = pickPoint; return hasPickPoint; }
  // Paints the whole canvas with the brush as one stroke, as if painted over
  // everywhere. With shift in light mode it paints no rig.
  void Fill(bool shift);
  // The next press on the canvas picks the normal under the brush, as Alt does
  void ArmPick() { pickArmed = true; }
  bool IsPickArmed() const { return pickArmed; }
  // Light strokes paint the normal with the rig while set
  bool GetRigNormal() const { return rigNormal; }
  void SetRigNormal(bool painted) { rigNormal = painted; }
  glm::vec3 GetNormal() const { return normal; }
  void SetNormal(glm::vec3 normal) { this->normal = normal; }
  // Sculpt strokes push positions away instead of pulling them, see LsCanvasBrush
  bool GetSculptPush() const { return sculptPush; }
  void SetSculptPush(bool push) { sculptPush = push; }
  // Sculpt strokes move the surface along its normal, otherwise along the camera rays
  bool GetSculptAlongNormal() const { return sculptAlongNormal; }
  void SetSculptAlongNormal(bool along) { sculptAlongNormal = along; }
  // Slot of the light rig light strokes paint, 0 for none, see LsCanvasBrush
  uint32_t GetRig() const { return rig; }
  void SetRig(uint32_t rig) { this->rig = rig; }
  // What the brush paints, see LsCanvasDabMode
  LsCanvasDabMode GetMode() const { return mode; }
  void SetMode(LsCanvasDabMode mode) { this->mode = mode; }
  // While claimed a press doesn't start a stroke, see LsCanvasTool::WantsMouse
  void SetMouseClaimed(bool claimed) { mouseClaimed = claimed; }
  bool IsPainting() const { return painting; }
  void OnSdlEvent(const SDL_Event& event) override;
};
