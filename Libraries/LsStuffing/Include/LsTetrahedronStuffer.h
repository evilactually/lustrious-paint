
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
// @ LsIStuffable
//-------------------------------------------------------------------------------
// Interface representing a model that can be stuffed by LsTetrahedronStuffer
//-------------------------------------------------------------------------------
class LsIStuffable {
public:
  virtual std::tuple<int,int,int> GetMinima() const = 0;
  virtual std::tuple<int,int,int> GetMaxima() const = 0;
  virtual LsBCCValue GetValueAtVertex(glm::vec3 vertex) const = 0;
  virtual glm::vec3 GetEdgeCutPoint(glm::vec3 e1, glm::vec3 e2) const = 0;
};

class LsTetrahedronStuffer
{
public:
  LsTetrahedronStuffer();
  ~LsTetrahedronStuffer();
  void Stuff(LsTetrahedronMesh& mesh, LsIStuffable const& stuffable);
  void SetAlphaLong(float a);
  void SetAlphaShort(float a);
  void SetStep(float step);
private:
  void UpdateValues(LsBCCLattice& lattice, LsIStuffable const& stuffable);
  void UpdateCutPoints(LsBCCLattice& lattice, LsIStuffable const& stuffable);
  void Warp(LsBCCLattice& lattice);
  void Fill(LsBCCLattice const& lattice, LsTetrahedronMesh& mesh);
};
