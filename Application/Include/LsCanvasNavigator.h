#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <LsFSdlEventHandler.h>

class LsRenderer;

//-------------------------------------------------------------------------------
// @ LsCanvasNavigator
//-------------------------------------------------------------------------------
// Moves the canvas around the window: dragging with space held or with the
// middle button pans, dragging with ctrl held zooms around the point where
// the drag started (right zooms in, left zooms out), and the mouse wheel
// zooms around the cursor.
//-------------------------------------------------------------------------------
class LsCanvasNavigator: public LsFSdlEventHandler
{
  enum class Mode { None, Pan, Zoom };

  std::shared_ptr<LsRenderer> renderer;
  glm::vec2 offset = glm::vec2(0.0f); // window position of the canvas origin, in pixels
  float zoom = 1.0f;                  // window pixels per canvas pixel

  // State captured when a drag starts
  Mode mode = Mode::None;
  Uint8 dragButton = 0;
  glm::vec2 dragStart;
  glm::vec2 startOffset;
  float startZoom = 1.0f;

  // What a drag would do if it started now, based on the held keys
  Mode GetArmedMode() const;
  // Sets the zoom, keeping canvasPoint at windowPoint on screen
  void ZoomAround(glm::vec2 windowPoint, glm::vec2 canvasPoint, float newZoom);
  void ApplyView();
public:
  LsCanvasNavigator(std::shared_ptr<LsRenderer> renderer);
  // True while space or ctrl is held or a navigation drag is in progress,
  // painting tools should leave the mouse alone then
  bool IsActive() const;
  // Shows a cursor matching the navigation mode, call between BeginFrame and EndFrame
  void UpdateCursor() const;
  glm::vec2 WindowToCanvas(glm::vec2 point) const;
  float GetZoom() const { return zoom; }
  void OnSdlEvent(const SDL_Event& event) override;
};
