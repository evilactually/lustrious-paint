
// evaluate level function for each vertex
// cut edges that have both positive and negative end points (requires iterating over edges)
// warp violated vertecies
// fill in stencils into tetrahedra (requires iterating over every lattice tetrahedra)


// Iterating over tetrahedra is a simple matter of generating sequences of four vertecies
// Iterating over edges is likewise a matter of generating sequence of vertex pairs

// Lattice grid is predetermined for every point is space
// The first step selects the subset of such points in a boundary domain

// Lattice
// Initialized from bound box
// Point iterator
// Edge iterator
// Tetrahedra iterator
// 

// Set 2 requirements:
// end points as coordinates (real indecies are an implementation detail)
// function values at end points
// Additional
// type of edge: red or black (no needed, but why not)
// cut point (distance from first point along second)

// polygon tests
// in polygon?
// out polygon?
// polygon-line intersection

#pragma once
#include "Optional.hpp"
#include <algorithm>
#include <iterator>
#include "tuples.h"
#include "math.h"
#include "BBox3f.h"
#include "BBox3i.h"

template< class InputIt, class UnaryPredicate>
Optional<int> find_index_if( InputIt first, InputIt last, UnaryPredicate q) {
  auto found_iter = find_if(first, last, q);
  if ( found_iter != last )
  {
    return Optional<int>(found_iter - first);
  }
  else 
  {
    return Optional<int>::None();
  }
}

class BCCLattice {
public:
  BCCLattice() { }
  BCCLattice(BBox3i bounds, float step);

  enum class Color
  {
    eRed,
    eBlack
  };
  
  enum class Value
  {
    eUnassigned = 0,
    eNegative = 1,
    eZero = 2,
    ePositive = 3
  };

  struct NodeOffset {
    int dx;
    int dy;
    int dz;
  };

  struct Node {
    int x;
    int y;
    int z;
  };

  struct Edge {
    Node n1;
    Node n2;
  };

  // Specifies the coordinates defining a tetrahedron
  struct Tetrahedron {
    Node n1;
    Node n2;
    Node n3;
    Node n4;
  };

  struct EdgeMetaData {
    Optional<glm::vec3> cutPoint;
  };

  struct NodeMetaData {
    Node coordinates;
    glm::vec3 position;
    Value value;
    EdgeMetaData edgeNexus[7];
  };

  class NodeIterator {
  friend BCCLattice;
  public:
    Optional<Node> Next();
  private:
    NodeIterator(std::vector<NodeMetaData> const& nodeMetaData):nodeMetaData(nodeMetaData) { };
    std::vector<NodeMetaData> const& nodeMetaData;
    size_t currentIndex = 0;
  };

  class TetrahedronIterator {
  public:
    TetrahedronIterator(BCCLattice const& lattice):lattice(lattice) { };
    Optional<Tetrahedron> Next() { return Optional<Tetrahedron>::None(); };
  private:
    BCCLattice const& lattice;
  };

  class NodeEdgeIterator {
  public:
    NodeEdgeIterator(BCCLattice const& lattice):lattice(lattice) { };
    Optional<Edge> Next() { return Optional<Edge>::None(); };
  private:
    BCCLattice const& lattice;
  };

  class EdgeIterator {
  public:
    EdgeIterator(BCCLattice const& lattice):lattice(lattice) { };
    Optional<Edge> Next();
  private:
    BCCLattice const& lattice;
    size_t currentNodeIndex = 0;
    int currentNexusIndex = 0;
  };

  TetrahedronIterator GetTetrahedronIterator(); // TODO: ???
  NodeIterator GetNodeIterator();
  NodeEdgeIterator GetNodeEdgeIterator();       //   TODO:
  EdgeIterator GetEdgeIterator();               //   TODO: iterate over vertecies, iterate over nexus edges, use bounds to filter non-existent 

  // Node info
  glm::vec3 GetNodePosition(Node node) const;
  Color GetNodeColor(Node node) const;
  Value GetNodeValue(Node node) const;
  void SetNodeValue(Node node, Value value);
  void SetNodePosition(Node node, glm::vec3 position); 
  void DeleteNodeCutPoints(Node node); // DeleteAdjacentCutPoints
 
  // Edge info
  Optional<glm::vec3> GetEdgeCutPoint(Edge edge) const;
  Color GetEdgeColor(Edge edge) const;
  void SetEdgeCutPoint(Edge edge, glm::vec3 position);

//public:
  
