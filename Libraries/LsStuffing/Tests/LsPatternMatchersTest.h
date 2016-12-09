#include <tuple>
#include <LsBCCLattice.h>
#include <LsBCCLatticeTypes.h>
#include <LsPatternMatchers.h>

TEST_CASE( "pppp01" ) {
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
  
  LsPPPPMatcher pppp;

  // Success match
  REQUIRE ( pppp.Match(lattice, LsBCCTetrahedron(n1,n2,n3,n4)) );

  // Fail match
  lattice.SetNodeValue(n1,LsBCCValue::eNegative);
  REQUIRE ( !pppp.Match(lattice, LsBCCTetrahedron(n1,n2,n3,n4)) );
}

TEST_CASE( "zppp01" ) {
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

  LsZPPPMatcher zppp;
  
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