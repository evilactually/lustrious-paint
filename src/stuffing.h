
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
    ePositive,
    eNegative,
    eZero,
    eUnassigned
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

  struct Tetrahedra {
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
    Optional<Node> Next() {
      if ( currentIndex >= nodeMetaData.size() )
      {
        return Optional<Node>::None();
      }
      return nodeMetaData[currentIndex++].coordinates;
    }
  private:
    NodeIterator(std::vector<NodeMetaData> const& nodeMetaData):nodeMetaData(nodeMetaData) { };
    std::vector<NodeMetaData> const& nodeMetaData;
    size_t currentIndex = 0;
  };

  // class NodeIterator: public std::iterator<std::forward_iterator_tag, Node> {
  // public:
  //   NodeIterator(std::vector<NodeMetaData> const& nodeMetaData):nodeMetaData(nodeMetaData) { };
  //   NodeIterator& operator++() { currentIndex++; return *this; }
  //   Node const& operator*() { return nodeMetaData[currentIndex].coordinates; }
  // private:
  //   std::vector<NodeMetaData> const& nodeMetaData;
  //   size_t currentIndex = 0;
  // };

  // class NodeIterator {
  // public:
  //   typedef NodeIterator self_type;
  //   typedef Node value_type;
  //   typedef std::forward_iterator_tag iterator_category;
  //   self_type operator++() { self_type i = *this; currentIndex++; return i; }
  //   value_type operator*() { return nodeMetaData[currentIndex].coordinates; }
  // private:
  //   NodeIterator(std::vector<NodeMetaData> const& nodeMetaData):nodeMetaData(nodeMetaData) { };
  //   std::vector<NodeMetaData> const& nodeMetaData;
  //   size_t currentIndex = 0;
  // };

  void GetTetrahedronIterator(); // TODO: ???
  NodeIterator GetNodeIterator();
  void GetNodeEdgeIterator();  //   TODO:
  void GetEdgeIterator();      //   TODO: iterate over vertecies, iterate over nexus edges, use bounds to filter non-existent 



  // Node info
  glm::vec3 GetNodePosition(Node node) const;
  Color GetNodeColor(Node node) const;
  Value GetNodeValue(Node node) const;
  void SetNodeValue(Node node, Value value);
  void SetNodePosition(Node node, glm::vec3 position);
  void DeleteNodeCutPoints(Node node);
 
  // Edge info
  Optional<glm::vec3> GetEdgeCutPoint(Edge edge) const;
  Color GetEdgeColor(Edge edge) const;
  void SetEdgeCutPoint(Edge edge, glm::vec3 position);

//public:
  

  const NodeOffset nexusOffsets[7] = {{1,1,1},{1,1,-1},{-1,1,-1},{-1,1,1},{2,0,0},{0,2,0},{0,0,2}};
  std::vector<NodeMetaData> nodeMetaData;
  BBox3i latticeBounds;
  
  //void GetNodeGridCoordinates(BCCLattice::Node n1, int& x, int& y, int& z);
  Optional<int> GetEdgeIndexInNexus(BCCLattice::Edge edge) const;
  Node GetEdgeNexusNode(Edge edge) const;
  NodeMetaData& GetNodeMetaDataReference(Node node);
  NodeMetaData const& GetNodeMetaDataConstReference(Node node) const;
  EdgeMetaData& GetEdgeMetaDataReference(Edge edge);
  EdgeMetaData const& GetEdgeMetaDataConstReference(Edge edge) const;
  int GetNodeIndex(int x, int y, int z) const;
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
  //std::cout << "*n1:" << n1  << std::endl;
  //std::cout << "*n2:" << n2  << std::endl;
  return {n1.x - n2.x, n1.y - n2.y, n1.z - n2.z};
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

// Unpacks storage efficient three-dimensional indecies into three-dimensional cartesian coordinates
// void BCCLattice::GetNodeGridCoordinates(BCCLattice::Node n1, int& x, int& y, int& z) {
//   int zigZag;

//   // Initialize
//   x = n1.x;
//   y = n1.y;
//   z = n1.z;

//   // Offset z and x by half grid length if y is odd
//   if ( y % 2 )
//   {
//     zigZag = 1;
//   } else {
//     zigZag = 0;
//   }

//   // Expand grid to size 2, so that half grid offset is 1
//   x *= 2;
//   z *= 2;

//   // Apply half grid size offset if needed
//   x += zigZag;
//   z += zigZag;
// }

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

int BCCLattice::GetNodeIndex(int x, int y, int z) const {
  assert(even(y) && even(x) && even(z) || odd(y) && odd(x) && odd(z));
  int evenPlanesCount = intervalEvenCount(latticeBounds.minZ, z - 1);
  int oddPlanesCount = intervalOddCount(latticeBounds.minZ, z - 1);
  int evenPlaneRowSize = intervalEvenCount(latticeBounds.minX,latticeBounds.maxX);
  int oddPlaneRowSize = intervalOddCount(latticeBounds.minX,latticeBounds.maxX);
  int evenPlaneRowCount = intervalEvenCount(latticeBounds.minY,latticeBounds.maxY);
  int oddPlaneRowCount = intervalOddCount(latticeBounds.minY,latticeBounds.maxY);
  int evenPlaneSize = evenPlaneRowCount*evenPlaneRowSize;
  int oddPlaneSize = oddPlaneRowCount*oddPlaneRowSize;
  int planeOffset = evenPlaneSize*evenPlanesCount + oddPlaneSize*oddPlanesCount;
  int rowOffset;
  int columnOffset;
  if ( even(z) ) {
    rowOffset = intervalEvenCount(latticeBounds.minY, y - 1)*evenPlaneRowSize;
    columnOffset = intervalEvenCount(latticeBounds.minX, x - 1);
  } else {
    rowOffset = intervalOddCount(latticeBounds.minY, y - 1)*oddPlaneRowSize;
    columnOffset = intervalOddCount(latticeBounds.minX, x - 1);
  }
  return planeOffset + rowOffset + columnOffset;
}

