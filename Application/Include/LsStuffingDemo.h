#pragma once

#include <memory>
#include <LsRenderer.h>
#include <glm/glm.hpp>
#include <windows.h>
#include <LsBCCLattice.h>

class LsStuffingDemo: public LsFWin32MessageHandler
{
public:
  LsStuffingDemo(std::shared_ptr<LsRenderer> renderer);
  ~LsStuffingDemo();
  void Render();
  virtual void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
private:
  std::shared_ptr<LsBCCLattice> lattice;
  std::shared_ptr<LsRenderer> renderer;
  glm::vec3 rotation = { 0.0f, 0.0f, 0.0f};
  float scale = 1.0f;
  glm::vec3 offset = {0.0f, 0.0f, 10.0f};
  float posx_previous = 0.0f;
  float posy_previous = 0.0f;
};
