
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

#pragma once
#include "Optional.hpp"
#include <algorithm>

class BCCLattice {
public:
  BCCLattice(glm::vec3 min, glm::vec3 max, float step) {
    float xf = 0.0f;
    float yf = 0.0f;
    float zf = 0.0f;
    for (int z = 0; z < 2; ++z)
    {
      for (int y = 0; y < 3; ++y)
      {
        for (int x = 0; x < 2; ++x)
        {
          //std::cout << y << "(" << xf << ", " << yf << ", " << zf << ")" << std::endl;

          xf += step;
        }
        yf += 0.5*step;
        if ( (y + 1) % 2 )
        {
          xf = 0.5f*step;
        } else {
          xf = 0.0f;
        }
      }
      zf += step;
      yf = 0.0f;
    }

    // first row xf += step, yf = 0.0
    // second row xf += 0.5*step + step, yf += 0.5*step
    // third row xf += step, yf += 0.5*step
    // forth row xf += 0.5*step + step, yf += 0.5*step
    // ...
    // zf += step

    // steps in x are whole steps starting with half step as initial value for even rows, and from zero for odd
    // vertical steps are always half the step, starting from zero
    // depth steps are whole steps, always

  }

  enum class Color
  {
    eRed,
    eBlack
  };
  
  struct NodeOffset {
    int xOffset;
    int yOffset;
    int zOffset;
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

  class EdgeIterator {

  };

  void GetTetrahedronIterator();
  void GetVertexIterator();
  void GetEdgeIterator();

  // Node info
  glm::vec3 GetNodePosition(Node node);
  Color GetNodeColor(Node node);
  float GetNodeValue(Node node);

  // Node actions
  void SetNodeValue(Node node, float value);
  void DeleteNode(Node node);
  void UndeleteNode(Node node);
  void WarpNode(Node node, glm::vec3 position);
  void DeleteNodeCutPoints(Node node);

  // Edge info
  Optional<glm::vec3> GetEdgeCutPoint(Edge edge);
  Color GetEdgeColor(Edge edge);

  // Edge actions
  void SetEdgeCutPoint(Edge edge, glm::vec3 position);

private:
  struct EdgeMetaData {
    glm::vec3 cutPoint;
  };

  // Nexus is a unit of storage of edge information.
  struct Nexus
  {
    EdgeMetaData black[3];
    EdgeMetaData red[3];
  };

  Node GetEdgeNexusNode(Edge edge);
};

BCCLattice::NodeOffset operator-(BCCLattice::Node const & n1, BCCLattice::Node const & n2) {
  return {n2.x - n1.x, n2.y - n1.y, n2.z - n1.z};
}

bool operator==(BCCLattice::NodeOffset const & o1, BCCLattice::NodeOffset const & o2) {
  return o2.xOffset == o1.xOffset && o2.yOffset == o1.yOffset && o2.zOffset == o1.zOffset;
}

std::ostream& operator<<(std::ostream& os, const BCCLattice::Node& n)
{
  os << "( " << n.x << ", " << n.y << ", " << n.z << " )";
  return os;
}

BCCLattice::Node BCCLattice::GetEdgeNexusNode(BCCLattice::Edge edge) {
  BCCLattice::Color color = GetEdgeColor(edge);
  // Black edges only have one coordinate that's different. Node having the smaller one is the nexus.
  // Red edges
  return {0,0,0};
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
void BCCLattice::SetEdgeCutPoint(BCCLattice::Edge edge, glm::vec3 position) {
  BCCLattice::Color color = GetEdgeColor(edge);
  //Node n = std::min(edge.n1, edge.n2);
  //std::cout << n << std::endl;
  // if ( color == BCCLattice::Color::eBlack ) {
  //   // Pick the smallest node to represent nexus node of the edge
  //   Node n = std::min(edge.n1, edge.n2);
  //   // look-up black nexus at those x y z
  //   // width*height*z + width*y + x
  //   NodeOffset offset = edge.n1 - edge.n2;
  //   //NodeOffset  = {0,0,0};
    

  // }
}

BCCLattice::Color BCCLattice::GetNodeColor(BCCLattice::Node node) {
  // Node is red iff it's y index coordinate is odd
  if ( node.y % 2 )
  {
    return BCCLattice::Color::eRed;
  } else {
    return BCCLattice::Color::eBlack;
  }
}

BCCLattice::Color BCCLattice::GetEdgeColor(BCCLattice::Edge edge) {
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

typedef float(*LevelFunctionPfn)(glm::vec3);

void EvaluateLevelFunction(BCCLattice& lattice, LevelFunctionPfn fn) {
  // iterate over nodes
  // set function values
}

void fff() {
  // std::cout << lowest<BCCLattice::Node>({1,2,1}, {1,3,2}) << std::endl;
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
}