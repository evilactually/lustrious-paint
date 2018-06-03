#include "LsBCCLattice.h"
#include <LsIsosurfaceStuffer.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include "LsITetrahedronProcessor.h"
#include "LsBCCLatticeRef.h"

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
  return static_cast<std::underlying_type_t<E>>(e);
}

bool operator<(LsBCCValue const& v1, LsBCCValue const& v2) {
  return to_underlying(v1) < to_underlying(v2);
}


// bool operator>(LsBCCValue const& v1, LsBCCValue const& v2) {
//   return to_underlying(v1) > to_underlying(v2);
// }

// bool operator>=(LsBCCValue const& v1, LsBCCValue const& v2) {
//   return to_underlying(v1) >= to_underlying(v2);
// }

// bool operator<=(LsBCCValue const& v1, LsBCCValue const& v2) {
//   return to_underlying(v1) <= to_underlying(v2);
// }

LsIsosurfaceStuffer::LsIsosurfaceStuffer()
{
}

LsIsosurfaceStuffer::~LsIsosurfaceStuffer()
{
}

void LsIsosurfaceStuffer::Stuff(LsITetrahedronProcessor& processor, LsIsosurface const& stuffable) {
  LsBCCLattice bccLattice(stuffable.GetDomain(), step);
  UpdateValues(bccLattice, stuffable);
  UpdateCutPoints(bccLattice, stuffable);
  Warp(bccLattice);
  Fill(bccLattice, processor);
}

void LsIsosurfaceStuffer::Stuff(LsTetrahedronMesh& mesh, LsIsosurface const& stuffable) {
  LsBCCLattice bccLattice(stuffable.GetDomain(), step);
  UpdateValues(bccLattice, stuffable);
  UpdateCutPoints(bccLattice, stuffable);
  Warp(bccLattice);
  Fill(bccLattice, mesh);
}

void LsIsosurfaceStuffer::SetAlphaLong(float a)
{
  alphaLong = a;
}

void LsIsosurfaceStuffer::SetAlphaShort(float a)
{
  alphaShort = a;
}

void LsIsosurfaceStuffer::SetStep(float step)
{
  this->step = step;
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
      glm::vec3 cutPoint = stuffable.IntersectByEdge(p1, p2); // This edge is guaranteed to have a cut point
      lattice.SetEdgeCutPoint(edge, cutPoint);
    }
  } while ( iterator.Next() );
}

void LsIsosurfaceStuffer::Warp(LsBCCLattice& lattice) {
  LsBCCLattice::NodeIterator nodeIterator = lattice.GetNodeIterator();
  do {
    LsBCCNode n1 = nodeIterator;
    LsBCCLattice::NodeEdgeIterator edgeIterator = lattice.GetNodeEdgeIterator(n1);
    do {
      LsBCCEdge edge = edgeIterator;
      if (!lattice.GetEdgeCutPoint(edge)) {
        continue;
      }
      LsBCCNode n2 = std::get<1>(edge);
      glm::vec3 cutPoint = lattice.GetEdgeCutPoint(edge);
      glm::vec3 p1 = lattice.GetNodePosition(n1);
      glm::vec3 p2 = lattice.GetNodePosition(n2);

      float alpha = alphaLong;
      if ( lattice.GetEdgeColor(edge) == LsBCCColor::eRed )
      {
        alpha = alphaShort;
      }

      if ( glm::abs(glm::length((cutPoint - p1)/(p2 - p1))) < alpha ) // cutPoint violates p1
      {
        lattice.SetNodePosition(n1, cutPoint);       // snap violated node to cut point
        lattice.SetNodeValue(n1, LsBCCValue::eZero); // update value, it lays on surface
        lattice.DeleteNodeCutPoints(n1);             // since it lays on surface, no cut points can be possible
      }
    } while ( edgeIterator.Next() );
  } while ( nodeIterator.Next() );
}

