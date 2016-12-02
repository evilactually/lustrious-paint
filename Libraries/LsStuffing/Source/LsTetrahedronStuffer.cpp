#include "LsBCCLattice.h"
#include <LsTetrahedronStuffer.h>

void LsTetrahedronStuffer::Stuff(LsTetrahedronMesh& mesh, LsIStuffable const& stuffable) {
  std::tuple<int,int,int> minima = stuffable.GetMinima();
  std::tuple<int,int,int> maxima = stuffable.GetMaxima();
  LsBCCLattice bccLattice(minima, maxima, 1.0f);
  UpdateValues(bccLattice, stuffable);
  UpdateCutPoints(bccLattice, stuffable);
  Warp(bccLattice);
  Fill(bccLattice, mesh);
}

void LsTetrahedronStuffer::UpdateValues(LsBCCLattice& lattice, LsIStuffable const& stuffable) {
  LsBCCLattice::NodeIterator iterator = lattice.GetNodeIterator();
}

void LsTetrahedronStuffer::UpdateCutPoints(LsBCCLattice& lattice, LsIStuffable const& stuffable) {

}

void LsTetrahedronStuffer::Warp(LsBCCLattice& lattice) {

}

void LsTetrahedronStuffer::Fill(LsBCCLattice const& lattice, LsTetrahedronMesh& mesh) {

}
