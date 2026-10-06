#pragma once

#include <vector>
#include <SDL3/SDL.h>

class LsFSdlEventHandler
{
  static std::vector<LsFSdlEventHandler*> Handlers;
public:
  LsFSdlEventHandler();
  ~LsFSdlEventHandler();
  // Event coordinates (mouse, pen) are converted to window pixels before broadcast
  virtual void OnSdlEvent(const SDL_Event& event) = 0;
  static void BroadcastSdlEvent(const SDL_Event& event);
};
