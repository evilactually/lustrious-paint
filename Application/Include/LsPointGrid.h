#pragma once

#include <memory>
#include <LsRenderer.h>
#include <LsFSdlEventHandler.h>

class LsPointGrid: public LsFSdlEventHandler
{
  struct {
    float r;
    float g;
    float b;
  } pointColor = {0.2f, 0.2f, 0.2f};

  struct {
    float r;
    float g;
    float b;
  } backgroundColor = {0.1f, 0.1f, 0.1f};

  float spacing = 32.0f; // in canvas pixels

  std::shared_ptr<LsRenderer> renderer;

public:
  LsPointGrid::LsPointGrid(std::shared_ptr<LsRenderer> renderer);
  ~LsPointGrid();
  void SetPointColor(float r, float g, float b);
  void SetBackgroundColor(float r, float g, float b);
  void SetSpacing(float spacing);
  void Render();
  void OnSdlEvent(const SDL_Event& event) override;
};
