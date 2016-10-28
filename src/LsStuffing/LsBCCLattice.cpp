//===============================================================================
// @ LsBccLattice.h
// 
// Body Centered Cubic Lattice data structure
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "LsBCCLattice.h"
#include "LsMath.h"
#include "assert.h"

//-------------------------------------------------------------------------------
//-- Typedefs -------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsBCCNodeOffset
//-------------------------------------------------------------------------------
// Specifies displacement between two nodes
//-------------------------------------------------------------------------------
typedef LsTuple<int, 3> LsBCCNodeOffset;

//-------------------------------------------------------------------------------
//-- Constants -------------------------------------------------------------------
//-------------------------------------------------------------------------------

static const LsBCCNodeOffset nexusOffsets[7] = {
  {1,1,1},
  {1,1,-1},
  {-1,1,-1},
  {-1,1,1},
  {2,0,0},
  {0,2,0},
  {0,0,2}
};

static const LsBCCNodeOffset adjacentOffsets[14] = {
  {1,1,1},
  {1,1,-1},
  {-1,1,-1},
  {-1,1,1},
  {2,0,0},
  {0,2,0},
  {0,0,2},
  {-1,-1,-1},
  {-1,-1,1},
  {1,-1,1},
  {1,-1,-1},
  {-2,0,0},
  {0,-2,0},
  {0,0,-2}
};

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

LsBCCNodeOffset SubtractNodes(LsBCCNode const & n1, LsBCCNode const & n2) {
  return LsBCCNodeOffset(n1[0] - n2[0], n1[1] - n2[1], n1[2] - n2[2]);
}

LsBCCNode AddNodeOffset(LsBCCNode const & node, LsBCCNodeOffset const & offset) {
  return LsBCCNode(node[0] + offset[0], node[1] + offset[1], node[2] + offset[2]);
}

bool NodesEqual(LsBCCNode const & n1, LsBCCNode const & n2) {
  return n1[0] == n2[0] && n1[1] == n2[1] && n1[2] == n2[2];
}

bool NodeOffsetsEqual(LsBCCNodeOffset const & o1, LsBCCNodeOffset const & o2) {
  return o1[0] == o2[0] && o1[1] == o2[1] && o1[2] == o2[2];
}

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

LsBCCLattice::NodeIterator::NodeIterator(LsBCCLattice const& lattice):lattice(lattice) {
  current = lattice.nodeMetaData[currentIndex].coordinates;
};

LsBCCLattice::NodeIterator::operator LsBCCNode() const {
  return current;
}

bool LsBCCLattice::NodeIterator::Next() {
  ++currentIndex;
  if ( currentIndex < lattice.nodeMetaData.size() )
  {
    current = lattice.nodeMetaData[currentIndex].coordinates;
    return true; 
  } else {
    return false;
  }
}

LsBCCLattice::TetrahedronIterator::TetrahedronIterator(LsBCCLattice const& lattice):lattice(lattice) { };

LsOptional<LsBCCTetrahedron> LsBCCLattice::TetrahedronIterator::Next() { 
  throw 1;
};

LsBCCLattice::NodeEdgeIterator::NodeEdgeIterator(LsBCCLattice const& lattice, LsBCCNode node):lattice(lattice),n1(node) {
  LsBCCNode n2 = AddNodeOffset(n1, adjacentOffsets[currentAdjacentIndex]);
  current = LsBCCEdge(n1, n2);
};

LsBCCLattice::NodeEdgeIterator::operator LsBCCEdge() const {
  return current;
}

bool LsBCCLattice::NodeEdgeIterator::Next() {
  ++currentAdjacentIndex;
  if ( currentAdjacentIndex < 14 )
  {
    LsBCCNode n2 = AddNodeOffset(n1, adjacentOffsets[currentAdjacentIndex]);
    current = LsBCCEdge(n1, n2);
    return true;
  } else {
    return false;
  }
}

LsBCCLattice::EdgeIterator::EdgeIterator(LsBCCLattice const& lattice):lattice(lattice) { };

LsOptional<LsBCCEdge> LsBCCLattice::EdgeIterator::Next() {
  throw 1;
}

