//===============================================================================
// @ LsHashedBCCLatticeVertex.h
// 
// Point in 3D space made from either lattice node or lattice edge cut point,
// hashed by original lattice coordinates. Original lattice can be freed safely 
// after LsHashedBCCLatticeVertex is constructed(because position is cached).
//
//===============================================================================

#pragma once

#include <glm/glm.hpp>
#include "LsBCCLatticeRef.h"
#include "LsBCCLatticeTypes.h"

class LsBCCLattice;

class LsHashedBCCLatticeVertex: public LsBCCLatticeRef {
  friend std::hash<LsHashedBCCLatticeVertex>;
  private:
    glm::vec3 position;
    std::size_t hash;
  public:
    LsHashedBCCLatticeVertex(LsBCCLattice const& lattice, LsBCCLatticeRef const& node);
    glm::vec3 GetPosition() const;
    std::size_t GetHash() const;
};

namespace std {
    template <>
    class hash<LsHashedBCCLatticeVertex>
    {
    public:
        std::size_t operator()(const LsHashedBCCLatticeVertex& ref) const {
            return ref.GetHash();
        }
        
    };
};
