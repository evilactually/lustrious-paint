
// compute sign +, -, 0, virtual
// compute intersection with zero surface given an edge, virtual
// compute signs, private
// compute cut points, private
// warp, private
// fill, private
// set alpha and beta, public
// void Stuff(LsTetrahedronMesh& mesh), public // stuff the surface and output resulting mesh into provided instance of LsTetrahedronMesh
#pragma once

#include <tuple>
#include <memory>
#include <glm/glm.hpp>
#include "LsBCCLattice.h"
#include "LsTetrahedronMesh.h"
#include "LsPatternMatchers.h"
#include "LsITetrahedronProcessor.h"

//-------------------------------------------------------------------------------
// @ LsIsosurface
//-------------------------------------------------------------------------------
// Interface representing a model that can be stuffed by LsIsosurfaceStuffer
//-------------------------------------------------------------------------------
class LsIsosurface {
public:
  virtual LsDomain GetDomain() const = 0;
  virtual LsBCCValue EvaluateAt(glm::vec3 vertex) const = 0;
  virtual glm::vec3 IntersectByEdge(glm::vec3 p1, glm::vec3 p2) const = 0;
};

class ILsTetrahedronProcessor;

class LsIsosurfaceStuffer
{
public:
  LsIsosurfaceStuffer();
  ~LsIsosurfaceStuffer();
  void Stuff(LsITetrahedronProcessor& processor, LsIsosurface const& stuffable);
  void Stuff(LsTetrahedronMesh& mesh, LsIsosurface const& stuffable);
  void SetAlphaLong(float a);
  void SetAlphaShort(float a);
  void SetStep(float step);
#ifndef TESTING
private:
#endif
  float step = 1.0f;
  float alphaLong = 0.24999f;
  float alphaShort = 0.40173f;
  LsPPPPMatcher pppp; // Group 1
  LsZPPPMatcher zppp;
  LsZZPPMatcher zzpp;
  LsZZZPMatcher zzzp;
  LsNZZPMatcher nzzp; // Group 2
  LsNNZPMatcher nnzp;
  LsNNNPMatcher nnnp;
  LsNZPPMatcher nzpp; // Group 3
  LsNZPPMatcher nnpp;
  LsNZPPParityMatcher nzpp_parity; // Group 4
  LsNPPPParityMatcher nppp_parity;
  LsNNPPParityMatcher nnpp_parity;
  void UpdateValues(LsBCCLattice& lattice, LsIsosurface const& stuffable);
  void UpdateCutPoints(LsBCCLattice& lattice, LsIsosurface const& stuffable);
  void Warp(LsBCCLattice& lattice);
  void Fill(LsBCCLattice const& lattice, LsITetrahedronProcessor& processor);
  void Fill(LsBCCLattice const& lattice, LsTetrahedronMesh& mesh);
};
