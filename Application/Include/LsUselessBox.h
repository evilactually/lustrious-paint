#pragma once

#include <memory>
#include <SDL3/SDL.h>
#include <LsFSdlEventHandler.h>

class LsUselessBox: public LsFSdlEventHandler
{
  std::shared_ptr<LsRenderer> renderer;
  SDL_FPoint grabOffset;
  bool dragging = false;
  bool painting = false;
  float x = 0.0f;
  float y = 0.0f;
  float width = 64.0f;
  float height = 64.0f;
public:
  LsUselessBox(std::shared_ptr<LsRenderer> renderer);
  ~LsUselessBox();
  void Render();
  bool HitTest(SDL_FPoint point);
  void OnSdlEvent(const SDL_Event& event) override;
};
