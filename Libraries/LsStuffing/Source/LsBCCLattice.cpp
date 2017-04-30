//===============================================================================
// @ LsBccLattice.h
// 
// Body Centered Cubic Lattice data structure
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsBCCLatticeTypes.h>
#include <LsBCCLattice.h>
#include <LsMath.h>
#include <assert.h>
#include <cmath>

#include <iostream> // test only

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

// We iterate all tetrahedra following a tiling pattern. The pattern has it's origin at some black node.
// The pattern has three axes along x, y and z with with adjacent black nodes at 2 units offsets.
// Surrounding each of those 3 axes are 4 tetrahedra, making up a pattern of total 12 tetrahedra.
// To iterate over all tetrahedra we iterate over all black vertecies and then over every tetrahedra in
// the pattern.
static const LsBCCNodeOffset tetrahedraOffsets[12][3] = {
  {{2,0,0},{1,1,-1},{1,-1,-1}}, // x-axis
  {{2,0,0},{1,1,1},{1,1,-1}},
  {{2,0,0},{1,-1,1},{1,1,1}},
  {{2,0,0},{1,-1,-1},{1,-1,1}},
  {{0,2,0},{-1,1,-1},{1,1,-1}}, // y-axis
  {{0,2,0},{1,1,1},{1,1,-1}},
  {{0,2,0},{-1,1,1},{1,1,1}},
  {{0,2,0},{-1,1,1},{-1,1,-1}},
  {{0,0,2},{-1,1,1},{1,1,1}},   // z-axis
  {{0,0,2},{1,1,1},{1,-1,1}},
  {{0,0,2},{-1,-1,1},{1,-1,1}},
  {{0,0,2},{-1,1,1},{-1,-1,1}}
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

LsBCCLattice::NodeIterator::NodeIterator(LsBCCLattice const& lattice):lattice(&lattice) {
  current = this->lattice->nodeMetaData[currentIndex].coordinates;
};

//LsBCCLattice::NodeIterator const& LsBCCLattice::NodeIterator::operator=(LsBCCLattice::NodeIterator const& other) {
//  this->lattice = other.lattice;
//}

LsBCCLattice::NodeIterator::operator LsBCCNode() const {
  return current;
}

bool LsBCCLattice::NodeIterator::Next() {
  ++currentIndex;
  if ( currentIndex < lattice->nodeMetaData.size() )
  {
    current = lattice->nodeMetaData[currentIndex].coordinates;
    return true; 
  } else {
    return false;
  }
}

LsBCCLattice::TetrahedronIterator::TetrahedronIterator(LsBCCLattice const& lattice):lattice(&lattice) {
  // Iteration pattern goes in steps of two, so to make sure all tetrahedra get the coverage
  // we have to expand lattice bounds into even, if they already aren't.
  minima.x = std::get<0>(this->lattice->minima);
  minima.y = std::get<1>(this->lattice->minima);
  minima.z = std::get<2>(this->lattice->minima);
  if (LsOdd(minima.x)) minima.x = std::get<0>(this->lattice->minima) - 1;
  if (LsOdd(minima.y)) minima.y = std::get<1>(this->lattice->minima) - 1;
  if (LsOdd(minima.z)) minima.z = std::get<2>(this->lattice->minima) - 1;
  maxima.x = std::get<0>(this->lattice->maxima);
  maxima.y = std::get<1>(this->lattice->maxima);
  maxima.z = std::get<2>(this->lattice->maxima);
  if (LsOdd(maxima.x)) maxima.x = std::get<0>(this->lattice->maxima) + 1;
  if (LsOdd(maxima.y)) maxima.y = std::get<1>(this->lattice->maxima) + 1;
  if (LsOdd(maxima.z)) maxima.z = std::get<2>(this->lattice->maxima) + 1;
  currentNode.x = minima.x; // It's normal if the currentNode is not part of the lattice, at the boundaries we have to
  currentNode.y = minima.y; // outside the grid to apply the pattern. The node mast be valid though(all even or all odd coordinates).
  currentNode.z = minima.z;
  assert(this->lattice->Valid(std::make_tuple(currentNode.x, currentNode.y, currentNode.z)));
  currentPatternIndex = -1;
  Next();
};

LsBCCLattice::TetrahedronIterator::operator LsBCCTetrahedron() const {
  return currentTetrahedron;
}

void printNode(LsBCCNode& node) {
  std::cout << "(" << std::get<0>(node) << ", " << std::get<1>(node) << ", " << std::get<2>(node) << ") ";
}

bool LsBCCLattice::TetrahedronIterator::Next() {
  ++currentPatternIndex;
  if ( currentPatternIndex < 12 ) {
    LsBCCNode n1 = { currentNode.x, currentNode.y, currentNode.z };
    LsBCCNode n2 = AddNodeOffset(n1, tetrahedraOffsets[currentPatternIndex][0]);
    LsBCCNode n3 = AddNodeOffset(n1, tetrahedraOffsets[currentPatternIndex][1]);
    LsBCCNode n4 = AddNodeOffset(n1, tetrahedraOffsets[currentPatternIndex][2]);
    currentTetrahedron = { n1, n2, n3, n4 };
    if (lattice->NodeExists(n1) && lattice->NodeExists(n2) && lattice->NodeExists(n3) && lattice->NodeExists(n4)) {
      return true;
    }
    else {
      return Next(); // Try to find another node
    }
  } else {
    currentNode.x += 2;
    if (currentNode.x > maxima.x) {
      currentNode.x = minima.x;
      currentNode.y += 2;
      if (currentNode.y > maxima.y) {
        currentNode.y = minima.y;
        currentNode.z += 2;
        if (currentNode.z > maxima.z) {
          return false; // No more nodes
        }
      }
    }
    currentPatternIndex = -1;
    return Next();
  }
};

LsBCCLattice::NodeEdgeIterator::NodeEdgeIterator(LsBCCLattice const& lattice, LsBCCNode node):lattice(&lattice),n1(node) {
  assert( this->lattice->NodeExists(node) );
  currentAdjacentIndex = -1; // Use Next() method to find first valid edge,
  Next();                    // in case if first edge doesn't exist
};

//-------------------------------------------------------------------------------
// @ LsBCCLattice::NodeEdgeIterator::operator LsBCCEdge() 
//-------------------------------------------------------------------------------
// Get the current edge. The origin node is guaranteed to be the first node of the edge.
//-------------------------------------------------------------------------------
LsBCCLattice::NodeEdgeIterator::operator LsBCCEdge() const {
  return current; //TODO: assert that this is valid
}

bool LsBCCLattice::NodeEdgeIterator::Next() {
  ++currentAdjacentIndex;
  if ( currentAdjacentIndex < 14 )
  {
    LsBCCNode n2 = AddNodeOffset(n1, adjacentOffsets[currentAdjacentIndex]);
    current = LsBCCEdge(n1, n2);
    if (lattice->NodeExists(n2))
    {
      return true;
    }
    return Next(); // Keep looking for next valid edge
  } else {
    return false;
  }
}

LsBCCLattice::EdgeIterator::EdgeIterator(LsBCCLattice const& lattice):lattice(&lattice), nodeIterator(lattice.GetNodeIterator()) {
  currentNexusIndex = -1; 
  Next();
}

LsBCCLattice::EdgeIterator::operator LsBCCEdge() const {
  return current;
}

bool LsBCCLattice::EdgeIterator::Next() {
  ++currentNexusIndex;
  if ( currentNexusIndex < 7 ) {
    LsBCCNode n1 = nodeIterator;
    LsBCCNode n2 = AddNodeOffset(n1, nexusOffsets[currentNexusIndex]);
    current = LsBCCEdge(n1, n2);
    if (lattice->NodeExists(n2))
    {
      return true;
    }
    return Next(); // Keep looking for next valid edge
  }
  else {
    if ( nodeIterator.Next() ) {
      currentNexusIndex = -1; // Recursive call to Next() will make it 0
      return Next();
    }
    else {
      return false; // No more nodes
    }
  }
}

LsBCCLattice::LsBCCLattice(LsDomain domain, float step) {
  // TODO: Implement this in terms of previous constructor code. Domain is a floating 
  // point bounding box. The grid must include every tetrahedron that is touched by it.
  // It can be accomplished simply by rounding up by step size outwards.
  std::tuple<int, int, int> minima = { floor(domain.x1 / step)-1, floor(domain.y1 / step)-1, floor(domain.z1 / step)-1 };
  std::tuple<int, int, int> maxima = { ceil(domain.x2 / step)+1, ceil(domain.y2 / step)+1, ceil(domain.z2 / step)+1 };

  CreateLattice(minima, maxima, step); // TODO: floor, ceiling, round up and down to even
}

//-------------------------------------------------------------------------------
// @ LsBCCLattice::LsBCCLattice()
//-------------------------------------------------------------------------------
// Initialize lattice bounded by inclusive interval
//-------------------------------------------------------------------------------
LsBCCLattice::LsBCCLattice(std::tuple<int, int, int> minima,
                           std::tuple<int, int, int> maxima,
                           float step) {
  CreateLattice(minima, maxima, step);
}

LsBCCLattice::TetrahedronIterator LsBCCLattice::GetTetrahedronIterator() const {
  return TetrahedronIterator(*this);
}

LsBCCLattice::NodeIterator LsBCCLattice::GetNodeIterator() const {
  return NodeIterator(*this);
}

LsBCCLattice::NodeEdgeIterator LsBCCLattice::GetNodeEdgeIterator(LsBCCNode node) const {
  return NodeEdgeIterator(*this, node);
}

LsBCCLattice::EdgeIterator LsBCCLattice::GetEdgeIterator() const {
  return EdgeIterator(*this);
}

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
  NodeEdgeIterator iterator = GetNodeEdgeIterator(node);
  do {
    LsBCCEdge edge = iterator;
    GetEdgeMetaDataReference(edge);
    GetEdgeMetaDataReference(edge).cutPoint = LsOptional<glm::vec3>::None();;
  } while ( iterator.Next() );
}
 
