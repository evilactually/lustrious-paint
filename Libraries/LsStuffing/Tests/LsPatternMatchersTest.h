#include <tuple>
#include <glm/glm.hpp>
#include <FactoradicPermutation.hh> // FIXME: This causes strange conflicts with catch framework
#include <LsBCCLattice.h>
#include <LsBCCLatticeTypes.h>
#include <LsPatternMatchers.h>
// MAGIC WHITESPACE
// MAGIC WHITESPACE
// MAGIC WHITESPACE
// MAGIC WHITESPACE
TEST_CASE( "pppp01" ) {
  LsPPPPMatcher pppp;
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int,int,int>(0,0,0), std::tuple<int,int,int>(10,10,10), 1.0f);
  
  LsBCCNode n1 = LsBCCNode(2,0,0);
  LsBCCNode n2 = LsBCCNode(2,0,2);
  LsBCCNode n3 = LsBCCNode(1,1,1);
  LsBCCNode n4 = LsBCCNode(3,1,1);

  // Create a PPPP pattern
  lattice.SetNodeValue(n1,LsBCCValue::ePositive);
  lattice.SetNodeValue(n2,LsBCCValue::ePositive);
  lattice.SetNodeValue(n3,LsBCCValue::ePositive);
  lattice.SetNodeValue(n4,LsBCCValue::ePositive);

  // Success match
  REQUIRE ( pppp.Match(lattice, LsBCCTetrahedron(n1,n2,n3,n4)) );

  // Fail match
  lattice.SetNodeValue(n1,LsBCCValue::eNegative);
  REQUIRE ( !pppp.Match(lattice, LsBCCTetrahedron(n1,n2,n3,n4)) );
}

TEST_CASE( "zppp01" ) {
  LsZPPPMatcher zppp;
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int,int,int>(0,0,0), std::tuple<int,int,int>(10,10,10), 1.0f);
  
  LsBCCNode n1 = LsBCCNode(2,0,0);
  LsBCCNode n2 = LsBCCNode(2,0,2);
  LsBCCNode n3 = LsBCCNode(1,1,1);
  LsBCCNode n4 = LsBCCNode(3,1,1);

  // Create a ZPPP pattern
  lattice.SetNodeValue(n1,LsBCCValue::eZero);
  lattice.SetNodeValue(n2,LsBCCValue::ePositive);
  lattice.SetNodeValue(n3,LsBCCValue::ePositive);
  lattice.SetNodeValue(n4,LsBCCValue::ePositive);
 
  // Success match
  REQUIRE ( zppp.Match(lattice, LsBCCTetrahedron(n1,n2,n3,n4)) );
  REQUIRE ( zppp.GetNodeById(4) == n1 );

  // Shift Zero to n2
  lattice.SetNodeValue(n1,LsBCCValue::ePositive);
  lattice.SetNodeValue(n2,LsBCCValue::eZero);
  lattice.SetNodeValue(n3,LsBCCValue::ePositive);
  lattice.SetNodeValue(n4,LsBCCValue::ePositive);
  REQUIRE ( zppp.Match(lattice, LsBCCTetrahedron(n1,n2,n3,n4)) );
  REQUIRE ( zppp.GetNodeById(4) == n2 );

  // Shift Zero to n3
  lattice.SetNodeValue(n1,LsBCCValue::ePositive);
  lattice.SetNodeValue(n2,LsBCCValue::ePositive);
  lattice.SetNodeValue(n3,LsBCCValue::eZero);
  lattice.SetNodeValue(n4,LsBCCValue::ePositive);
  REQUIRE ( zppp.Match(lattice, LsBCCTetrahedron(n1,n2,n3,n4)) );
  REQUIRE ( zppp.GetNodeById(4) == n3 );

  // Shift Zero to n4
  lattice.SetNodeValue(n1,LsBCCValue::ePositive);
  lattice.SetNodeValue(n2,LsBCCValue::ePositive);
  lattice.SetNodeValue(n3,LsBCCValue::ePositive);
  lattice.SetNodeValue(n4,LsBCCValue::eZero);
  REQUIRE ( zppp.Match(lattice, LsBCCTetrahedron(n1,n2,n3,n4)) );
  REQUIRE ( zppp.GetNodeById(4) == n4 );
}

