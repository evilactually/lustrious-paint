#include "LsBCCLattice.h"
#include <LsIsosurfaceStuffer.h>

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

void LsIsosurfaceStuffer::Fill(LsBCCLattice const& lattice, LsTetrahedronMesh& mesh) {
  LsBCCLattice::TetrahedronIterator iterator = lattice.GetTetrahedronIterator();
  do {
    LsBCCTetrahedron tetrahedron = iterator;
    LsBCCNode nodes[4] = { 
      std::get<0>(tetrahedron),
      std::get<1>(tetrahedron),
      std::get<2>(tetrahedron),
      std::get<3>(tetrahedron)
    };

    // Sort nodes by value using a sorting network
    if ( lattice.GetNodeValue(nodes[0]) > lattice.GetNodeValue(nodes[1]) ) {
      std::swap(nodes[0], nodes[1]);
    }

    if (lattice.GetNodeValue(nodes[2]) > lattice.GetNodeValue(nodes[3])) {
      std::swap(nodes[2], nodes[3]);
    }

    if (lattice.GetNodeValue(nodes[0]) > lattice.GetNodeValue(nodes[2]) ) {
      std::swap(nodes[0], nodes[2]);
    }
    
    if (lattice.GetNodeValue(nodes[1]) > lattice.GetNodeValue(nodes[3])) {
      std::swap(nodes[1], nodes[3]);
    }

    if (lattice.GetNodeValue(nodes[1]) > lattice.GetNodeValue(nodes[2])) {
      std::swap(nodes[1], nodes[2]);
    }

    LsBCCValue v0 = lattice.GetNodeValue(nodes[0]);
    LsBCCValue v1 = lattice.GetNodeValue(nodes[1]);
    LsBCCValue v2 = lattice.GetNodeValue(nodes[2]);
    LsBCCValue v3 = lattice.GetNodeValue(nodes[3]);

    // Group 1
    bool pppp = v0 == LsBCCValue::ePositive &&
                v1 == LsBCCValue::ePositive &&
                v2 == LsBCCValue::ePositive &&
                v3 == LsBCCValue::ePositive;
    
    bool zppp = v0 == LsBCCValue::eZero &&
                v1 == LsBCCValue::ePositive &&
                v2 == LsBCCValue::ePositive &&
                v3 == LsBCCValue::ePositive;
    
    bool zzpp = v0 == LsBCCValue::eZero &&
                v1 == LsBCCValue::eZero &&
                v2 == LsBCCValue::ePositive &&
                v3 == LsBCCValue::ePositive;

    bool zzzp = v0 == LsBCCValue::eZero &&
                v1 == LsBCCValue::eZero &&
                v2 == LsBCCValue::eZero &&
                v3 == LsBCCValue::ePositive;

    // Group 2
    bool nzzp = v0 == LsBCCValue::eNegative &&
                v1 == LsBCCValue::eZero &&
                v2 == LsBCCValue::eZero &&
                v3 == LsBCCValue::ePositive;

    bool nnzp = v0 == LsBCCValue::eNegative &&
                v1 == LsBCCValue::eNegative &&
                v2 == LsBCCValue::eZero &&
                v3 == LsBCCValue::ePositive;

    bool nnnp = v0 == LsBCCValue::eNegative &&
                v1 == LsBCCValue::eNegative &&
                v2 == LsBCCValue::eNegative &&
                v3 == LsBCCValue::ePositive;

    // Group 3 & 4
    bool nzpp = v0 == LsBCCValue::eNegative &&
                v1 == LsBCCValue::eZero &&
                v2 == LsBCCValue::ePositive &&
                v3 == LsBCCValue::ePositive;

    bool nnpp = v0 == LsBCCValue::eNegative &&
                v1 == LsBCCValue::eNegative &&
                v2 == LsBCCValue::ePositive &&
                v3 == LsBCCValue::ePositive;

    // Group 4
    bool nppp = v0 == LsBCCValue::eNegative &&
                v1 == LsBCCValue::ePositive &&
                v2 == LsBCCValue::ePositive &&
                v3 == LsBCCValue::ePositive;

    glm::vec3 p0 = lattice.GetNodePosition(nodes[0]);
    glm::vec3 p1 = lattice.GetNodePosition(nodes[1]);
    glm::vec3 p2 = lattice.GetNodePosition(nodes[2]);
    glm::vec3 p3 = lattice.GetNodePosition(nodes[3]);

    if ( pppp && zppp && zzpp && zzzp )
    {
      mesh.AddTetrahedron(p0, p1, p2, p3);
    }

  } while ( iterator.Next() );
}


 