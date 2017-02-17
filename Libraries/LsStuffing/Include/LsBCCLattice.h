//===============================================================================
// @ LsBccLattice.h
// 
// Body Centered Cubic Lattice data structure
//
//===============================================================================

#pragma once

#include <vector>
#include <tuple>
#include <LsOptional.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <LsBCCLatticeTypes.h>

//-------------------------------------------------------------------------------
//-- Classes --------------------------------------------------------------------
//-------------------------------------------------------------------------------

class LsBCCLattice {
protected:
  struct EdgeMetaData {
    LsOptional<glm::vec3> cutPoint;
  };
  struct NodeMetaData {
    LsBCCNode coordinates;
    glm::vec3 position;
    LsBCCValue value;
    EdgeMetaData edgeNexus[7];
  };
public:
  class NodeIterator {
  friend LsBCCLattice;
  public:
    bool Next();
    operator LsBCCNode() const;
  private:
    NodeIterator(LsBCCLattice const& lattice);
    LsBCCLattice const* lattice;
    LsBCCNode current;
    size_t currentIndex = 0;
  };

  class TetrahedronIterator {
  friend LsBCCLattice;
  public:
    bool Next();
    operator LsBCCTetrahedron() const;
  private:
    TetrahedronIterator(LsBCCLattice const& lattice);
    LsBCCLattice const* lattice;
    struct {
      int x;
      int y;
      int z;
    } minima;
    struct {
      int x;
      int y;
      int z;
    } maxima;
    struct {
      int x;
      int y;
      int z;
    } currentNode;
    int currentPatternIndex;
    LsBCCTetrahedron currentTetrahedron;
  };

  class NodeEdgeIterator {
  friend LsBCCLattice;
  public:
    bool Next();
    operator LsBCCEdge() const;
  private:
    NodeEdgeIterator(LsBCCLattice const& lattice, LsBCCNode node);
    LsBCCLattice const* lattice;
    LsBCCNode n1;
    LsBCCEdge current;
    short currentAdjacentIndex = 0;
  };

  class EdgeIterator {
  friend LsBCCLattice;
  public:
    bool Next();
    operator LsBCCEdge() const;
  private:
    EdgeIterator(LsBCCLattice const& lattice);
    LsBCCLattice const* lattice;
	  NodeIterator nodeIterator;
	  LsBCCEdge current;
    int currentNexusIndex = 0;
  };
  LsBCCLattice(LsDomain domain);
  LsBCCLattice(std::tuple<int, int, int> minima, std::tuple<int, int, int> maxima, float step);

  TetrahedronIterator GetTetrahedronIterator() const; //   TODO: Use the cube tile
  NodeIterator GetNodeIterator() const;
  NodeEdgeIterator GetNodeEdgeIterator(LsBCCNode node) const;
  EdgeIterator GetEdgeIterator() const;               //   TODO: iterate over vertecies, iterate over nexus edges, use bounds to filter non-existent 

  glm::vec3 GetNodePosition(LsBCCNode node) const;
  LsBCCColor GetNodeColor(LsBCCNode node) const;
  LsBCCValue GetNodeValue(LsBCCNode node) const;
  void SetNodeValue(LsBCCNode node, LsBCCValue value);
  void SetNodePosition(LsBCCNode node, glm::vec3 position);
  void DeleteNodeCutPoints(LsBCCNode node);
  LsOptional<glm::vec3> GetEdgeCutPoint(LsBCCEdge edge) const;
  void SetEdgeCutPoint(LsBCCEdge edge, glm::vec3 position);
  LsBCCColor GetEdgeColor(LsBCCEdge edge) const;
protected:
  std::tuple<int, int, int> minima;
  std::tuple<int, int, int> maxima;
  std::vector<NodeMetaData> nodeMetaData;
  LsOptional<int> GetEdgeIndexInNexus(LsBCCEdge edge) const;
  void FindEdgeInNexus(LsBCCEdge edge, LsBCCNode* nexusNode, int* nexusOffset) const;
  NodeMetaData& GetNodeMetaDataReference(LsBCCNode node);
  NodeMetaData const& GetNodeMetaDataConstReference(LsBCCNode node) const;
  EdgeMetaData& GetEdgeMetaDataReference(LsBCCEdge edge);
  EdgeMetaData const& GetEdgeMetaDataConstReference(LsBCCEdge edge) const;
  int GetNodeIndex(LsBCCNode node) const;
  bool Valid(LsBCCNode node) const;
  bool WithinBounds(LsBCCNode node) const;
  bool NodeExists(LsBCCNode node) const;
};

// Lattice bounded by domain must contain every terahedra that has at least one of it's vertecies in the domain