void LsIsosurfaceStuffer::Fill(LsBCCLattice const& lattice, LsITetrahedronProcessor& processor) {
  LsBCCLattice::TetrahedronIterator iterator = lattice.GetTetrahedronIterator();
  do {
    LsBCCTetrahedron tetrahedron = iterator;   
    if ( pppp.Match(lattice, tetrahedron) ) { // Group 1

      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::NodeRef(pppp.GetNodeById(1)),
                                      LsBCCLatticeRef::NodeRef(pppp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(pppp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(pppp.GetNodeById(4)) );
    } else if ( zppp.Match(lattice, tetrahedron) ) {
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::NodeRef(zppp.GetNodeById(1)),
                                      LsBCCLatticeRef::NodeRef(zppp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(zppp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(zppp.GetNodeById(4)) );
    } else if ( zzpp.Match(lattice, tetrahedron) ) {
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::NodeRef(zzpp.GetNodeById(1)),
                                      LsBCCLatticeRef::NodeRef(zzpp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(zzpp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(zzpp.GetNodeById(4)) );
    } else if ( zzzp.Match(lattice, tetrahedron) ) {
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::NodeRef(zzzp.GetNodeById(1)),
                                      LsBCCLatticeRef::NodeRef(zzzp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(zzzp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(zzzp.GetNodeById(4)) );
    } else if ( nzzp.Match(lattice, tetrahedron) ) {  // Group 2
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::EdgeCutPointRef(nzzp.GetNodeById(4), nzzp.GetNodeById(1)), // LsCutPoint(GetNodeById(4),GetNodeById(1))
                                      LsBCCLatticeRef::NodeRef(nzzp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(nzzp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(nzzp.GetNodeById(4)) );
    } else if ( nnzp.Match(lattice, tetrahedron) ) {
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::EdgeCutPointRef(nnzp.GetNodeById(4), nnzp.GetNodeById(1)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnzp.GetNodeById(4), nnzp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(nnzp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(nnzp.GetNodeById(4)) );
    } else if ( nnnp.Match(lattice, tetrahedron) ) {
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::EdgeCutPointRef(nnnp.GetNodeById(4), nnnp.GetNodeById(1)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnnp.GetNodeById(4), nnnp.GetNodeById(2)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnnp.GetNodeById(4), nnnp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(nnnp.GetNodeById(4)) );
    } else if ( nnnp.Match(lattice, tetrahedron) ) {
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::EdgeCutPointRef(nnnp.GetNodeById(4), nnnp.GetNodeById(1)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnnp.GetNodeById(4), nnnp.GetNodeById(2)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnnp.GetNodeById(4), nnnp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(nnnp.GetNodeById(4)) );
    } else if ( nzpp.Match(lattice, tetrahedron) ) { // Group 3
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::NodeRef(nzpp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(nzpp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(nzpp.GetNodeById(4)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nzpp.GetNodeById(1), nzpp.GetNodeById(2)) );
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::EdgeCutPointRef(nzpp.GetNodeById(1), nzpp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(nzpp.GetNodeById(3)),
                                      LsBCCLatticeRef::NodeRef(nzpp.GetNodeById(4)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nzpp.GetNodeById(1), nzpp.GetNodeById(2)));
    } else if (nnpp.Match(lattice, tetrahedron)) {
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::NodeRef(nnpp.GetNodeById(1)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnpp.GetNodeById(1), nnpp.GetNodeById(2)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnpp.GetNodeById(3), nnpp.GetNodeById(4)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnpp.GetNodeById(1), nnpp.GetNodeById(4)) );
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::NodeRef(nnpp.GetNodeById(1)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnpp.GetNodeById(1), nnpp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(nnpp.GetNodeById(3)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnpp.GetNodeById(3), nnpp.GetNodeById(4)) );
      processor.OnTetrahedronEmitted( lattice,
                                      LsBCCLatticeRef::EdgeCutPointRef(nnpp.GetNodeById(2), nnpp.GetNodeById(3)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnpp.GetNodeById(1), nnpp.GetNodeById(2)),
                                      LsBCCLatticeRef::NodeRef(nnpp.GetNodeById(3)),
                                      LsBCCLatticeRef::EdgeCutPointRef(nnpp.GetNodeById(3), nnpp.GetNodeById(4)) );
    } else if (nnpp_parity.Match(lattice, tetrahedron)) {  // Group 4
      if ( nnpp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nnpp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        std::cout << "g4 nnpp" << std::endl;
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(2), nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(1), nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(1), nnpp_parity.GetNodeById(4)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nnpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(1), nnpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(2), nnpp_parity.GetNodeById(3)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(2), nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nnpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(1), nnpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(2), nnpp_parity.GetNodeById(4)) );
      } else {
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nnpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(4), nnpp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(4), nnpp_parity.GetNodeById(1)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(1), nnpp_parity.GetNodeById(3)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(1), nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(2), nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(4), nnpp_parity.GetNodeById(2)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nnpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::NodeRef(nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(1), nnpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nnpp_parity.GetNodeById(4), nnpp_parity.GetNodeById(2)) );
      }
    } else if (nppp_parity.Match(lattice, tetrahedron)) {
      if ( nppp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nppp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        std::cout << "g4 nppp" << std::endl;
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(3)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(2)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(2)) );
      } else {
        std::cout << "g4 nppp flip" << std::endl;
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(3)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(2)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nppp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nppp_parity.GetNodeById(1), nppp_parity.GetNodeById(2)) );
      }
    } else if (nzpp_parity.Match(lattice, tetrahedron)) {
      if ( nzpp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nzpp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nzpp_parity.GetNodeById(1), nzpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nzpp_parity.GetNodeById(1), nzpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(3)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nzpp_parity.GetNodeById(1), nzpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nzpp_parity.GetNodeById(1), nzpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(3)) );
      }
      else {
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nzpp_parity.GetNodeById(1), nzpp_parity.GetNodeById(4)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nzpp_parity.GetNodeById(1), nzpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(4)) );
        processor.OnTetrahedronEmitted( lattice,
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(2)),
                                        LsBCCLatticeRef::EdgeCutPointRef(nzpp_parity.GetNodeById(1), nzpp_parity.GetNodeById(3)),
                                        LsBCCLatticeRef::NodeRef(nzpp_parity.GetNodeById(4)) );
      }
    }
  } while ( iterator.Next() );
}

