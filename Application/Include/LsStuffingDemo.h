#pragma once

#include <memory>
#include <LsRenderer.h>
#include <glm/glm.hpp>
#include <windows.h>

//var d = renderer.focal_distance;
//var np = renderer.near_plane;
//var fp = renderer.far_plane;
//var a = renderer.aspect;
//
//var x = v[0];
//var y = v[1];
//var z = v[2];
//var out = [d*x / (z*a), d*y / z, (z - np) / (fp - np)];
//return out;

class LsStuffingDemo: public LsFWin32MessageHandler
{
public:
  LsStuffingDemo(std::shared_ptr<LsRenderer> renderer);
  ~LsStuffingDemo();
  void Render();
  virtual void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
private:
  std::shared_ptr<LsRenderer> renderer;
  glm::vec3 rotation = { 0.0f, 0.0f, 0.0f};
  float scale = 1.0f;
  glm::vec3 offset = {0.0f, 0.0f, 10.0f};
  float posx_previous = 0.0f;
  float posy_previous = 0.0f;
};

// update view parameters from input or animation
// construct a matrix to place an object in view
// apply the matrix to points as we draw
// project the points as well (which is not a matrix)
// apply final viewport transformation to the result of projection

// Bounding box
// Grid and points
// Red = in/surface; Yellow = out
// 