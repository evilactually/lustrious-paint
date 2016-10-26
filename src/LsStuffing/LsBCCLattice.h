//===============================================================================
// @ LsBccLattice.h
// 
// Body Centered Cubic Lattice data structure
//
//===============================================================================

#pragma once

#include <vector>
#include "LsTuple.h"
#include "LsOptional.h"
#include "LsVector3.h"

//-------------------------------------------------------------------------------
//-- Typedefs -------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsBCCNode
//-------------------------------------------------------------------------------
// Specifies coordinates of a node
//-------------------------------------------------------------------------------
typedef LsTuple<int, 3> LsBCCNode;

//-------------------------------------------------------------------------------
// @ LsBCCEdge
//-------------------------------------------------------------------------------
// Specifies coordinates of nodes defining an edge
//-------------------------------------------------------------------------------
typedef LsTuple<LsBCCNode, 2> LsBCCEdge;

//-------------------------------------------------------------------------------
// @ LsBCCTetrahedron
//-------------------------------------------------------------------------------
// Specifies coordinates of nodes defining a tetrahedron
//-------------------------------------------------------------------------------
typedef LsTuple<LsBCCNode, 4> LsBCCTetrahedron;


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
private:
  struct LsBCCEdgeMetaData {
    LsOptional<LsVector3> cutPoint;
  };
  struct LsBCCNodeMetaData {
    LsBCCNode coordinates;
    LsVector3 position;
    LsBCCValue value;
    LsBCCEdgeMetaData edgeNexus[7];
  };
public:
  class NodeIterator {
  friend LsBCCLattice;
  public:
    LsOptional<LsBCCNode> Next();
  private:
    NodeIterator(LsBCCLattice const& lattice);
    LsBCCLattice const& lattice;
    size_t currentIndex = 0;
  };

  class TetrahedronIterator {
  friend LsBCCLattice;
  public:
    LsOptional<LsBCCTetrahedron> Next();
  private:
    TetrahedronIterator(LsBCCLattice const& lattice);
    LsBCCLattice const& lattice;
  };

  class NodeEdgeIterator {
  friend LsBCCLattice;
  public:
    LsOptional<LsBCCEdge> Next();
  private:
    NodeEdgeIterator(LsBCCLattice const& lattice, LsBCCNode node);
    LsBCCLattice const& lattice;
  };

  class EdgeIterator {
  friend LsBCCLattice;
  public:
    LsOptional<LsBCCEdge> Next();
  private:
    EdgeIterator(LsBCCLattice const& lattice);
    LsBCCLattice const& lattice;
    size_t currentNodeIndex = 0;
    int currentNexusIndex = 0;
  };
  
  LsBCCLattice(LsTuple<int,3> minima, LsTuple<int,3> maxima, float step);
  TetrahedronIterator GetTetrahedronIterator(); //   TODO: ???
  NodeIterator GetNodeIterator();
  NodeEdgeIterator GetNodeEdgeIterator(LsBCCNode node);       //   TODO:
  EdgeIterator GetEdgeIterator();               //   TODO: iterate over vertecies, iterate over nexus edges, use bounds to filter non-existent 
  LsVector3 GetNodePosition(LsBCCNode node) const;
  LsBCCColor GetNodeColor(LsBCCNode node) const;
  LsBCCValue GetNodeValue(LsBCCNode node) const;
  void SetNodeValue(LsBCCNode node, LsBCCValue value);
  void SetNodePosition(LsBCCNode node, LsVector3 position); 
  void DeleteNodeCutPoints(LsBCCNode node);
  LsOptional<LsVector3> GetEdgeCutPoint(LsBCCEdge edge) const;
  LsBCCColor GetEdgeColor(LsBCCEdge edge) const;
  void SetEdgeCutPoint(LsBCCEdge edge, LsVector3 position);
private:
  LsTuple<int,3> minima;
  LsTuple<int,3> maxima;
  std::vector<LsBCCNodeMetaData> nodeMetaData;
  LsOptional<int> GetEdgeIndexInNexus(LsBCCLattice::LsBCCEdge edge) const;
  LsBCCNode GetEdgeNexusNode(LsBCCEdge edge) const;
  LsBCCNodeMetaData& GetNodeMetaDataReference(LsBCCNode node);
  LsBCCNodeMetaData const& GetNodeMetaDataConstReference(LsBCCNode node) const;
  LsBCCEdgeMetaData& GetEdgeMetaDataReference(LsBCCEdge edge);
  LsBCCEdgeMetaData const& GetEdgeMetaDataConstReference(LsBCCEdge edge) const;
  int GetNodeIndex(LsBCCNode node) const;
  bool NodeExists(LsBCCNode node) const;
};