  static const int NEXUS_SIZE = 7;
  const NodeOffset nexusOffsets[NEXUS_SIZE] = {{1,1,1},{1,1,-1},{-1,1,-1},{-1,1,1},{2,0,0},{0,2,0},{0,0,2}};
  std::vector<NodeMetaData> nodeMetaData;
  BBox3i latticeBounds;
  
  //void GetNodeGridCoordinates(BCCLattice::Node n1, int& x, int& y, int& z);
  Optional<int> GetEdgeIndexInNexus(BCCLattice::Edge edge) const;
  Node GetEdgeNexusNode(Edge edge) const;
  NodeMetaData& GetNodeMetaDataReference(Node node);
  NodeMetaData const& GetNodeMetaDataConstReference(Node node) const;
  EdgeMetaData& GetEdgeMetaDataReference(Edge edge);
  EdgeMetaData const& GetEdgeMetaDataConstReference(Edge edge) const;
  int GetNodeIndex(Node node) const;
  bool NodeExists(Node node) const;
};

// void ComputeGridBounds(int3_t* min) {

// }

bool operator==(BCCLattice::NodeOffset const & o1, BCCLattice::NodeOffset const & o2) {
  return o2.dx == o1.dx && o2.dy == o1.dy && o2.dz == o1.dz;
}

std::ostream& operator<<(std::ostream& os, const BCCLattice::Node& n)
{
  os << "( " << n.x << ", " << n.y << ", " << n.z << " )";
  return os;
}

std::ostream& operator<<(std::ostream& os, const BCCLattice::NodeOffset& n)
{
  os << "( " << n.dx << ", " << n.dy << ", " << n.dz << " )";
  return os;
}

BCCLattice::NodeOffset operator-(BCCLattice::Node const & n1, BCCLattice::Node const & n2) {
  return {n1.x - n2.x, n1.y - n2.y, n1.z - n2.z};
}

BCCLattice::Node operator+(BCCLattice::Node const & node, BCCLattice::NodeOffset const & offset) {
  return {node.x + offset.dx, node.y + offset.dy, node.z + offset.dz};
}

BCCLattice::Node operator+(BCCLattice::NodeOffset const & offset, BCCLattice::Node const & node) {
  return node + offset;
}

BCCLattice::NodeOffset operator-(BCCLattice::NodeOffset const & o1, BCCLattice::NodeOffset const & o2) {
  return {o1.dx - o2.dx, o1.dy - o2.dy, o1.dz - o2.dz};
}

BCCLattice::Node operator+(BCCLattice::NodeOffset const & o1, BCCLattice::NodeOffset const & o2) {
  return {o1.dx + o2.dx, o1.dy + o2.dy, o1.dz + o2.dz};
}

Optional<BCCLattice::Node> BCCLattice::NodeIterator::Next() {
  if ( currentIndex >= nodeMetaData.size() )
  {
    return Optional<Node>::None();
  }
  return nodeMetaData[currentIndex++].coordinates;
}

Optional<BCCLattice::Edge> BCCLattice::EdgeIterator::Next() {
  if ( currentNodeIndex >= lattice.nodeMetaData.size() )
  {
    return Optional<Edge>::None();
  }
  Node n2;
  Edge e;
  do {
    Node n1 = lattice.nodeMetaData[currentNodeIndex].coordinates;
    NodeOffset offset = lattice.nexusOffsets[currentNexusIndex];
    n2 = n1 + offset;
    e = {n1, n2};
    currentNexusIndex = currentNexusIndex + 1;
    if ( currentNexusIndex >= NEXUS_SIZE ) {
      currentNexusIndex = 0;
      currentNodeIndex++;
    }
  } while (!lattice.NodeExists(n2));
  return e;
}

BBox3i ComputeGridBounds(BBox3f box3f) {
  BBox3i box3i;
  // Shrink floating point bounding box to integer grid
  box3i.minX = std::ceil(box3f.minX);
  box3i.minY = std::ceil(box3f.minY);
  box3i.minZ = std::ceil(box3f.minZ);
  box3i.maxX = std::floor(box3f.maxX);
  box3i.maxY = std::floor(box3f.maxY);
  box3i.maxZ = std::floor(box3f.maxZ);
  // Expand integer grid by two units to include all nodes that are sharing edges with interior points
  box3i.minX -= 2;
  box3i.minY -= 2;
  box3i.minZ -= 2;
  box3i.maxX += 2;
  box3i.maxY += 2;
  box3i.maxZ += 2;

  return box3i;
}