//-------------------------------------------------------------------------------
// @ LsBCCLattice::LsBCCLattice()
//-------------------------------------------------------------------------------
// Initialize lattice bounded by inclusive interval
//-------------------------------------------------------------------------------
LsBCCLattice::LsBCCLattice(LsTuple<int,3> minima,
                           LsTuple<int,3> maxima,
                           float step):minima(minima), maxima(maxima) {
  for (int z = minima[2]; z <= maxima[2]; ++z)
  {
    for (int y = minima[1]; y <= maxima[1]; ++y)
    {
      for (int x = minima[0]; x <= maxima[0]; ++x)
      {
        LsBCCNode node = {x,y,z};
        if ( Valid(node) )
        {
          NodeMetaData nodeInfo;
          nodeInfo.coordinates = node;
          nodeInfo.position = step*LsVector3(x,y,z); // Vertex coordinates correspond to 
          nodeInfo.value = LsBCCValue::eUnassigned;  // BCC grid coordinates scaled by step
          nodeMetaData.push_back(nodeInfo);
        }
      }
    }
  }
}

LsBCCLattice::TetrahedronIterator LsBCCLattice::GetTetrahedronIterator() {   //   TODO: ???
  return TetrahedronIterator(*this);
}

LsBCCLattice::NodeIterator LsBCCLattice::GetNodeIterator() {
  return NodeIterator(*this);
}

LsBCCLattice::NodeEdgeIterator LsBCCLattice::GetNodeEdgeIterator(LsBCCNode node) {
  return NodeEdgeIterator(*this, node);
}

// BCCLattice::EdgeIterator BCCLattice::GetEdgeIterator();               //   TODO: iterate over vertecies, iterate over nexus edges, use bounds to filter non-existent 

LsVector3 LsBCCLattice::GetNodePosition(LsBCCNode node) const {
  return GetNodeMetaDataConstReference(node).position;
}

LsBCCColor LsBCCLattice::GetNodeColor(LsBCCNode node) const {
  assert(Valid(node));
  if ( LsEven(node[0]) ) // Node is black iff all of it's coordinates are even
    return LsBCCColor::eBlack;
  else
    return LsBCCColor::eRed;
}

LsBCCValue LsBCCLattice::GetNodeValue(LsBCCNode node) const {
  assert(Valid(node));
  return GetNodeMetaDataConstReference(node).value;
}

void LsBCCLattice::SetNodeValue(LsBCCNode node, LsBCCValue value) {
  assert(Valid(node));
  GetNodeMetaDataReference(node).value = value;
}

void LsBCCLattice::SetNodePosition(LsBCCNode node, LsVector3 position) {
  assert(Valid(node));
  GetNodeMetaDataReference(node).position = position;
}

void LsBCCLattice::DeleteNodeCutPoints(LsBCCNode node) {
  assert(Valid(node));
  NodeEdgeIterator nodeEdges = GetNodeEdgeIterator(node);
  LsBCCEdge edge = nodeEdges;
  GetEdgeMetaDataReference(edge);
}
 
LsOptional<LsVector3> LsBCCLattice::GetEdgeCutPoint(LsBCCEdge edge) const {
  return GetEdgeMetaDataConstReference(edge).cutPoint;
}

void LsBCCLattice::SetEdgeCutPoint(LsBCCEdge edge, LsVector3 position) {
  GetEdgeMetaDataReference(edge).cutPoint = position;
}

LsBCCColor LsBCCLattice::GetEdgeColor(LsBCCEdge edge) const {
  // Edge is black if both it's points are red or both it's points are black, otherwise it's red
  LsBCCColor color1 = GetNodeColor(edge[0]);
  LsBCCColor color2 = GetNodeColor(edge[1]);
  if ( color1 == LsBCCColor::eRed && color2 == LsBCCColor::eRed )
  {
    return LsBCCColor::eBlack; 
  } else if ( color1 == LsBCCColor::eBlack && color2 == LsBCCColor::eBlack ) {
    return LsBCCColor::eBlack;
  } else {
    return LsBCCColor::eRed;
  }
}

