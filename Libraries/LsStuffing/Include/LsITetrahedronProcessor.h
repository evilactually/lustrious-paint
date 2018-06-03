#pragma once

#include "LsBCCLatticeRef.h"

class LsHashedBCCLatticeVertex;

class LsBCCLattice;

class LsITetrahedronProcessor
{
public:
  virtual void OnTetrahedronEmitted(const LsBCCLattice& lattice, const LsBCCLatticeRef& ref1, const LsBCCLatticeRef& ref2, const LsBCCLatticeRef& ref3, const LsBCCLatticeRef& ref4) = 0;
};



