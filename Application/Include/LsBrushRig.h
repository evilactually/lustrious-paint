#pragma once

#include <memory>
#include <SDL3/SDL.h>
#include <LsFSdlEventHandler.h>
#include <LsMath.h>
#include <LsRenderer.h>

class LsBrushRig: public LsFSdlEventHandler
{
  std::shared_ptr<LsRenderer> renderer;
  struct {
    float position[2];
    float orientation[2] = { kPI, kPI/2.0f }; // azimuth, altitude
    float pressure;
    float tilt[2];                            // SDL tilt angles in degrees, -90 to 90
  } penStatus = {};

  void UpdateOrientationFromTilt();
public:
  LsBrushRig(std::shared_ptr<LsRenderer> renderer);
  ~LsBrushRig();
  void OnSdlEvent(const SDL_Event& event) override;
  void Render();
};
