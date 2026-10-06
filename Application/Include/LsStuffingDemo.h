#pragma once

#include <memory>
#include <LsRenderer.h>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <LsFSdlEventHandler.h>
#include <LsBCCLattice.h>
#include <LsBCCLatticeTypes.h>
#include <LsIsosphere.h>
#include <LsTetrahedronMesh.h>
#include <LsIsosurfaceStuffer.h>

class LsStuffingDemo: public LsFSdlEventHandler
{
public:
  LsStuffingDemo(std::shared_ptr<LsRenderer> renderer);
  ~LsStuffingDemo();
  void Render();
  virtual void OnSdlEvent(const SDL_Event& event) override;
private:
  glm::vec3 pan = { 0.0f, 0.0f, 0.0f };
  LsDomain domain;
  std::shared_ptr<LsBCCLattice> lattice;
  std::shared_ptr<LsRenderer> renderer;
  glm::vec3 rotation = { 0.0f, 0.0f, 0.0f};
  float scale = 1.0f;
  glm::vec3 offset = {0.0f, 0.0f, 10.0f};
  float posx_previous = 0.0f;
  float posy_previous = 0.0f;
  // stuffing
  LsIsosphere sphere;
  LsTetrahedronMesh mesh;
  LsIsosurfaceStuffer stuffer;
};