void LsIsosurfaceStuffer::Fill(LsBCCLattice const& lattice, LsTetrahedronMesh& mesh) {
  LsBCCLattice::TetrahedronIterator iterator = lattice.GetTetrahedronIterator();
  do {
    LsBCCTetrahedron tetrahedron = iterator;   
    if ( pppp.Match(lattice, tetrahedron) ) { // Group 1

      mesh.AddTetrahedron( pppp.GetNodePosition(1),
                           pppp.GetNodePosition(2),
                           pppp.GetNodePosition(3),
                           pppp.GetNodePosition(4) );
    } else if ( zppp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( zppp.GetNodePosition(1),
                           zppp.GetNodePosition(2),
                           zppp.GetNodePosition(3),
                           zppp.GetNodePosition(4) );
    } else if ( zzpp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( zzpp.GetNodePosition(1),
                           zzpp.GetNodePosition(2),
                           zzpp.GetNodePosition(3),
                           zzpp.GetNodePosition(4) );
    } else if ( zzzp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( zzzp.GetNodePosition(1),
                           zzzp.GetNodePosition(2),
                           zzzp.GetNodePosition(3),
                           zzzp.GetNodePosition(4) );
    } else if ( nzzp.Match(lattice, tetrahedron) ) {  // Group 2
      mesh.AddTetrahedron( nzzp.GetEdgeCutPoint(4, 1), // LsCutPoint(GetNodeById(4),GetNodeById(1))
                           nzzp.GetNodePosition(2),
                           nzzp.GetNodePosition(3),
                           nzzp.GetNodePosition(4) );
    } else if ( nnzp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( nnzp.GetEdgeCutPoint(4, 1),
                           nnzp.GetEdgeCutPoint(4, 2),
                           nnzp.GetNodePosition(3),
                           nnzp.GetNodePosition(4) );
    } else if ( nnnp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( nnnp.GetEdgeCutPoint(4, 1),
                           nnnp.GetEdgeCutPoint(4, 2),
                           nnnp.GetEdgeCutPoint(4, 3),
                           nnnp.GetNodePosition(4) );
    } else if ( nnnp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( nnnp.GetEdgeCutPoint(4, 1),
                           nnnp.GetEdgeCutPoint(4, 2),
                           nnnp.GetEdgeCutPoint(4, 3),
                           nnnp.GetNodePosition(4) );
    } else if ( nzpp.Match(lattice, tetrahedron) ) { // Group 3
      mesh.AddTetrahedron( nzpp.GetNodePosition(2),
                           nzpp.GetNodePosition(3),
                           nzpp.GetNodePosition(4),
                           nzpp.GetEdgeCutPoint(1, 2) );
      mesh.AddTetrahedron( nzpp.GetEdgeCutPoint(1, 3),
                           nzpp.GetNodePosition(3),
                           nzpp.GetNodePosition(4),
                           nzpp.GetEdgeCutPoint(1, 2));
    } else if (nnpp.Match(lattice, tetrahedron)) {
      mesh.AddTetrahedron( nnpp.GetNodePosition(1),
                           nnpp.GetEdgeCutPoint(1, 2),
                           nnpp.GetEdgeCutPoint(3, 4),
                           nnpp.GetEdgeCutPoint(1, 4) );
      mesh.AddTetrahedron( nnpp.GetNodePosition(1),
                           nnpp.GetEdgeCutPoint(1, 2),
                           nnpp.GetNodePosition(3),
                           nnpp.GetEdgeCutPoint(3, 4) );
      mesh.AddTetrahedron( nnpp.GetEdgeCutPoint(2, 3),
                           nnpp.GetEdgeCutPoint(1, 2),
                           nnpp.GetNodePosition(3),
                           nnpp.GetEdgeCutPoint(3, 4) );
    } else if (nnpp_parity.Match(lattice, tetrahedron)) {  // Group 4
      if ( nnpp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nnpp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        std::cout << "g4 nnpp" << std::endl;
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(3),
                             nnpp_parity.GetEdgeCutPoint(2, 3),
                             nnpp_parity.GetEdgeCutPoint(1, 3),
                             nnpp_parity.GetEdgeCutPoint(1, 4) );
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(3),
                             nnpp_parity.GetNodePosition(4),
                             nnpp_parity.GetEdgeCutPoint(1, 4),
                             nnpp_parity.GetEdgeCutPoint(2, 3) );
        mesh.AddTetrahedron( nnpp_parity.GetEdgeCutPoint(2, 3),
                             nnpp_parity.GetNodePosition(4),
                             nnpp_parity.GetEdgeCutPoint(1, 4),
                             nnpp_parity.GetEdgeCutPoint(2, 4) );
      } else {
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(4),
                             nnpp_parity.GetEdgeCutPoint(4, 2),
                             nnpp_parity.GetEdgeCutPoint(4, 1),
                             nnpp_parity.GetEdgeCutPoint(1, 3) );
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(3),
                             nnpp_parity.GetEdgeCutPoint(1, 3),
                             nnpp_parity.GetEdgeCutPoint(2, 3),
                             nnpp_parity.GetEdgeCutPoint(4, 2) );
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(4),
                             nnpp_parity.GetNodePosition(3),
                             nnpp_parity.GetEdgeCutPoint(1, 3),
                             nnpp_parity.GetEdgeCutPoint(4, 2) );
      }
    } else if (nppp_parity.Match(lattice, tetrahedron)) {
      if ( nppp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nppp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        std::cout << "g4 nppp" << std::endl;
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetEdgeCutPoint(1, 4),
                             nppp_parity.GetEdgeCutPoint(1, 2),
                             nppp_parity.GetEdgeCutPoint(1, 3) );
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetNodePosition(4),
                             nppp_parity.GetNodePosition(2),
                             nppp_parity.GetEdgeCutPoint(1, 2) );
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetNodePosition(4),
                             nppp_parity.GetEdgeCutPoint(1, 4),
                             nppp_parity.GetEdgeCutPoint(1, 2) );
      } else {
        std::cout << "g4 nppp flip" << std::endl;
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(4),
                             nppp_parity.GetEdgeCutPoint(1, 4),
                             nppp_parity.GetEdgeCutPoint(1, 2),
                             nppp_parity.GetEdgeCutPoint(1, 3) );
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetNodePosition(4),
                             nppp_parity.GetNodePosition(2),
                             nppp_parity.GetEdgeCutPoint(1, 2) );
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetNodePosition(4),
                             nppp_parity.GetEdgeCutPoint(1, 3),
                             nppp_parity.GetEdgeCutPoint(1, 2) );
      }
    } else if (nzpp_parity.Match(lattice, tetrahedron)) {
      if ( nzpp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nzpp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        mesh.AddTetrahedron( nzpp_parity.GetNodePosition(2),
                             nzpp_parity.GetEdgeCutPoint(1, 4),
                             nzpp_parity.GetEdgeCutPoint(1, 3),
                             nzpp_parity.GetNodePosition(3) );
        mesh.AddTetrahedron( nzpp_parity.GetNodePosition(2),
                             nzpp_parity.GetEdgeCutPoint(1, 4),
                             nzpp_parity.GetEdgeCutPoint(1, 3),
                             nzpp_parity.GetNodePosition(3) );
      }
      else {
        mesh.AddTetrahedron( nzpp_parity.GetNodePosition(2),
                             nzpp_parity.GetEdgeCutPoint(1, 4),
                             nzpp_parity.GetEdgeCutPoint(1, 3),
                             nzpp_parity.GetNodePosition(4) );
        mesh.AddTetrahedron( nzpp_parity.GetNodePosition(3),
                             nzpp_parity.GetNodePosition(2),
                             nzpp_parity.GetEdgeCutPoint(1, 3),
                             nzpp_parity.GetNodePosition(4) );
      }
    }
  } while ( iterator.Next() );
}