BCCLattice::NodeMetaData& BCCLattice::GetNodeMetaDataReference(Node node) {
  return nodeMetaData[GetNodeIndex(node.x, node.y, node.z)];
}

BCCLattice::NodeMetaData const& BCCLattice::GetNodeMetaDataConstReference(Node node) const {
  return nodeMetaData[GetNodeIndex(node.x, node.y, node.z)]; 
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



  // First row Red or Black?
  //even(minGridBound[1]);

  // Red nodes lay on odd coordinates
  //int redRowWidth = 


  // Grid bounds from bounding box
  // Expand grid bounds to include neighboring edges, by padding it by 2 on each side

  // How many vertecies - can be determined from bounds, since every vertex has a coordinate in space
  
  // how much to allocate?

  // * * * *
  //  * * * 
  // * * * *
  //  * * * 

  // gridWidth per row divided by 2?



  // int xMinIn = std::ceil(min.x);
  // int xMinOut = std::floor(min.x);
  // int xMaxIn = std::floor(max.x);
  // int xMaxOut = std::ceil(max.x);
  // int yMinIn = std::ceil(min.y);
  // int yMinOut = std::floor(min.y);
  // int yMaxIn = std::floor(max.y);
  // int yMaxOut = std::ceil(max.y);

  // std::cout << "xMinIn " << xMinIn << std::endl;
  // std::cout << "xMinOut " << xMinOut << std::endl;
  // std::cout << "xMaxIn " << xMaxIn << std::endl;
  // std::cout << "xMaxOut " << xMaxOut << std::endl;
  // std::cout << "yMinIn " << yMinIn << std::endl;
  // std::cout << "yMinOut " << yMinOut << std::endl;
  // std::cout << "yMaxIn " << yMaxIn << std::endl;
  // std::cout << "yMaxOut " << yMaxOut << std::endl;

//  float aaa[3];
  //ComputeGridBounds(&aaa);

  // * * * *
  //  * * * *
  // * * * *
  //  * * * *
  // int width = 10;
  // int height = 10;
  // int depth = 10;
  // nodeMetaData.reserve(width*height*depth);

  //nodeMetaData[x+y*width+z*depth]

  // even y -> even vertecies
  // odd y -> odd vertecies

  //   float xf = 0.0f;
  //   float yf = 0.0f;
  //   float zf = 0.0f;
  //   for (int z = 0; z < 2; ++z)
  //   {
  //     for (int y = 0; y < 3; ++y)
  //     {
  //       for (int x = 0; x < 2; ++x)
  //       {
  //         //std::cout << y << "(" << xf << ", " << yf << ", " << zf << ")" << std::endl;

  //         xf += step;
  //       }
  //       yf += 0.5*step;
  //       if ( (y + 1) % 2 )
  //       {
  //         xf = 0.5f*step;
  //       } else {
  //         xf = 0.0f;
  //       }
  //     }
  //     zf += step;
  //     yf = 0.0f;


    // first row xf += step, yf = 0.0
    // second row xf += 0.5*step + step, yf += 0.5*step
    // third row xf += step, yf += 0.5*step
    // forth row xf += 0.5*step + step, yf += 0.5*step
    // ...
    // zf += step

    // steps in x are whole steps starting with half step as initial value for even rows, and from zero for odd
    // vertical steps are always half the step, starting from zero
    // depth steps are whole steps, always

  //}

Optional<int> BCCLattice::GetEdgeIndexInNexus(BCCLattice::Edge edge) const {
  // Get vector representing an edge
  NodeOffset offset = edge.n2 - edge.n1;

  // Test if edge matches one of nexus pattern edges
  for (int i = 0; i < sizeof(nexusOffsets); ++i)
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

// bool operator>=(BCCLattice::Node const & n1, BCCLattice::NodeOffset const & n2) {
//   return n1.x >= n2.x && n1.y >= n2.y && n1.z >= n2.z;
// }

// edge information is stored in blocks of 7 (because that's the tiling element size)
// edge is specified as two vertecies
// each edge is stored in a unique nexus
// only one of the vertecies lays at the "storage" vertex of the edge
// of the two vertecies the one closer to origin is the nexus

// edge type
// nexus determination rule
// both edges black, 

// for each node on main grid there's a nexus for three black edges, that includes the red points
// in addition to that each point(both red and black) has a second nexus for three red edges
// nexus size = 2*(black points + red points) 
// each nexus entry stores 3 edges

// 1. Determine color of the edge
// 2. Determine Nexus Point
// 3. Look-up Nexus Entry
// 4. Look-up Edge Entry

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

void fff() {
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
  BCCLattice bbb(BBox3i(1, 1, 1, 70, 40, 40), 1.0f);

  std::cout << "offset " << bbb.GetNodeIndex(3,1,3) << std::endl;
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