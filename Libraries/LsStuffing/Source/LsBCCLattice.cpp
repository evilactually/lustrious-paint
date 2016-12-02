//===============================================================================
// @ LsBccLattice.h
// 
// Body Centered Cubic Lattice data structure
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsBCCLattice.h>
#include <LsMath.h>
#include <assert.h>

//-------------------------------------------------------------------------------
//-- Typedefs -------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsBCCNodeOffset
//-------------------------------------------------------------------------------
// Specifies displacement between two nodes
//-------------------------------------------------------------------------------
typedef std::tuple<int, int, int> LsBCCNodeOffset;

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
  return LsBCCNodeOffset(std::get<0>(n1) - std::get<0>(n2), std::get<1>(n1) - std::get<1>(n2), std::get<2>(n1) - std::get<2>(n2));
}

LsBCCNode AddNodeOffset(LsBCCNode const & node, LsBCCNodeOffset const & offset) {
  return LsBCCNode(std::get<0>(node) + std::get<0>(offset), std::get<1>(node) + std::get<1>(offset), std::get<2>(node) + std::get<2>(offset));
}

bool NodesEqual(LsBCCNode const & n1, LsBCCNode const & n2) {
  return std::get<0>(n1) == std::get<0>(n2) && std::get<1>(n1) == std::get<1>(n2) && std::get<2>(n1) == std::get<2>(n2);
}

bool NodeOffsetsEqual(LsBCCNodeOffset const & o1, LsBCCNodeOffset const & o2) {
  return std::get<0>(o1) == std::get<0>(o2) && std::get<1>(o1) == std::get<1>(o2) && std::get<2>(o1) == std::get<2>(o2);
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

LsBCCLattice::LsBCCLattice(LsDomain domain) {
  // TODO: Implement this in terms of previous constructor code. Domain is a floating 
  // point bounding box. The grid must include every tetrahedron that is touched by it.
  // It can be accomplished simply by rounding up by step size outwards.
}

//-------------------------------------------------------------------------------
// @ LsBCCLattice::LsBCCLattice()
//-------------------------------------------------------------------------------
// Initialize lattice bounded by inclusive interval
//-------------------------------------------------------------------------------
LsBCCLattice::LsBCCLattice(std::tuple<int, int, int> minima,
                           std::tuple<int, int, int> maxima,
                           float step):minima(minima), maxima(maxima) {
  for (int z = std::get<2>(minima); z <= std::get<2>(maxima); ++z)
  {
    for (int y = std::get<1>(minima); y <= std::get<1>(maxima); ++y)
    {
      for (int x = std::get<0>(minima); x <= std::get<0>(maxima); ++x)
      {
        LsBCCNode node = {x,y,z};
        if ( Valid(node) )
        {
          NodeMetaData nodeInfo;
          nodeInfo.coordinates = node;
          nodeInfo.position = step*glm::vec3(x,y,z); // Vertex coordinates correspond to 
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

glm::vec3 LsBCCLattice::GetNodePosition(LsBCCNode node) const {
  return GetNodeMetaDataConstReference(node).position;
}

LsBCCColor LsBCCLattice::GetNodeColor(LsBCCNode node) const {
  assert(Valid(node));
  if ( LsEven(std::get<0>(node)) ) // Node is black iff all of it's coordinates are even
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

void LsBCCLattice::SetNodePosition(LsBCCNode node, glm::vec3 position) {
  assert(Valid(node));
  GetNodeMetaDataReference(node).position = position;
}

void LsBCCLattice::DeleteNodeCutPoints(LsBCCNode node) {
  assert(Valid(node));
  NodeEdgeIterator nodeEdges = GetNodeEdgeIterator(node);
  LsBCCEdge edge = nodeEdges;
  GetEdgeMetaDataReference(edge);
}
 
LsOptional<glm::vec3> LsBCCLattice::GetEdgeCutPoint(LsBCCEdge edge) const {
  return GetEdgeMetaDataConstReference(edge).cutPoint;
}

void LsBCCLattice::SetEdgeCutPoint(LsBCCEdge edge, glm::vec3 position) {
  GetEdgeMetaDataReference(edge).cutPoint = position;
}

LsBCCColor LsBCCLattice::GetEdgeColor(LsBCCEdge edge) const {
  // Edge is black if both it's points are red or both it's points are black, otherwise it's red
  LsBCCColor color1 = GetNodeColor(std::get<0>(edge));
  LsBCCColor color2 = GetNodeColor(std::get<1>(edge));
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
  LsBCCNodeOffset offset = SubtractNodes(std::get<1>(edge), std::get<0>(edge));

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
    return std::get<0>(edge);
  }
  // If edge didn't match any pattern it must be flipped
  return std::get<1>(edge);
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
  int evenPlanesCount = LsEvenCount(std::get<2>(minima), std::get<2>(node) - 1);
  int oddPlanesCount = LsOddCount(std::get<2>(minima), std::get<2>(node) - 1);
  int evenPlaneRowSize = LsEvenCount(std::get<0>(minima), std::get<0>(maxima));
  int oddPlaneRowSize = LsOddCount(std::get<0>(minima), std::get<0>(maxima));
  int evenPlaneRowCount = LsEvenCount(std::get<1>(minima), std::get<1>(maxima));
  int oddPlaneRowCount = LsOddCount(std::get<1>(minima), std::get<1>(maxima));
  int evenPlaneSize = evenPlaneRowCount*evenPlaneRowSize;
  int oddPlaneSize = oddPlaneRowCount*oddPlaneRowSize;
  int planeOffset = evenPlaneSize*evenPlanesCount + oddPlaneSize*oddPlanesCount;
  int rowOffset;
  int columnOffset;
  if ( LsEven(std::get<2>(node)) ) {
    rowOffset = LsEvenCount(std::get<1>(minima), std::get<1>(node) - 1)*evenPlaneRowSize;
    columnOffset = LsEvenCount(std::get<0>(minima), std::get<0>(node) - 1);
  } else {
    rowOffset = LsOddCount(std::get<1>(minima), std::get<1>(node) - 1)*oddPlaneRowSize;
    columnOffset = LsOddCount(std::get<0>(minima), std::get<0>(node) - 1);
  }
  return planeOffset + rowOffset + columnOffset;
}

bool LsBCCLattice::WithinBounds(LsBCCNode node) const {
  return std::get<0>(node) >= std::get<0>(minima) &&
         std::get<1>(node) >= std::get<1>(minima) &&
         std::get<2>(node) >= std::get<2>(minima) &&
         std::get<0>(node) <= std::get<0>(maxima) &&
         std::get<1>(node) <= std::get<1>(maxima) &&
         std::get<2>(node) <= std::get<2>(maxima);
}

bool LsBCCLattice::Valid(LsBCCNode node) const {
  return LsEven(std::get<0>(node)) && LsEven(std::get<1>(node)) && LsEven(std::get<2>(node)) ||
         LsOdd(std::get<0>(node)) && LsOdd(std::get<1>(node)) && LsOdd(std::get<2>(node));
}

bool LsBCCLattice::NodeExists(LsBCCNode node) const {
  return WithinBounds(node) && Valid(node);
}