/* Graph matching algorithm

1 is +
2 is +
3 is +
3 is +
any edge 1 2
any edge 2 3
any edge 3 1
any edge 4 1
any edge 4 2
any edge 4 3

1 is -
2 is +
black edge 1 2 cut
3 is +
red edge 1 3 cut
red edge 3 2
4 is 0
black edge 4 3
red edge 4 2
red edge 4 1


Implicitly assign cut points an id based on nodes of the edge
GetPosition(1);
GetPosition(1,2);
GetNode(1);
GetNode(1,2); // ERROR: no such function

auto it = lattice.GetNodeIterator();
lattice.GetNodeValue(it) == LsBCCValue::eNegative; // 1
matches[1] = it;

// find all positive
lattice.GetNodeValue(it) == LsBCCValue::ePositive; // 2
// recur for each

- 0 + +
0 1 2 3

1 n0
2 n2, n3
3 n2, n3
4 n1

1 is +
2 is -
black edge 1 2 cut
3 is +
red edge 2 3 cut
red edge 1 3
4 is -
red edge 4 1 cut
black edge 4 3 cut
red edge 4 2

- - + +
0 1 2 3

1 n2, n3
2 n2, n4
3 n2, n3
4 n2, n4

n2, n3 are good candidates, because they are +'ses
look for all edges that have the 1 in them:

black edge 1 2 cut
red edge 1 3
red edge 4 1 cut

we cannot make a decision right now, because we have not selected 2, 3, 4
but if did select 2, 3, 4

lattice.GetEdgeColor(<edge between 1 and 2>) == black;
lattice.GetEdgeCutPoint(<edge between 1 and 2>) == <not Nothing>;
lattice.GetEdgeColor(<edge between 1 and 3>) == red;
lattice.GetEdgeCutPoint(<edge between 1 and 3>) == <Nothing>;
lattice.GetEdgeColor(<edge between 1 and 4>) == red;
lattice.GetEdgeCutPoint(<edge between 1 and 4>) == <not Nothing>;

// select nodes that are required to check the edges
// if it cannot do that, fail
// if you can 

// make a choice
// recur with the choice
// until no more choices need to be made

// check each possible choice of node 4 choices, each can be 4 different things
// choose 1 as n0, n1, n2, n3
//   choose 2 as n1, n2, n3
//     choose 3 as n2, n3
//       choose 4 as n3
//         check edges

// start with n0, remainding n1,n2,n3
//

// class LsStencil
// {
// public:
//   LsStencil();
//   ~LsStencil();
//   Get
// };

*/

