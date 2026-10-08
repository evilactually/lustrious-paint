#pragma once

#include <memory>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <LsFSdlEventHandler.h>
#include <LsCanvasNavigator.h>
#include <LsCanvas.h>

enum class LsPaintTool
{
  Brush,  // transfers positions from the construction geometry
  Eraser, // removes positions
};

class LsCanvasPainter: public LsFSdlEventHandler
{
  std::shared_ptr<LsRenderer> renderer;
  std::shared_ptr<LsCanvasNavigator> navigator;
  std::shared_ptr<LsCanvas> canvas;
  SDL_FPoint grabOffset;
  bool dragging = false;
  bool painting = false;
  float x = 0.0f;
  float y = 0.0f;
  float width = 64.0f;
  float height = 64.0f;
  float brushRadius = 10.0f; // in pixels
  LsPaintTool tool = LsPaintTool::Brush;
  // Taken from the tool when the stroke starts, so switching tools doesn't change a stroke
  LsCanvasDabMode strokeMode = LsCanvasDabMode::Transfer;
  // Stroke state in canvas pixels, dabs are spaced evenly along the path regardless of the input rate
  glm::vec2 lastStrokeSample = glm::vec2(0.0f);
  float distanceToNextDab = 0.0f;
  float GetDabSpacing();
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
  LsPaintTool GetTool() const { return tool; }
  void SetTool(LsPaintTool tool) { this->tool = tool; }
  void OnSdlEvent(const SDL_Event& event) override;
};
