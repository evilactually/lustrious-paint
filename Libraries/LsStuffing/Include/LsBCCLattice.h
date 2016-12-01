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
#include <LsVector3.h>

//-------------------------------------------------------------------------------
//-- Typedefs -------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsBCCNode
//-------------------------------------------------------------------------------
// Specifies coordinates of a node
//-------------------------------------------------------------------------------
typedef std::tuple<int, int, int> LsBCCNode;

//-------------------------------------------------------------------------------
// @ LsBCCEdge
//-------------------------------------------------------------------------------
// Specifies coordinates of nodes defining an edge
//-------------------------------------------------------------------------------
typedef std::tuple<LsBCCNode, LsBCCNode> LsBCCEdge;

//-------------------------------------------------------------------------------
// @ LsBCCTetrahedron
//-------------------------------------------------------------------------------
// Specifies coordinates of nodes defining a tetrahedron
//-------------------------------------------------------------------------------
typedef std::tuple<LsBCCNode, LsBCCNode, LsBCCNode, LsBCCNode> LsBCCTetrahedron;


//-------------------------------------------------------------------------------
//-- Enums ----------------------------------------------------------------------
//-------------------------------------------------------------------------------
enum class LsBCCColor
{
  eRed,
  eBlack
};

enum class LsBCCValue
{
  eUnassigned = 0,
  eNegative = 1,
  eZero = 2,
  ePositive = 3
};

//-------------------------------------------------------------------------------
//-- Classes --------------------------------------------------------------------
//-------------------------------------------------------------------------------

class LsBCCLattice {
protected:
  struct EdgeMetaData {
    LsOptional<LsVector3> cutPoint;
  };
  struct NodeMetaData {
    LsBCCNode coordinates;
    LsVector3 position;
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
    LsBCCLattice const& lattice;
    LsBCCNode current;
    size_t currentIndex = 0;
  };

  class TetrahedronIterator {
  friend LsBCCLattice;
  public:
    LsOptional<LsBCCTetrahedron> Next();
    //operator LsBCCTetrahedron() const;
  private:
    TetrahedronIterator(LsBCCLattice const& lattice);
    LsBCCLattice const& lattice;
  };

  class NodeEdgeIterator {
  friend LsBCCLattice;
  public:
    bool Next();
    operator LsBCCEdge() const;
  private:
    NodeEdgeIterator(LsBCCLattice const& lattice, LsBCCNode node);
    LsBCCLattice const& lattice;
    LsBCCNode n1;
    LsBCCEdge current;
    short currentAdjacentIndex = 0;
  };

  class EdgeIterator {
  friend LsBCCLattice;
  public:
    LsOptional<LsBCCEdge> Next();
    //operator LsBCCEdge() const;
  private:
    EdgeIterator(LsBCCLattice const& lattice);
    LsBCCLattice const& lattice;
    size_t currentNodeIndex = 0;
    int currentNexusIndex = 0;
  };
  
  LsBCCLattice(std::tuple<int, int, int> minima, std::tuple<int, int, int> maxima, float step);
  TetrahedronIterator GetTetrahedronIterator(); //   TODO: WTF DO I DO???
  NodeIterator GetNodeIterator();
  NodeEdgeIterator GetNodeEdgeIterator(LsBCCNode node);
  EdgeIterator GetEdgeIterator();               //   TODO: iterate over vertecies, iterate over nexus edges, use bounds to filter non-existent 
  LsVector3 GetNodePosition(LsBCCNode node) const;
  LsBCCColor GetNodeColor(LsBCCNode node) const;
  LsBCCValue GetNodeValue(LsBCCNode node) const;
  void SetNodeValue(LsBCCNode node, LsBCCValue value);
  void SetNodePosition(LsBCCNode node, LsVector3 position);
  void DeleteNodeCutPoints(LsBCCNode node);
  LsOptional<LsVector3> GetEdgeCutPoint(LsBCCEdge edge) const;
  void SetEdgeCutPoint(LsBCCEdge edge, LsVector3 position);
  LsBCCColor GetEdgeColor(LsBCCEdge edge) const;
protected:
  std::tuple<int, int, int> minima;
  std::tuple<int, int, int> maxima;
  std::vector<NodeMetaData> nodeMetaData;
  LsOptional<int> GetEdgeIndexInNexus(LsBCCLattice::LsBCCEdge edge) const;
  LsBCCNode GetEdgeNexusNode(LsBCCEdge edge) const;
  NodeMetaData& GetNodeMetaDataReference(LsBCCNode node);
  NodeMetaData const& GetNodeMetaDataConstReference(LsBCCNode node) const;
  EdgeMetaData& GetEdgeMetaDataReference(LsBCCEdge edge);
  EdgeMetaData const& GetEdgeMetaDataConstReference(LsBCCEdge edge) const;
  int GetNodeIndex(LsBCCNode node) const;
  bool Valid(LsBCCNode node) const;
  bool WithinBounds(LsBCCNode node) const;
  bool NodeExists(LsBCCNode node) const;
};