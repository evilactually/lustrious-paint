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
  LsOptional<glm::vec3> GetEdgeCutPoint(int id1, int id2);
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
  void AddEdge(int id1, int id2, LsBCCMaybeColor maybeColor); // Note: if you want to match a cut point, you just specify +/- edge
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

class LsZZPPMatcher: public LsPatternMatcher {
public:
  LsZZPPMatcher();
  ~LsZZPPMatcher() {};
};

class LsZZZPMatcher: public LsPatternMatcher {
public:
  LsZZZPMatcher();
  ~LsZZZPMatcher() {};
};

class LsNZZPMatcher: public LsPatternMatcher {
public:
  LsNZZPMatcher();
  ~LsNZZPMatcher() {};
};

class LsNNZPMatcher: public LsPatternMatcher {
public:
  LsNNZPMatcher();
  ~LsNNZPMatcher() {};
};

class LsNNNPMatcher: public LsPatternMatcher {
public:
  LsNNNPMatcher();
  ~LsNNNPMatcher() {};
};

class LsNZPPMatcher: public LsPatternMatcher {
public:
  LsNZPPMatcher();
  ~LsNZPPMatcher() {};
};

class LsNNPPMatcher : public LsPatternMatcher {
public:
  LsNNPPMatcher();
  ~LsNNPPMatcher() {};
};

class LsNZPPParityMatcher : public LsPatternMatcher {
public:
  LsNZPPParityMatcher();
  ~LsNZPPParityMatcher() {};
};

class LsNPPPParityMatcher : public LsPatternMatcher {
public:
  LsNPPPParityMatcher();
  ~LsNPPPParityMatcher() {};
};

class LsNNPPParityMatcher : public LsPatternMatcher {
public:
  LsNNPPParityMatcher();
  ~LsNNPPParityMatcher() {};
};