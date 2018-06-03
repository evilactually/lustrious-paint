//===============================================================================
// @ LsBCCLatticeRef.h
// 
// A simple wrapper that abstract away a difference between a cut point and a node.
//
//===============================================================================

#pragma once

#include "LsBCCLatticeTypes.h"

enum class LsBCCLatticeRefType
{
  eNode = 0,
  eEdgeCutPoint = 1
};

class LsBCCLatticeRef
{
  protected:
  LsBCCNode n1, n2;
  LsBCCLatticeRefType type;
  LsBCCLatticeRef(LsBCCNode n1);
  LsBCCLatticeRef(LsBCCNode n1, LsBCCNode n2);
  public:
  LsBCCLatticeRef(LsBCCLatticeRef const& ref);
  LsBCCLatticeRefType GetReferenceType() const;
  LsBCCNode GetNode1() const;
  LsBCCNode GetNode2() const;
  bool operator==( const LsBCCLatticeRef& other ) const;
  static LsBCCLatticeRef NodeRef(LsBCCNode n1);
  static LsBCCLatticeRef EdgeCutPointRef(LsBCCNode n1, LsBCCNode n2);
};