BCCLattice::BCCLattice(BBox3i bounds, float step) {
  latticeBounds = bounds;
  for (int z = bounds.minZ; z <= bounds.maxZ; ++z)
  {
    for (int y = bounds.minY; y <= bounds.maxY; ++y)
    {
      for (int x = bounds.minX; x <= bounds.maxX; ++x)
      {
        NodeMetaData nodeInfo;
        nodeInfo.coordinates = {x,y,z};
        nodeInfo.position = glm::vec3((x - bounds.minX)*step, (y - bounds.minY)*step, (z - bounds.minZ)*step);
        nodeInfo.value = Value::eUnassigned;
        if ( even(y) && even(x) && even(z) || odd(y) && odd(x) && odd(z) )
        {
          nodeMetaData.push_back(nodeInfo);
          char c;
          if ( even(y) && even(x) && even(z) )
          {
            c = 'b';
          } else {
            c = 'r';
          }
          std::cout << c;
        } else {
          std::cout << "_";
        }
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}

BCCLattice::NodeIterator BCCLattice::GetNodeIterator() {
  return NodeIterator(nodeMetaData);
}

BCCLattice::EdgeIterator BCCLattice::GetEdgeIterator() {
  return EdgeIterator(*this);
}

BCCLattice::TetrahedronIterator BCCLattice::GetTetrahedronIterator() {
  return TetrahedronIterator(*this);
}

glm::vec3 BCCLattice::GetNodePosition(Node node) const {
  return GetNodeMetaDataConstReference(node).position;
}

BCCLattice::Value BCCLattice::GetNodeValue(Node node) const {
  return GetNodeMetaDataConstReference(node).value;
}

BCCLattice::Color BCCLattice::GetNodeColor(BCCLattice::Node node) const {
  assert(even(node.y) && even(node.x) && even(node.z) || odd(node.y) && odd(node.x) && odd(node.z));
  // Node is black iff all of it's coordinates are even
  if ( even(node.x) )
  {
    return BCCLattice::Color::eBlack;
  } else {
    return BCCLattice::Color::eRed;
  }
}

void BCCLattice::SetNodeValue(Node node, Value value) {
  GetNodeMetaDataReference(node).value = value;
}

void BCCLattice::SetNodePosition(Node node, glm::vec3 position) {
  GetNodeMetaDataReference(node).position = position;
}

void BCCLattice::DeleteNodeCutPoints(Node node) {
  // TODO: iterate over edges of a vertex
}

int BCCLattice::GetNodeIndex(Node node) const {
  assert(NodeExists(node));
  int evenPlanesCount = intervalEvenCount(latticeBounds.minZ, node.z - 1);
  int oddPlanesCount = intervalOddCount(latticeBounds.minZ, node.z - 1);
  int evenPlaneRowSize = intervalEvenCount(latticeBounds.minX,latticeBounds.maxX);
  int oddPlaneRowSize = intervalOddCount(latticeBounds.minX,latticeBounds.maxX);
  int evenPlaneRowCount = intervalEvenCount(latticeBounds.minY,latticeBounds.maxY);
  int oddPlaneRowCount = intervalOddCount(latticeBounds.minY,latticeBounds.maxY);
  int evenPlaneSize = evenPlaneRowCount*evenPlaneRowSize;
  int oddPlaneSize = oddPlaneRowCount*oddPlaneRowSize;
  int planeOffset = evenPlaneSize*evenPlanesCount + oddPlaneSize*oddPlanesCount;
  int rowOffset;
  int columnOffset;
  if ( even(node.z) ) {
    rowOffset = intervalEvenCount(latticeBounds.minY, node.y - 1)*evenPlaneRowSize;
    columnOffset = intervalEvenCount(latticeBounds.minX, node.x - 1);
  } else {
    rowOffset = intervalOddCount(latticeBounds.minY, node.y - 1)*oddPlaneRowSize;
    columnOffset = intervalOddCount(latticeBounds.minX, node.x - 1);
  }
  return planeOffset + rowOffset + columnOffset;
}

bool BCCLattice::NodeExists(Node node) const {
  return latticeBounds.in(node.x, node.y, node.z) && 
         (even(node.y) && even(node.x) && even(node.z) || odd(node.y) && odd(node.x) && odd(node.z));
}

BCCLattice::NodeMetaData& BCCLattice::GetNodeMetaDataReference(Node node) {
  return nodeMetaData[GetNodeIndex(node)];
}

BCCLattice::NodeMetaData const& BCCLattice::GetNodeMetaDataConstReference(Node node) const {
  return nodeMetaData[GetNodeIndex(node)]; 
}

BCCLattice::EdgeMetaData& BCCLattice::GetEdgeMetaDataReference(Edge edge) {
  Node nexusNode = GetEdgeNexusNode(edge);
  int edgeIndex = GetEdgeIndexInNexus(edge);
  return GetNodeMetaDataReference(nexusNode).edgeNexus[edgeIndex];
}

BCCLattice::EdgeMetaData const& BCCLattice::GetEdgeMetaDataConstReference(Edge edge) const {
  Node nexusNode = GetEdgeNexusNode(edge);
  int edgeIndex = GetEdgeIndexInNexus(edge);
  return GetNodeMetaDataConstReference(nexusNode).edgeNexus[edgeIndex]; 
}

Optional<int> BCCLattice::GetEdgeIndexInNexus(BCCLattice::Edge edge) const {
  // Get vector representing an edge
  NodeOffset offset = edge.n2 - edge.n1;

  // Test if edge matches one of nexus pattern edges
  for (int i = 0; i < NEXUS_SIZE; ++i)
  {
    if( nexusOffsets[i] == offset ) {
      return i;
    }
  }
  return Optional<int>::None();
}

// Every edge has one of it's nodes designated to be used as storage of edge information, called nexus node
BCCLattice::Node BCCLattice::GetEdgeNexusNode(BCCLattice::Edge edge) const {
  if ( GetEdgeIndexInNexus(edge) )
  {
    return edge.n1;
  }
  // If edge didn't match any pattern it must be flipped
  return edge.n2;
}

BCCLattice::Color BCCLattice::GetEdgeColor(BCCLattice::Edge edge) const {
  // Edge is black if both it's points are red or both it's points are black, otherwise it's red
  BCCLattice::Color color1 = GetNodeColor(edge.n1);
  BCCLattice::Color color2 = GetNodeColor(edge.n2);
  if ( color1 == BCCLattice::Color::eRed && color2 == BCCLattice::Color::eRed )
  {
    return BCCLattice::Color::eBlack; 
  } else if ( color1 == BCCLattice::Color::eBlack && color2 == BCCLattice::Color::eBlack ) {
    return BCCLattice::Color::eBlack;
  } else {
    return BCCLattice::Color::eRed;
  }
}

void BCCLattice::SetEdgeCutPoint(BCCLattice::Edge edge, glm::vec3 position) {
  GetEdgeMetaDataReference(edge).cutPoint = position;
}

typedef float(*LevelFunctionPfn)(glm::vec3);

void EvaluateLevelFunction(BCCLattice& lattice, LevelFunctionPfn fn) {
  // iterate over nodes
  // set function values
}

void DetermineCutPoints() {
  
}


// 1. Match the stencil to tetrahedra
// a) match by signs
// b) match by black edges


// struct ValuePattern {
//   BCCLattice::Value values[4];
// };

// bool operator==(ValuePattern const & p1, ValuePattern const & p2) {
//   return std::equal(&p1.values[0], &p1.values[3], &p2.values[0]);
// }

// ValuePattern GetTerahedronValuePattern(BCCLattice const& lattice, Tetrahedron)

// // int MatchStencil(Tetrahedron t, BCCLattice const& lattice) {

// //   return 0;
// // }

class Stencil {
public:
  Stencil();
  void Output(BCCLattice const& lattice, BCCLattice::Tetrahedron const& tetrahedron, std::vector<glm::vec3>* tetrahedra);
  bool Match(BCCLattice const& lattice, BCCLattice::Tetrahedron const& tetrahedron);
};


// Quadratic-time repack routine
// Take single list of vertecies and convert them into a list of vertecies and indecies
// removing duplicate vertecies along the way



// auto v_duplicate = find_if(tetrahedra.begin(), tetrahedra.end(), eq_pred);
//     if ( v_duplicate != tetrahedra.end() )
//     {
//       return Optional<int>(v_duplicate - tetrahedra.begin());
//     } else {
//       Optional<int>::None();
//     }

void BuildMeshBuffers(std::vector<glm::vec3>const& tetrahedra, std::vector<glm::vec3>& vertecies, std::vector<int>& indecies) {
  indecies.reserve(tetrahedra.size());
  for (auto v_src:tetrahedra)
  {
    auto epsilon_eq = [](float a, float b) { return float_near_eq(a, b, 0.00001f); };
    auto eq_pred = [&](glm::vec3 v_dst) { return epsilon_eq(v_dst[0], v_src[0]) && 
                                                 epsilon_eq(v_dst[1], v_src[1]) &&
                                                 epsilon_eq(v_dst[2], v_src[2]); };
    auto maybeIndex = find_index_if(tetrahedra.begin(), tetrahedra.end(), eq_pred);
    int finalIndex;
    if ( !maybeIndex )
    {
      vertecies.push_back(v_src);
      finalIndex = vertecies.size() - 1;
    } else {
      finalIndex = maybeIndex;
    }
    indecies.push_back(finalIndex);
  }
}


// For each vertex of tetrahedra
//   Is it equal to any other in the vertex array?
//   No? Push it to vertex array and get it's index.
//   Yes? Get it's index.
//   Put index in place of vertex.

//   a a a b b b c c c
//   1 4 5 1 2 4 5 1 3 

//   a a a b b b c c c
//         _

//   1 4 5 _ 2 _ _ _


class StencilA;
class StencilB;
class StencilC;

class Tetrahedron {
  // edges
  // cut points
  // values
  // nodes
};


class IsosurfaceStuffer {
public:
bool MatchValuePattern(BCCLattice::Tetrahedron tetrahedron, std::array<BCCLattice::Value, 4> pattern) {
  BCCLattice::Value tetrahedronValues[4];
  tetrahedronValues[0] = lattice.GetNodeValue(tetrahedron.n1);
  tetrahedronValues[1] = lattice.GetNodeValue(tetrahedron.n2);
  tetrahedronValues[2] = lattice.GetNodeValue(tetrahedron.n3);
  tetrahedronValues[3] = lattice.GetNodeValue(tetrahedron.n4);
  std::sort(&tetrahedronValues[0], &tetrahedronValues[3]);
  std::sort(&pattern[0], &pattern[3]); // TODO: why do i sort it every time? Move in a class, pre-sort?
  return std::equal(&pattern[0], &pattern[3], &tetrahedronValues[0]);
}
void TriangulateLattice() {
  BCCLattice::TetrahedronIterator tetrahedronIterator = lattice.GetTetrahedronIterator();
  Optional<BCCLattice::Tetrahedron> maybeTetrahedron;
  while ( maybeTetrahedron = tetrahedronIterator.Next() ) {
    BCCLattice::Tetrahedron tetrahedron = maybeTetrahedron;
    if (MatchValuePattern(tetrahedron, { BCCLattice::Value::ePositive,
                                         BCCLattice::Value::ePositive,
                                         BCCLattice::Value::ePositive,
                                         BCCLattice::Value::ePositive })) {

    } else if (MatchValuePattern(tetrahedron, { BCCLattice::Value::ePositive,
                                                BCCLattice::Value::ePositive,
                                                BCCLattice::Value::ePositive,
                                                BCCLattice::Value::eZero })) {

    }
  }
}
std::vector<glm::vec3> vertecies;
BCCLattice& lattice;
};

//bool MatchTetrahedronValues(BCCLattice)


void fff() {
  assert(BCCLattice::Value::ePositive > BCCLattice::Value::eNegative);
  assert(BCCLattice::Value::ePositive > BCCLattice::Value::eZero);
  assert(BCCLattice::Value::eNegative < BCCLattice::Value::eZero);
  assert(BCCLattice::Value::eNegative < BCCLattice::Value::ePositive);

  floatx3_t dd = {1.0f, 1.0f, 1.0f};
  BCCLattice aaa;
  //BCCLattice::Node n1 = {0,0,0};
  //BCCLattice::Node n1 = {1,0,0};
  //BCCLattice::Edge eee = {{0,0,0}, {1,1,1}};
  
  // Black
  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {2,0,0}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {0,2,0}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {0,0,2}}) << std::endl;

  // Black flipped
  std::cout << aaa.GetEdgeNexusNode({{-2,0,0}, {0,0,0}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{0,-2,0}, {0,0,0}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{0,0,-2}, {0,0,0}}) << std::endl;

  // Normal reds
  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {1,1,1}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {1,1,-1}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {-1,1,-1}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {-1,1,1}}) << std::endl;

  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {-1,-1,-1}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {-2,0,0}}) << std::endl;
  std::cout << aaa.GetEdgeNexusNode({{2,0,0}, {0,0,0}}) << std::endl;

  // // Changed order(should not change)
  // std::cout << aaa.GetEdgeNexusNode({{0,1,0}, {0,0,0}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{0,1,-1}, {0,0,0}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{-1,1,-1}, {0,0,0}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{-1,1,0}, {0,0,0}}) << std::endl;

  // // Changed signs (will change)
  // std::cout << aaa.GetEdgeNexusNode({{0,-1,0}, {0,0,0}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{0,-1,-1}, {0,0,0}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{-1,-1,-1}, {0,0,0}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{-1,-1,0}, {0,0,0}}) << std::endl;

  // std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {0,-1,0}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {0,-1,-1}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {-1,-1,-1}}) << std::endl;
  // std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {-1,-1,0}}) << std::endl;

  floatx3_t hhh = {-10.1f,-10.1f,-10.0f};
  floatx3_t jjj = {10.0f,10.1f,10.0f};
  BCCLattice bbb(BBox3i(1, 1, 1, 7, 4, 4), 1.0f);

  std::cout << "offset " << bbb.GetNodeIndex({3,1,3}) << std::endl;
  // bbb.GetNodeIndex(2,3,1);
  // bbb.GetNodeIndex(2,3,2);
  // bbb.GetNodeIndex(2,3,3);
  //std::cout << aaa.GetEdgeNexusNode({{0,0,0}, {-1,-1,0}}) << std::endl;

  //std::cout << aaa.GetEdgeNexusNode({{0,-1,0}, {0,0,0}}) << std::endl;
  //std::cout << aaa.GetEdgeNexusNode({{0,3,0}, {0,0,0}}) << std::endl;
  //std::cout << aaa.GetEdgeNexusNode({{0,-1,-1}, {0,0,0}}) << std::endl;
  //std::cout << aaa.GetEdgeNexusNode({{-1,-1,-1}, {0,0,0}}) << std::endl;
  //std::cout << aaa.GetEdgeNexusNode({{-1,-1,0}, {0,0,0}}) << std::endl;
  // std::cout << lowest<BCCLattice::Node>({1,2,1}, {2,3,2}) << std::endl;
  // std::cout << lowest<BCCLattice::Node>({1,2,1}, {2,3,1}) << std::endl;

  // std::cout << lowest<BCCLattice::Node>({1,2,1}, {2,1,2}) << std::endl;
  // std::cout << lowest<BCCLattice::Node>({2,1,2}, {1,2,1}) << std::endl;
  // BCCLattice lat(glm::vec3(), glm::vec3(), 1.0f);

  // for (int z = 0; z < 2; ++z)
  // {
  //   for (int y = 0; y < 3; ++y)
  //   {
  //     for (int x = 0; x < 2; ++x)
  //     {
  //       //std::cout << y << "(" << xf << ", " << yf << ", " << zf << ")" << std::endl;
  //       if ( lat.GetNodeColor({x,y,z}) == BCCLattice::Color::eRed ) {
  //         std::cout << "red" << std::endl;
  //       } else {
  //         std::cout << "black" << std::endl;
  //       }
  //     }
  //   }
  // }

  std::cout << intervalOddCount(0,5) << std::endl;
  std::cout << intervalOddCount(1,6) << std::endl;
  std::cout << intervalOddCount(1,7) << std::endl;
  std::cout << intervalOddCount(0,7) << std::endl;
  std::cout << intervalOddCount(-1,7) << std::endl;
  std::cout << intervalEvenCount(0,6) << std::endl;
  std::cout << intervalEvenCount(0,8) << std::endl;

  std::cout << (float)2 << std::endl;

  auto nodeIterator = bbb.GetNodeIterator();
  Optional<BCCLattice::Node> node;
  while( node = nodeIterator.Next() ) {
    //std::cout << node << std::endl;
    BCCLattice::Node n = node;
  }

  //auto iter = std::iterator<std::forward_iterator_tag, BCCLattice::Node>::begin();
  // auto iter = bbb.GetNodeIterator();
  // for (std::vector<>::iterator i = .begin(); i != .end(); ++i)
  // {
    
  // }
}