LsOptional<int> LsBCCLattice::GetEdgeIndexInNexus(LsBCCEdge edge) const {
  // Get vector representing an edge
  LsBCCNodeOffset offset = SubtractNodes(edge[1], edge[0]);

  // Test if edge matches one of nexus pattern edges
  for (int i = 0; i < 7; ++i)
  {
    if( NodeOffsetsEqual(nexusOffsets[i], offset) ) {
      return i;
    }
  }
  return LsOptional<int>::None();
}

// Every edge has one of it's nodes designated to be used as storage of edge information, called nexus node
LsBCCNode LsBCCLattice::GetEdgeNexusNode(LsBCCEdge edge) const {
  if ( GetEdgeIndexInNexus(edge) )
  {
    return edge[0];
  }
  // If edge didn't match any pattern it must be flipped
  return edge[1];
}

LsBCCLattice::NodeMetaData& LsBCCLattice::GetNodeMetaDataReference(LsBCCNode node) {
  return nodeMetaData[GetNodeIndex(node)];
}

LsBCCLattice::NodeMetaData const& LsBCCLattice::GetNodeMetaDataConstReference(LsBCCNode node) const {
  return nodeMetaData[GetNodeIndex(node)];
}

LsBCCLattice::EdgeMetaData& LsBCCLattice::GetEdgeMetaDataReference(LsBCCEdge edge) {
  LsBCCNode nexusNode = GetEdgeNexusNode(edge);
  int edgeIndex = GetEdgeIndexInNexus(edge);
  return GetNodeMetaDataReference(nexusNode).edgeNexus[edgeIndex];
}

LsBCCLattice::EdgeMetaData const& LsBCCLattice::GetEdgeMetaDataConstReference(LsBCCEdge edge) const {
  LsBCCNode nexusNode = GetEdgeNexusNode(edge);
  int edgeIndex = GetEdgeIndexInNexus(edge);
  return GetNodeMetaDataConstReference(nexusNode).edgeNexus[edgeIndex]; 
}

int LsBCCLattice::GetNodeIndex(LsBCCNode node) const {
  assert(NodeExists(node));
  int evenPlanesCount = LsEvenCount(minima[2], node[2] - 1);
  int oddPlanesCount = LsOddCount(minima[2], node[2] - 1);
  int evenPlaneRowSize = LsEvenCount(minima[0],maxima[0]);
  int oddPlaneRowSize = LsOddCount(minima[0],maxima[0]);
  int evenPlaneRowCount = LsEvenCount(minima[1],maxima[1]);
  int oddPlaneRowCount = LsOddCount(minima[1],maxima[1]);
  int evenPlaneSize = evenPlaneRowCount*evenPlaneRowSize;
  int oddPlaneSize = oddPlaneRowCount*oddPlaneRowSize;
  int planeOffset = evenPlaneSize*evenPlanesCount + oddPlaneSize*oddPlanesCount;
  int rowOffset;
  int columnOffset;
  if ( LsEven(node[2]) ) {
    rowOffset = LsEvenCount(minima[1], node[1] - 1)*evenPlaneRowSize;
    columnOffset = LsEvenCount(minima[0], node[0] - 1);
  } else {
    rowOffset = LsOddCount(minima[1], node[1] - 1)*oddPlaneRowSize;
    columnOffset = LsOddCount(minima[0], node[0] - 1);
  }
  return planeOffset + rowOffset + columnOffset;
}

bool LsBCCLattice::WithinBounds(LsBCCNode node) const {
  return node[0] >= minima[0] &&
         node[1] >= minima[1] &&
         node[2] >= minima[2] &&
         node[0] <= maxima[0] &&
         node[1] <= maxima[1] &&
         node[2] <= maxima[2];
}

bool LsBCCLattice::Valid(LsBCCNode node) const {
  return LsEven(node[0]) && LsEven(node[1]) && LsEven(node[2]) ||
         LsOdd(node[0]) && LsOdd(node[1]) && LsOdd(node[2]);
}

bool LsBCCLattice::NodeExists(LsBCCNode node) const {
  return WithinBounds(node) && Valid(node);
}