#include <LsOptional.h>

using NodeSelections = std::vector<LsBCCNode>;

bool match(LsBCCLattice const& lattice, 
                                 LsBCCTetrahedron const& tetrahedron,
                                 std::vector<LsBCCNode> selected, 
                                 std::vector<LsBCCNode> remaining,
                                 int index) {
  // n cases
  // n = 3 - index; worsk size of NEXT recursion
  // 4 - index
  // 4 0
  // 3 1
  // 2 2
  // 1 3
  // 0 4
  return false;
}

// template<class T>
// void find_permutation_if(std::vector<T> elements, bool (*predicate)(std::vector<T>const& permutation)) {

// }

#include <FactoradicPermutation.hh>
#include <string>
#include <iostream>

void test() {
  std::string x = "abcd";
  NthPermutation(x.begin(), x.end(), 2);
  std::cout << x << std::endl;
}

class LsPattern
{
public:
  LsPattern();
  ~LsPattern();
  void AddNode(int id);
  void AddEdge(int id);

  
};

class PatternDSL;

struct _id_data_ {
  int id;
  LsBCCValue value;
};

class _is_
{
  PatternDSL& mPattern;
public:
  _is_(PatternDSL& pattern):mPattern(pattern) {};
  ~_is_() {};
  PatternDSL& plus();
};

