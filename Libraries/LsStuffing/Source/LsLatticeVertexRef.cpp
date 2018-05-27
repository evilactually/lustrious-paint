#include "LsLatticeVertexRef.h"
#include "LsBCCLattice.h"

LsLatticeVertexRef::LsLatticeVertexRef(LsBCCLattice const* lattice, LsBCCNode node):pLattice(lattice) {
	this->isCutPointReference = false;
	this->node1 = node;
}

LsLatticeVertexRef::LsLatticeVertexRef(LsBCCLattice const* lattice, LsBCCNode node1, LsBCCNode node2):pLattice(lattice) {
	this->isCutPointReference = false;
	this->node1 = node1;
	this->node2 = node2;
}

glm::vec3 LsLatticeVertexRef::GetPosition() {
	if(isCutPointReference) {
		return pLattice->GetEdgeCutPoint(LsBCCEdge(node1, node2));
	} else {
		return pLattice->GetNodePosition(node1);
	}
}

std::size_t LsLatticeVertexRef::GetHash() const {
  std::size_t hash = 0;
  // XORing hashes because XOR truth table has even distribution of 1s and 0s
  hash ^= std::hash<int>()(std::get<0>(this->node1));
  hash ^= std::hash<int>()(std::get<1>(this->node1));
  hash ^= std::hash<int>()(std::get<2>(this->node1));
  if(isCutPointReference) {
  	hash ^= std::hash<int>()(std::get<0>(this->node2));
    hash ^= std::hash<int>()(std::get<1>(this->node2));
    hash ^= std::hash<int>()(std::get<2>(this->node2));
  }
  return hash;
}

bool LsLatticeVertexRef::operator==( const LsLatticeVertexRef& other ) const {
  if(isCutPointReference) {
  	return other.node1 == node1 && other.node2 == node2; 
  } else {
    return other.node1 == node1; 
  }
}

LsLatticeVertexRef LsLatticeVertexRef::MkNodeRef(LsBCCLattice const* lattice, LsBCCNode node) {
  return LsLatticeVertexRef(lattice, node);
}

LsLatticeVertexRef LsLatticeVertexRef::MkCutPointRef(LsBCCLattice const* lattice, LsBCCNode node1, LsBCCNode node2) {
  return LsLatticeVertexRef(lattice, node1, node2);
}

std::size_t hash_lattice_vertex_ref(LsLatticeVertexRef const& ref) {
    return ref.GetHash();
}
