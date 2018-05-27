//===============================================================================
// @ LsLatticeVertexRef.h
// 
// A reference to vertex on a lattice. Reference can be made either to an actual 
// node or to a cut point between two nodes. Vertex references are hashable by their
// coordinates in the lattice. Used as a key in intermediate hash map during spring mesh
// construction.
//
//===============================================================================
#pragma once

#include <glm/glm.hpp>
#include "LsVector3.h"
#include "LsBCCLatticeTypes.h"

class LsBCCLattice;

class LsLatticeVertexRefHasher;

class LsLatticeVertexRef {
  private:
  	bool isCutPointReference;
  	LsBCCLattice const* pLattice;
  	LsBCCNode node1;
  	LsBCCNode node2;
    LsLatticeVertexRef(LsBCCLattice const* lattice, LsBCCNode node);
    LsLatticeVertexRef(LsBCCLattice const* lattice, LsBCCNode node1, LsBCCNode node2);
  public:
    glm::vec3 GetPosition();
    std::size_t GetHash() const;
    bool operator==( const LsLatticeVertexRef& other ) const;
    static LsLatticeVertexRef MkNodeRef(LsBCCLattice const* lattice, LsBCCNode node);
    static LsLatticeVertexRef MkCutPointRef(LsBCCLattice const* lattice, LsBCCNode node1, LsBCCNode node2);
};

std::size_t hash_lattice_vertex_ref(LsLatticeVertexRef const& ref);

// class LsLatticeVertexRefHasher {
// public:
//   std::size_t operator()(LsLatticeVertexRef const& ref) const {
//     return ref.GetHash();
//   }
// };