class _id_
{
  PatternDSL& mPattern;
  int mId;
public:
  _id_(PatternDSL& pattern, int id):mPattern(pattern),mId(id){};
  ~_id_();
  _is_ is();
};

class IPatternDSLInterpreter
{
public:
  IPatternDSLInterpreter();
  ~IPatternDSLInterpreter();
  virtual void OnId(int id, LsBCCValue value) = 0;
  virtual void OnEdge(int id1, int id2, LsBCCColor color, bool isCut) = 0;
};

class PatternDSL
{
friend ::_id_;
friend ::_is_;
  IPatternDSLInterpreter& mInterpreter;
  IPatternDSLInterpreter& GetInterpreter();
public:
  PatternDSL(IPatternDSLInterpreter& interpreter):mInterpreter(interpreter){};
  ~PatternDSL();
  _id_ id(int _id);
};

_is_ _id_::is() {
    return ::_is_(mPattern);
};

PatternDSL& _is_::plus() {
  mPattern.GetInterpreter().OnId(0, LsBCCValue::ePositive);
  return mPattern;
}

IPatternDSLInterpreter& PatternDSL::GetInterpreter() {
  return mInterpreter;
}

_id_ PatternDSL::id(int id) {
  return ::_id_(*this, id);
}

void aaa() {
  // PatternDSL pattern;
  // pattern
  // .id(1).is().plus()
  // .id(2).is().plus()
  // .red().edge(1,2).cut()
}

using LsBCCMaybeColor = LsOptional<LsBCCColor>;

