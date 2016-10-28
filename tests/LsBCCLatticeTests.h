//===============================================================================
// @ LsBccLatticeTests.h
// 
// Tests for LsBccLattice
//
//===============================================================================

#include "LsBccLattice.h"
#include "LsTuple.h"

int CountPoints(LsBCCLattice* lattice) {
  LsBCCLattice::NodeIterator iter = lattice->GetNodeIterator();
  int count = 0;
  do { count++; } while (iter.Next());
  return count;
}

TEST_CASE( "lattice01", "[stuffing]" ) {
  LsBCCLattice lattice = LsBCCLattice(LsTuple<int,3>(0,0,0), LsTuple<int,3>(10,10,10), 1.0f);
  REQUIRE ( CountPoints(&lattice) == 341 );
  lattice = LsBCCLattice(LsTuple<int,3>(0,0,0), LsTuple<int,3>(5,5,5), 1.0f);
  REQUIRE ( CountPoints(&lattice) == 54 );
}

TEST_CASE( "lattice02", "[stuffing]" ) {
  LsBCCLattice lattice = LsBCCLattice(LsTuple<int,3>(0,0,0), LsTuple<int,3>(10,10,10), 1.0f);
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(0,0,0)) == LsBCCColor::eBlack );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(2,0,0)) == LsBCCColor::eBlack );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(4,0,0)) == LsBCCColor::eBlack );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(8,0,0)) == LsBCCColor::eBlack );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(10,0,0)) == LsBCCColor::eBlack );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(1,1,1)) == LsBCCColor::eRed );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(3,1,1)) == LsBCCColor::eRed );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(5,1,1)) == LsBCCColor::eRed );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(7,1,1)) == LsBCCColor::eRed );
  REQUIRE ( lattice.GetNodeColor(LsBCCNode(9,1,1)) == LsBCCColor::eRed );
}

TEST_CASE( "lattice03", "[stuffing]" ) {
  LsBCCLattice lattice = LsBCCLattice(LsTuple<int,3>(0,0,0), LsTuple<int,3>(10,10,10), 1.0f);
  LsBCCEdge black01 = LsBCCEdge(LsBCCNode(0,0,0), LsBCCNode(2,0,0));
  LsBCCEdge black02 = LsBCCEdge(LsBCCNode(0,0,0), LsBCCNode(0,2,0));
  LsBCCEdge black03 = LsBCCEdge(LsBCCNode(0,0,0), LsBCCNode(0,0,2));
  LsBCCEdge red04 = LsBCCEdge(LsBCCNode(0,0,0), LsBCCNode(1,1,1));
  LsVector3 v01 = LsVector3(0.1f,0.1f,0.1f);
  LsVector3 v02 = LsVector3(0.2f,0.2f,0.2f);
  LsVector3 v03 = LsVector3(0.3f,0.3f,0.3f);
  LsVector3 v04 = LsVector3(0.4f,0.4f,0.4f);
  lattice.SetEdgeCutPoint(black01, v01);
  lattice.SetEdgeCutPoint(black02, v02);
  lattice.SetEdgeCutPoint(black03, v03);
  lattice.SetEdgeCutPoint(red04, v04);
  REQUIRE ( lattice.GetEdgeCutPoint(black01).GetValue() == v01 );
  REQUIRE ( lattice.GetEdgeCutPoint(black02).GetValue() == v02 );
  REQUIRE ( lattice.GetEdgeCutPoint(black03).GetValue() == v03 );
  REQUIRE ( lattice.GetEdgeCutPoint(red04).GetValue() == v04 );
  lattice.DeleteNodeCutPoints(LsBCCNode(0,0,0));
  REQUIRE ( !lattice.GetEdgeCutPoint(black01) );
}