TEST_CASE( "nzpp01" ) {
  LsNZPPMatcher nzpp;
  LsBCCLattice lattice = LsBCCLattice(std::tuple<int,int,int>(0,0,0), std::tuple<int,int,int>(10,10,10), 1.0f);
  
  LsBCCNode n1 = LsBCCNode(2,0,0);
  LsBCCNode n2 = LsBCCNode(2,0,2);
  LsBCCNode n4 = LsBCCNode(3,1,1);
  LsBCCNode n3 = LsBCCNode(1,1,1);
  
  // Create a NZPP pattern
  lattice.SetNodeValue(n1,LsBCCValue::eNegative);
  lattice.SetNodeValue(n2,LsBCCValue::ePositive);
  lattice.SetNodeValue(n3,LsBCCValue::ePositive);
  lattice.SetNodeValue(n4,LsBCCValue::eZero);
  glm::vec3 c1 = glm::vec3(1.0f,2.0f,3.0f);
  glm::vec3 c2 = glm::vec3(1.1f,2.2f,3.3f);
  lattice.SetEdgeCutPoint(LsBCCEdge(n1,n2), c1);
  lattice.SetEdgeCutPoint(LsBCCEdge(n1,n3), c2);

  // Sanity check
  REQUIRE( lattice.GetEdgeColor(LsBCCEdge(n1,n2)) == LsBCCColor::eBlack );
  REQUIRE( lattice.GetEdgeColor(LsBCCEdge(n3,n4)) == LsBCCColor::eBlack );
  REQUIRE( lattice.GetEdgeColor(LsBCCEdge(n1,n3)) == LsBCCColor::eRed );
  REQUIRE( lattice.GetEdgeColor(LsBCCEdge(n2,n3)) == LsBCCColor::eRed );
  REQUIRE( lattice.GetEdgeColor(LsBCCEdge(n1,n4)) == LsBCCColor::eRed );
  REQUIRE( lattice.GetEdgeColor(LsBCCEdge(n2,n4)) == LsBCCColor::eRed );

  // Success matches, try every permutation
  LsBCCNode nodes[4] = {n1, n2, n3, n4};
  for (int i = 0; i < 24; ++i)
  {
    NthPermutation(&nodes[0], &nodes[4], i);
    REQUIRE ( nzpp.Match(lattice, LsBCCTetrahedron(nodes[0],nodes[1],nodes[2],nodes[3])) );
    REQUIRE( nzpp.GetEdgeColor(1,2) == LsBCCColor::eBlack );
    REQUIRE( nzpp.GetEdgeColor(2,1) == LsBCCColor::eBlack );
    REQUIRE( nzpp.GetEdgeColor(3,4) == LsBCCColor::eBlack );
    REQUIRE( nzpp.GetEdgeColor(4,3) == LsBCCColor::eBlack );
    REQUIRE( nzpp.GetEdgeColor(4,1) == LsBCCColor::eRed );
    REQUIRE( nzpp.GetEdgeColor(1,4) == LsBCCColor::eRed );
    REQUIRE( nzpp.GetEdgeColor(4,2) == LsBCCColor::eRed );
    REQUIRE( nzpp.GetEdgeColor(2,4) == LsBCCColor::eRed );
    REQUIRE( nzpp.GetEdgeColor(3,1) == LsBCCColor::eRed );
    REQUIRE( nzpp.GetEdgeColor(1,3) == LsBCCColor::eRed );
    REQUIRE( nzpp.GetEdgeColor(3,2) == LsBCCColor::eRed );
    REQUIRE( nzpp.GetEdgeColor(2,3) == LsBCCColor::eRed );
    REQUIRE( nzpp.GetNodeById(1) == n1 );
    REQUIRE( nzpp.GetNodeById(2) == n2 );
    REQUIRE( nzpp.GetNodeById(3) == n3 );
    REQUIRE( nzpp.GetNodeById(4) == n4 );
    REQUIRE( glm::all(glm::equal(nzpp.GetEdgeCutPoint(1,2).GetValue(), c1)));
    REQUIRE( glm::all(glm::equal(nzpp.GetEdgeCutPoint(1,3).GetValue(), c2)));
  }
}
