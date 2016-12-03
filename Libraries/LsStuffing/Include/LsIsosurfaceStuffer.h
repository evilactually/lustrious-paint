
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

//-------------------------------------------------------------------------------
// @ LsIsosurface
//-------------------------------------------------------------------------------
// Interface representing a model that can be stuffed by LsIsosurfaceStuffer
//-------------------------------------------------------------------------------
class LsIsosurface {
public:
  virtual LsDomain GetDomain() const = 0;
  virtual LsBCCValue EvaluateAt(glm::vec3 vertex) const = 0;
  virtual glm::vec3 IntersectByEdge(glm::vec3 e1, glm::vec3 e2) const = 0;
};

class LsIsosurfaceStuffer
{
public:
  LsIsosurfaceStuffer();
  ~LsIsosurfaceStuffer();
  void Stuff(LsTetrahedronMesh& mesh, LsIsosurface const& stuffable);
  void SetAlphaLong(float a);
  void SetAlphaShort(float a);
  void SetStep(float step);
private:
  float alphaLong;
  float alphaShort;
  void UpdateValues(LsBCCLattice& lattice, LsIsosurface const& stuffable);
  void UpdateCutPoints(LsBCCLattice& lattice, LsIsosurface const& stuffable);
  void Warp(LsBCCLattice& lattice);
  void Fill(LsBCCLattice const& lattice, LsTetrahedronMesh& mesh);
};
