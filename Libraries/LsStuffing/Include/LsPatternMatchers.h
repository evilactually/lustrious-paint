//===============================================================================
// @ LsPatternMatcher.h
// 
// A base class for all pattern matchers. A pattern matcher extends this class 
// and describes it's pattern during construction using protected methods
// AddNode and AddEdge. A pattern consists of node identifiers, their signs, 
// edges connecting them and optionally edge colors.
//
//===============================================================================

// NOTE: We don't need to mark cut edges in pattern. Plus-minus edges are always cut.

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsBCCLatticeTypes.h>
#include <LsBCCLattice.h>
#include <LsOptional.h>

using LsBCCMaybeColor = LsOptional<LsBCCColor>;

//-------------------------------------------------------------------------------
//-- Classes --------------------------------------------------------------------
//-------------------------------------------------------------------------------

class LsPatternMatcher
{
public:
  bool Match(LsBCCLattice const& lattice, LsBCCTetrahedron tetrahedron);
  LsBCCNode GetNodeById(int id);
  glm::vec3 GetNodePosition(int id);
  glm::vec3 GetEdgeCutPoint(int id1, int id2);
  LsBCCColor GetNodeColor(int id);
  LsBCCColor GetEdgeColor(int id1, int id2);
  LsBCCValue GetNodeValue(int id);
private:
  LsBCCNode matchedNodes[4];                             // Node stored at index 0 in matchedNodes
  std::tuple<int, LsBCCValue> patternNodes[4];           // corresponds to id stored at patternNodes
  int currentNodeIndex = 0;                              // under same index. By permuting elements of 
  std::tuple<int, int, LsBCCMaybeColor> patternEdges[6]; // matchedNodes array we can assign different
  int currentEdgeIndex = 0;                              // ids to nodes. This is what Match() method does.
  LsBCCLattice const* pLattice;
protected:
  LsPatternMatcher() {};
  void AddNode(int id, LsBCCValue value);
  void AddEdge(int id1, int id2, LsBCCMaybeColor maybeColor);
};

class LsPPPPMatcher: public LsPatternMatcher {
public:
  LsPPPPMatcher();
  ~LsPPPPMatcher() {};
};

class LsZPPPMatcher: public LsPatternMatcher {
public:
  LsZPPPMatcher();
  ~LsZPPPMatcher() {};
};

class LsG3NZPPMatcher: public LsPatternMatcher {
public:
  LsG3NZPPMatcher();
  ~LsG3NZPPMatcher() {};
};