class PatternMatcher // NOTE: we don't need to mark cut, + - edges are always cut, everything else is not
{
private:
  LsBCCNode matchedNodes[4];
  std::tuple<int, LsBCCValue> patternNodes[4];
  int currentNodeIndex = 0;
  std::tuple<int, int, LsBCCMaybeColor> patternEdges[6];
  int currentEdgeIndex = 0;
  LsBCCLattice const* pLattice;
protected:
  void AddNode(int id, LsBCCValue value);
  void AddEdge(int id1, int id2, LsBCCMaybeColor maybeColor);
public:
  PatternMatcher() {};
  ~PatternMatcher() {};
  bool Match(LsBCCLattice const& lattice, LsBCCTetrahedron tetrahedron);
  LsBCCNode GetNodeById(int id);
  glm::vec3 GetNodePosition(int id);             // Convenience functions
  glm::vec3 GetEdgeCutPoint(int id1, int id2);
  LsBCCColor GetNodeColor(int id);
  LsBCCColor GetEdgeColor(int id1, int id2);
  LsBCCValue GetNodeValue(int id);
};

void PatternMatcher::AddNode(int id, LsBCCValue value) {
  assert(currentNodeIndex < 4);
  patternNodes[currentNodeIndex++] = {id, value};
}

void PatternMatcher::AddEdge(int id1, int id2, LsBCCMaybeColor maybeColor) {
  assert(currentEdgeIndex < 6);
  patternEdges[currentEdgeIndex++] = {id1, id2, maybeColor};
}

LsBCCNode PatternMatcher::GetNodeById(int id) {
  for (int i = 0; i < 4; ++i)
  {
    if (std::get<0>(patternNodes[i]) == id) {
      return matchedNodes[i];
    }
  }
}

LsBCCColor PatternMatcher::GetEdgeColor(int id1, int id2) {
  return pLattice->GetEdgeColor(LsBCCEdge(GetNodeById(id1), GetNodeById(id2)));
}

bool PatternMatcher::Match(LsBCCLattice const& lattice, LsBCCTetrahedron tetrahedron) {
  pLattice = &lattice;
  
  // Fill permutation array with initial values
  matchedNodes[0] = std::get<0>(tetrahedron); // Node stored at index 0 in matchedNodes
  matchedNodes[1] = std::get<1>(tetrahedron); // corresponds to id stored at patternNodes
  matchedNodes[2] = std::get<2>(tetrahedron); // under same index.
  matchedNodes[3] = std::get<3>(tetrahedron);

  for (int i = 0; i < 24; ++i) // This loop will try to permute matchedNodes until the pattern fits it.
  {
    NthPermutation(&matchedNodes[0], &matchedNodes[4], i); // Integer tuples implement lexicographical comparison.

    bool failed = false;
    
    // Test that nodes have correct values
    for (int j = 0; j < 4; ++j)
    {
      if (lattice.GetNodeValue(matchedNodes[j]) != std::get<1>(patternNodes[j])) {
        failed = true;
        break;
      }
    }

    // Try next permutation
    if (failed) continue;

    // Test edge colors
    for (int j = 0; j < 6; ++j)
    {
      LsBCCMaybeColor patternEdgeColor = std::get<2>(patternEdges[j]);

      if (!patternEdgeColor) continue; // Pattern accepts any color for this edge, no test.

      int id1 = std::get<0>(patternEdges[j]);
      int id2 = std::get<1>(patternEdges[j]);
      if ( GetEdgeColor(id1, id2) != patternEdgeColor ) {
        failed = true;
        break;
      }
    }

    if(!failed) return true; // Found.
  }
  return false; // Not found.
}

class PPPPMatcher: public PatternMatcher {
public:
  PPPPMatcher();
  ~PPPPMatcher() {};
};

PPPPMatcher::PPPPMatcher() {
  AddNode(1, LsBCCValue::ePositive);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

/*
  Node('+', 1) 
  Edge(eRed, 1, 2).Cut(1).
  Edge(eBlack, 1, 2).Cut(1).

Id(1).Is().Plus().
Id(2).Is().Minus().
Black().Edge(1,2).Cut().
Red().Edge(1,2).Cut()

1 is +
2 is -
black edge 1 2 cut
3 is +
red edge 2 3 cut
red edge 1 3
4 is -
red edge 4 1 cut
black edge 4 3 cut
red edge 4 2

matcher01.AddNode(1, ePositive);
matcher01.AddEdge(1, 2, eRed, true);
.id(1).is().plus();


*/