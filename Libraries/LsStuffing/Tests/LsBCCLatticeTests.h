//===============================================================================
// @ LsBccLatticeTests.h
// 
// Tests for LsBccLattice
//
//===============================================================================

#include "LsBccLattice.h"
#include <tuple>
#include <glm/glm.hpp>

int CountPoints(LsBCCLattice* lattice) {
  LsBCCLattice::NodeIterator iter = lattice->GetNodeIterator();
  int count = 0;
  do { count++; } while (iter.Next());
  return count;
}

TEST_CASE( "lattice01", "[stuffing]" ) {
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int,int,int>(0,0,0), std::tuple<int,int,int>(10,10,10), 1.0f);
  REQUIRE ( CountPoints(&lattice) == 341 );
  lattice = LsBCCLattice(std::tuple<int,int,int>(0,0,0), std::tuple<int,int,int>(5,5,5), 1.0f);
  REQUIRE ( CountPoints(&lattice) == 54 );
}

TEST_CASE( "lattice02", "[stuffing]" ) {
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int,int,int>(0,0,0), std::tuple<int,int,int>(10,10,10), 1.0f);
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
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int,int,int>(0,0,0), std::tuple<int,int,int>(10,10,10), 1.0f);
  lattice.SetNodeValue(LsBCCNode(0,0,0), LsBCCValue::ePositive);
  lattice.SetNodeValue(LsBCCNode(2,0,0), LsBCCValue::eNegative);
  lattice.SetNodeValue(LsBCCNode(0,2,0), LsBCCValue::eNegative);
  lattice.SetNodeValue(LsBCCNode(0,0,2), LsBCCValue::eNegative);
  lattice.SetNodeValue(LsBCCNode(1,1,1), LsBCCValue::eNegative);
  LsBCCEdge black01 = LsBCCEdge(LsBCCNode(0,0,0), LsBCCNode(2,0,0));
  LsBCCEdge black02 = LsBCCEdge(LsBCCNode(0,0,0), LsBCCNode(0,2,0));
  LsBCCEdge black03 = LsBCCEdge(LsBCCNode(0,0,0), LsBCCNode(0,0,2));
  LsBCCEdge red04 = LsBCCEdge(LsBCCNode(0,0,0), LsBCCNode(1,1,1));
  glm::vec3 v01 = glm::vec3(0.1f,0.1f,0.1f);
  glm::vec3 v02 = glm::vec3(0.2f,0.2f,0.2f);
  glm::vec3 v03 = glm::vec3(0.3f,0.3f,0.3f);
  glm::vec3 v04 = glm::vec3(0.4f,0.4f,0.4f);
  lattice.SetEdgeCutPoint(black01, v01);
  lattice.SetEdgeCutPoint(black02, v02);
  lattice.SetEdgeCutPoint(black03, v03);
  lattice.SetEdgeCutPoint(red04, v04);
  REQUIRE ( glm::all(glm::equal(lattice.GetEdgeCutPoint(black01).GetValue(), v01)) );
  REQUIRE ( glm::all(glm::equal(lattice.GetEdgeCutPoint(black02).GetValue(), v02)) );
  REQUIRE ( glm::all(glm::equal(lattice.GetEdgeCutPoint(black03).GetValue(), v03)) );
  REQUIRE ( glm::all(glm::equal(lattice.GetEdgeCutPoint(red04).GetValue(), v04)) );
  lattice.DeleteNodeCutPoints(LsBCCNode(0,0,0));
  REQUIRE ( !lattice.GetEdgeCutPoint(black01) );
  REQUIRE ( !lattice.GetEdgeCutPoint(black02) );
  REQUIRE ( !lattice.GetEdgeCutPoint(black03) );
}

TEST_CASE("NodePosition", "[lattice]") {
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int, int, int>(0, 0, 0), std::tuple<int, int, int>(10, 10, 10), 1.0f);
  lattice.SetNodePosition(LsBCCNode(1, 1, 1), glm::vec3(0.1f, 0.2f, 0.3f));
  lattice.SetNodePosition(LsBCCNode(2, 2, 2), glm::vec3(0.4f, 0.5f, 0.6f));
  REQUIRE(glm::all(glm::equal(lattice.GetNodePosition(LsBCCNode(1, 1, 1)), glm::vec3(0.1f, 0.2f, 0.3f))));
  REQUIRE(glm::all(glm::equal(lattice.GetNodePosition(LsBCCNode(2, 2, 2)), glm::vec3(0.4f, 0.5f, 0.6f))));
}


TEST_CASE("NodeValue", "[lattice]") {
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int, int, int>(0, 0, 0), std::tuple<int, int, int>(10, 10, 10), 1.0f);
  lattice.SetNodeValue( LsBCCNode(1, 1, 1), LsBCCValue::ePositive );
  lattice.SetNodeValue( LsBCCNode(2, 2, 2), LsBCCValue::eZero );
  lattice.SetNodeValue( LsBCCNode(3, 3, 3), LsBCCValue::eNegative );
  REQUIRE( lattice.GetNodeValue(LsBCCNode(1, 1, 1)) == LsBCCValue::ePositive );
  REQUIRE( lattice.GetNodeValue(LsBCCNode(2, 2, 2)) == LsBCCValue::eZero );
  REQUIRE( lattice.GetNodeValue(LsBCCNode(3, 3, 3)) == LsBCCValue::eNegative );
}

TEST_CASE("NodeColor", "[lattice]") {
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int, int, int>(0, 0, 0), std::tuple<int, int, int>(10, 10, 10), 1.0f);
  REQUIRE( lattice.GetNodeColor(LsBCCNode(1, 1, 1)) == LsBCCColor::eRed );
  REQUIRE( lattice.GetNodeColor(LsBCCNode(2, 2, 2)) == LsBCCColor::eBlack );
  REQUIRE( lattice.GetNodeColor(LsBCCNode(1, 3, 3)) == LsBCCColor::eRed );
  REQUIRE( lattice.GetNodeColor(LsBCCNode(2, 0, 2)) == LsBCCColor::eBlack );
}

TEST_CASE("EdgeColor", "[lattice]") {
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int, int, int>(0, 0, 0), std::tuple<int, int, int>(10, 10, 10), 1.0f);
  REQUIRE(lattice.GetEdgeColor( LsBCCEdge(LsBCCNode(1, 1, 1), LsBCCNode(3, 1, 1)) ) == LsBCCColor::eBlack);
  REQUIRE(lattice.GetEdgeColor(LsBCCEdge(LsBCCNode(1, 1, 1), LsBCCNode(2, 2, 2))) == LsBCCColor::eRed);
  // REQUIRE(lattice.GetEdgeColor(LsBCCEdge(LsBCCNode(1, 1, 1), LsBCCNode(5, 5, 5))) == LsBCCColor::eBlack); // TODO: This should abort, invalid edge
}

// TODO:
// GetTetrahedronIterator
// GetNodeEdgeIterator
// GetEdgeIterator