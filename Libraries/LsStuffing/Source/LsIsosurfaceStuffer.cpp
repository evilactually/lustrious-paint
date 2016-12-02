#include "LsBCCLattice.h"
#include <LsIsosurfaceStuffer.h>

void LsIsosurfaceStuffer::Stuff(LsTetrahedronMesh& mesh, LsIsosurface const& stuffable) {
  LsBCCLattice bccLattice(stuffable.GetDomain());
  UpdateValues(bccLattice, stuffable);
  UpdateCutPoints(bccLattice, stuffable);
  Warp(bccLattice);
  Fill(bccLattice, mesh);
}

void LsIsosurfaceStuffer::UpdateValues(LsBCCLattice& lattice, LsIsosurface const& stuffable) {
  LsBCCLattice::NodeIterator iterator = lattice.GetNodeIterator();
  do {
    LsBCCNode node = iterator;
    lattice.SetNodeValue(node, stuffable.EvaluateAt(lattice.GetNodePosition(node)));
  } while ( iterator.Next() );
}

void LsIsosurfaceStuffer::UpdateCutPoints(LsBCCLattice& lattice, LsIsosurface const& stuffable) {
  LsBCCLattice::EdgeIterator iterator = lattice.GetEdgeIterator();
  do {
    LsBCCEdge edge = iterator;
    LsBCCNode n1 = std::get<0>(edge);
    LsBCCNode n2 = std::get<1>(edge);
    LsBCCValue v1 = lattice.GetNodeValue(n1);
    LsBCCValue v2 = lattice.GetNodeValue(n2);
    if ( (v1 == LsBCCValue::eNegative && v2 == LsBCCValue::ePositive) ||
         (v1 == LsBCCValue::ePositive && v2 == LsBCCValue::eNegative) ) {
      glm::vec3 p1 = lattice.GetNodePosition(n1);
      glm::vec3 p2 = lattice.GetNodePosition(n2);
      glm::vec3 cutPoint = stuffable.IntersectByEdge(p1, p2);
      lattice.SetEdgeCutPoint(edge, cutPoint);
    }
  } while ( iterator.Next() );
}

void LsIsosurfaceStuffer::Warp(LsBCCLattice& lattice) {

}

void LsIsosurfaceStuffer::Fill(LsBCCLattice const& lattice, LsTetrahedronMesh& mesh) {

}

/*
  eUnassigned = 0,
  eNegative = 1,
  eZero = 2,
  ePositive = 3
*/