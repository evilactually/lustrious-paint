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
  friend std::hash<LsLatticeVertexRef>;
  private:
    bool isCutPointReference;
    LsBCCLattice const* pLattice;
    LsBCCNode node1;
    LsBCCNode node2;
    LsLatticeVertexRef(LsBCCLattice const* lattice, LsBCCNode node);
    LsLatticeVertexRef(LsBCCLattice const* lattice, LsBCCNode node1, LsBCCNode node2);
  public:
    glm::vec3 GetPosition() const;
    bool operator==( const LsLatticeVertexRef& other ) const;
    static LsLatticeVertexRef MkNodeRef(LsBCCLattice const* lattice, LsBCCNode node);
    static LsLatticeVertexRef MkCutPointRef(LsBCCLattice const* lattice, LsBCCNode node1, LsBCCNode node2);
};

namespace std {
    template <>
    class hash<LsLatticeVertexRef>
    {
    public:
        std::size_t operator()(const LsLatticeVertexRef& ref) const {
            std::size_t hash = 0;
            // XORing hashes because XOR truth table has even distribution of 1s and 0s
            hash ^= std::hash<int>()(std::get<0>(ref.node1));
            hash ^= std::hash<int>()(std::get<1>(ref.node1));
            hash ^= std::hash<int>()(std::get<2>(ref.node1));
            if(ref.isCutPointReference) {
                hash ^= std::hash<int>()(std::get<0>(ref.node2));
                hash ^= std::hash<int>()(std::get<1>(ref.node2));
                hash ^= std::hash<int>()(std::get<2>(ref.node2));
            }
            return hash;
        }
        
    };
};

// class LsLatticeVertexRefHasher {
// public:
//   std::size_t operator()(LsLatticeVertexRef const& ref) const {
//     return ref.GetHash();
//   }
// };