LsOptional<glm::vec3> LsBCCLattice::GetEdgeCutPoint(LsBCCEdge edge) const {
  return GetEdgeMetaDataConstReference(edge).cutPoint;
}

void LsBCCLattice::SetEdgeCutPoint(LsBCCEdge edge, glm::vec3 position) {
  LsBCCValue v1 = GetNodeValue(std::get<0>(edge));
  LsBCCValue v2 = GetNodeValue(std::get<1>(edge));
  assert( v1 == LsBCCValue::ePositive && v2 == LsBCCValue::eNegative  ||
          v1 == LsBCCValue::eNegative && v2 == LsBCCValue::ePositive );
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

void LsBCCLattice::CreateLattice(std::tuple<int, int, int> minima,
  std::tuple<int, int, int> maxima,
  float step) {
  this->minima = minima;
  this->maxima = maxima;
  for (int z = std::get<2>(minima); z <= std::get<2>(maxima); ++z)
  {
    for (int y = std::get<1>(minima); y <= std::get<1>(maxima); ++y)
    {
      for (int x = std::get<0>(minima); x <= std::get<0>(maxima); ++x)
      {
        LsBCCNode node = { x,y,z };
        if (Valid(node))
        {
          NodeMetaData nodeInfo;
          nodeInfo.coordinates = node;
          nodeInfo.position = step*glm::vec3(x, y, z); // Vertex coordinates correspond to 
          nodeInfo.value = LsBCCValue::eUnassigned;    // BCC grid coordinates scaled by step
          nodeMetaData.push_back(nodeInfo);
        }
      }
    }
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

//-------------------------------------------------------------------------------
// @ LsBCCLattice::FindEdgeInNexus()
//-------------------------------------------------------------------------------
// Find out nexus node and nexus offset of an edge
//-------------------------------------------------------------------------------
void LsBCCLattice::FindEdgeInNexus(LsBCCEdge edge, LsBCCNode* nexusNode, int* nexusOffset) const {
  // Get vector representing an edge, assume first node is nexus
  LsBCCNodeOffset offset = SubtractNodes(std::get<1>(edge), std::get<0>(edge));

  // Test if edge matches one of nexus pattern edges
  for (int i = 0; i < 7; ++i)
  {
    if( NodeOffsetsEqual(nexusOffsets[i], offset) ) {
      if ( nexusNode ) *nexusNode = std::get<0>(edge);
      if ( nexusOffset ) *nexusOffset = i;
      return;
    }
  }

  // If still not found, assume second node is nexus
  offset = SubtractNodes(std::get<0>(edge), std::get<1>(edge));
  for (int i = 0; i < 7; ++i)
  {
    if( NodeOffsetsEqual(nexusOffsets[i], offset) ) {
      if ( nexusNode ) *nexusNode = std::get<1>(edge);
      if ( nexusOffset ) *nexusOffset = i;
      return;
    }
  }
  assert(false);
}

LsBCCLattice::NodeMetaData& LsBCCLattice::GetNodeMetaDataReference(LsBCCNode node) {
  return nodeMetaData[GetNodeIndex(node)];
}

LsBCCLattice::NodeMetaData const& LsBCCLattice::GetNodeMetaDataConstReference(LsBCCNode node) const {
  return nodeMetaData[GetNodeIndex(node)];
}

LsBCCLattice::EdgeMetaData& LsBCCLattice::GetEdgeMetaDataReference(LsBCCEdge edge) {
  LsBCCNode nexusNode;
  int edgeIndex;
  FindEdgeInNexus(edge, &nexusNode, &edgeIndex);
  return GetNodeMetaDataReference(nexusNode).edgeNexus[edgeIndex];
}

LsBCCLattice::EdgeMetaData const& LsBCCLattice::GetEdgeMetaDataConstReference(LsBCCEdge edge) const {
  LsBCCNode nexusNode;
  int edgeIndex;
  FindEdgeInNexus(edge, &nexusNode, &edgeIndex);
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
  if (std::get<0>(node) == 1 && std::get<1>(node) == -9 && std::get<2>(node) == -9) {
    std::cout << " ";
  }
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
