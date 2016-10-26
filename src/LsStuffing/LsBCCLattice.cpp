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

LsBCCLattice::NodeIterator::NodeIterator(LsBCCLattice const& lattice):lattice(lattice) { };

LsOptional<LsBCCNode> LsBCCLattice::NodeIterator::Next() {
  if ( currentIndex >= lattice.nodeMetaData.size() )
  {
    return LsOptional<LsBCCNode>::None();
  }
  return lattice.nodeMetaData[currentIndex++].coordinates;
}

LsBCCLattice::TetrahedronIterator::TetrahedronIterator(LsBCCLattice const& lattice):lattice(lattice) { };

LsOptional<LsBCCTetrahedron> LsBCCLattice::TetrahedronIterator::Next() { 
  throw 1;
};

LsBCCLattice::NodeEdgeIterator::NodeEdgeIterator(LsBCCLattice const& lattice, LsBCCNode node):lattice(lattice) { };

LsOptional<LsBCCEdge> LsBCCLattice::NodeEdgeIterator::Next() { 
  throw 1;
}

LsBCCLattice::EdgeIterator::EdgeIterator(LsBCCLattice const& lattice):lattice(lattice) { };

LsOptional<LsBCCEdge> LsBCCLattice::EdgeIterator::Next() {
  throw 1;
}

LsBCCLattice::LsBCCLattice(LsTuple<int,3> minima,
                           LsTuple<int,3> maxima,
                           float step):minima(minima), maxima(maxima) {
  for (int z = minima[2]; z <= maxima[2]; ++z)
  {
    for (int y = minima[1]; y <= maxima[1]; ++y)
    {
      for (int x = minima[0]; x <= maxima[0]; ++x)
      {
        LsBCCNodeMetaData nodeInfo;
        nodeInfo.coordinates = {x,y,z};
        nodeInfo.position = step*LsVector3(x,y,z); // Vertex coordinates correspond to 
        nodeInfo.value = LsBCCValue::eUnassigned;  // BCC grid coordinates scaled by step
        
        // Node are either have all coordinates even(black) or odd(red).
        // All other coordinates don't represent any node.
        if ( LsEven(y) && LsEven(x) && LsEven(z) || LsOdd(y) && LsOdd(x) && LsOdd(z) )
        {
          nodeMetaData.push_back(nodeInfo);
        }
      }
    }
  }
}

LsBCCLattice::TetrahedronIterator LsBCCLattice::GetTetrahedronIterator() {   //   TODO: ???
  return TetrahedronIterator(*this);
}


// BCCLattice::NodeIterator BCCLattice::GetNodeIterator();
// BCCLattice::NodeEdgeIterator BCCLattice::GetNodeEdgeIterator();       //   TODO:
// BCCLattice::EdgeIterator BCCLattice::GetEdgeIterator();               //   TODO: iterate over vertecies, iterate over nexus edges, use bounds to filter non-existent 

// // Node info
// LsVector3 BCCLattice::GetNodePosition(LsNode node) const;
// LsNodeColor BCCLattice::GetNodeColor(LsNode node) const;
// LsNodeValue BCCLattice::GetNodeValue(LsNode node) const;
// void BCCLattice::SetNodeValue(LsNode node, Value value);
// void BCCLattice::SetNodePosition(LsNode node, LsVector3 position); 
// void BCCLattice::DeleteNodeCutPoints(LsNode node);
 
// // Edge info
// LsOptional<LsVector3> BCCLattice::GetEdgeCutPoint(LsEdge edge) const;
// LsColor BCCLattice::GetEdgeColor(LsEdge edge) const;
// void BCCLattice::SetEdgeCutPoint(LsEdge edge, LsVector3 position);

// Optional<int> GetEdgeIndexInNexus(BCCLattice::LsEdge edge) const;
// LsNode GetEdgeNexusNode(LsEdge edge) const;
// LsNodeMetaData& GetNodeMetaDataReference(Node node);
// LsNodeMetaData const& GetNodeMetaDataConstReference(Node node) const;
// LsEdgeMetaData& GetEdgeMetaDataReference(LsEdge edge);
// LsEdgeMetaData const& GetEdgeMetaDataConstReference(LsEdge edge) const;
// int GetNodeIndex(LsNode node) const;
// bool NodeExists(LsNode node) const;
