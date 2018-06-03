#include "LsHashedBCCLatticeVertex.h"
#include "LsBCCLattice.h"
#include "LsBCCLatticeRef.h"

LsHashedBCCLatticeVertex::LsHashedBCCLatticeVertex(LsBCCLattice const& lattice, LsBCCLatticeRef const& ref):LsBCCLatticeRef(ref) {
  if( GetReferenceType() == LsBCCLatticeRefType::eEdgeCutPoint ) {
    position = lattice.GetEdgeCutPoint(LsBCCEdge(n1, n2));
	} else {
    position = lattice.GetNodePosition(n1);
	}

  // XORing hashes because XOR truth table has even distribution of 1s and 0s
  hash = 0;
  hash ^= std::hash<int>()(std::get<0>(n1));
  hash ^= std::hash<int>()(std::get<1>(n1));
  hash ^= std::hash<int>()(std::get<2>(n1));
  if( type == LsBCCLatticeRefType::eEdgeCutPoint ) {
    hash ^= std::hash<int>()(std::get<0>(n2));
    hash ^= std::hash<int>()(std::get<1>(n2));
    hash ^= std::hash<int>()(std::get<2>(n2));
  }
}

glm::vec3 LsHashedBCCLatticeVertex::GetPosition() const {
	return position;
}

std::size_t LsHashedBCCLatticeVertex::GetHash() const {
  return hash;
}

