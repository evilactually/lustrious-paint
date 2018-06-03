#include "LsBCCLatticeRef.h"

LsBCCLatticeRef::LsBCCLatticeRef(LsBCCNode n1):n1(n1),type(LsBCCLatticeRefType::eNode) {

}

LsBCCLatticeRef::LsBCCLatticeRef(LsBCCNode n1, LsBCCNode n2):n1(n1),n2(n2),type(LsBCCLatticeRefType::eNode) {

}

LsBCCLatticeRef::LsBCCLatticeRef(LsBCCLatticeRef const& other):n1(other.n1),n2(other.n2),type(other.type) {
  
}

LsBCCLatticeRefType LsBCCLatticeRef::GetReferenceType() const {
  return type;
}

LsBCCNode LsBCCLatticeRef::GetNode1() const {
  return n1;
}

LsBCCNode LsBCCLatticeRef::GetNode2() const {
  return n2;
}

LsBCCLatticeRef LsBCCLatticeRef::NodeRef(LsBCCNode n1) {
  return LsBCCLatticeRef(n1);
}

LsBCCLatticeRef LsBCCLatticeRef::EdgeCutPointRef(LsBCCNode n1, LsBCCNode n2) {
  return LsBCCLatticeRef(n1, n2);
}

bool LsBCCLatticeRef::operator==( const LsBCCLatticeRef& other ) const {
  if (this->type != other.type)
  {
    return false;
  }
  
  if(this->type == LsBCCLatticeRefType::eEdgeCutPoint) {
    return other.n1 == n1 && other.n2 == n2;
  } else {
    return other.n1 == n1;
  }
}


// class LsBCCLatticeRef
// {
//   private:
//   LsBCCLatticeRef(LsBCCNode n1);
//   LsBCCLatticeRef(LsBCCNode n1, LsBCCNode n2);
//   public:
//   LsBCCLatticeRefType GetReferenceType();
//   LsBCCNode GetNode1();
//   LsBCCNode GetNode2();
//   static LsBCCLatticeRef NodeRef(LsBCCNode n1);
//   static LsBCCLatticeRef EdgeCutPointRef(LsBCCNode n1, LsBCCNode n2);
// };