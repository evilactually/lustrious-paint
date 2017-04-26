
//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsOptional.h>
#include <FactoradicPermutation.hh>
#include <LsPatternMatchers.h>

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsPatternMatcher::AddNode(int id, LsBCCValue value) {
  assert(currentNodeIndex < 4);
  patternNodes[currentNodeIndex++] = {id, value};
}

void LsPatternMatcher::AddEdge(int id1, int id2, LsBCCMaybeColor maybeColor) {
  assert(currentEdgeIndex < 6);
  patternEdges[currentEdgeIndex++] = {id1, id2, maybeColor};
}

LsBCCNode LsPatternMatcher::GetNodeById(int id) {
  for (int i = 0; i < 4; ++i)
  {
    if (std::get<0>(patternNodes[i]) == id) {
      return matchedNodes[i];
    }
  }
}

glm::vec3 LsPatternMatcher::GetNodePosition(int id)
{
  return pLattice->GetNodePosition(GetNodeById(id));
}

LsOptional<glm::vec3> LsPatternMatcher::GetEdgeCutPoint(int id1, int id2) {
  return pLattice->GetEdgeCutPoint(LsBCCEdge(GetNodeById(id1), GetNodeById(id2)));
}

LsBCCColor LsPatternMatcher::GetNodeColor(int id)
{
  return pLattice->GetNodeColor(GetNodeById(id));
}

LsBCCColor LsPatternMatcher::GetEdgeColor(int id1, int id2) {
  return pLattice->GetEdgeColor(LsBCCEdge(GetNodeById(id1), GetNodeById(id2)));
}

bool LsPatternMatcher::Match(LsBCCLattice const& lattice, LsBCCTetrahedron tetrahedron) {
  pLattice = &lattice;
  
  // Fill permutation array with initial values
  matchedNodes[0] = std::get<0>(tetrahedron);
  matchedNodes[1] = std::get<1>(tetrahedron);
  matchedNodes[2] = std::get<2>(tetrahedron);
  matchedNodes[3] = std::get<3>(tetrahedron);

  for (int i = 0; i < 24; ++i) // This loop will try to permute matchedNodes until the pattern fits it.
  {
    NthPermutation(&matchedNodes[0], &matchedNodes[4], i); // Integer tuples implement lexicographical comparison.

    bool failed = false;
    
    // Test that nodes have correct values
    for (int j = 0; j < 4; ++j)
    {
      if (lattice.GetNodeValue(matchedNodes[j]) != std::get<1>(patternNodes[j])) {
        failed = true;
        break;
      }
    }

    // Try next permutation
    if (failed) continue;

    // Test edge colors
    for (int j = 0; j < 6; ++j)
    {
      LsBCCMaybeColor patternEdgeColor = std::get<2>(patternEdges[j]);

      if (!patternEdgeColor) continue; // Pattern accepts any color for this edge, no test.

      int id1 = std::get<0>(patternEdges[j]);
      int id2 = std::get<1>(patternEdges[j]);
      if ( GetEdgeColor(id1, id2) != patternEdgeColor ) {
        failed = true;
        break;
      }
    }

    if(!failed) return true; // Found.
  }
  return false; // Not found.
}

LsPPPPMatcher::LsPPPPMatcher() {
  AddNode(1, LsBCCValue::ePositive);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsZPPPMatcher::LsZPPPMatcher() {
  AddNode(1, LsBCCValue::ePositive);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::eZero);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None()); 
}

LsZZPPMatcher::LsZZPPMatcher() {
  AddNode(1, LsBCCValue::eZero);
  AddNode(2, LsBCCValue::eZero);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsZZZPMatcher::LsZZZPMatcher() {
  AddNode(1, LsBCCValue::eZero);
  AddNode(2, LsBCCValue::eZero);
  AddNode(3, LsBCCValue::eZero);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsNZZPMatcher::LsNZZPMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eZero);
  AddNode(3, LsBCCValue::eZero);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsNNZPMatcher::LsNNZPMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eNegative);
  AddNode(3, LsBCCValue::eZero);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsNNNPMatcher::LsNNNPMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eNegative);
  AddNode(3, LsBCCValue::eNegative);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsNZPPMatcher::LsNZPPMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::eZero);
  AddEdge(1, 2, LsBCCColor::eBlack); // Cut red
  AddEdge(3, 4, LsBCCColor::eBlack); 
  AddEdge(3, 2, LsBCCColor::eRed);
  AddEdge(3, 1, LsBCCColor::eRed); // Cut black
  AddEdge(4, 1, LsBCCColor::eRed);
  AddEdge(4, 2, LsBCCColor::eRed);
}

LsNNPPMatcher::LsNNPPMatcher()
{
  AddNode(1, LsBCCValue::ePositive);
  AddNode(2, LsBCCValue::eNegative);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::eNegative);
  AddEdge(1, 2, LsBCCColor::eBlack);
  AddEdge(3, 4, LsBCCColor::eBlack);
  AddEdge(1, 4, LsBCCColor::eRed);
  AddEdge(1, 3, LsBCCColor::eRed);
  AddEdge(2, 4, LsBCCColor::eRed);
  AddEdge(2, 3, LsBCCColor::eRed);
}

LsNZPPParityMatcher::LsNZPPParityMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eZero);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCColor::eBlack); // Note: This looks like previous NZPP pattern, but nodes have different signs!
  AddEdge(3, 4, LsBCCColor::eBlack);
  AddEdge(3, 2, LsBCCColor::eRed);
  AddEdge(3, 1, LsBCCColor::eRed); // Cut red
  AddEdge(4, 1, LsBCCColor::eRed); // Cut red
  AddEdge(4, 2, LsBCCColor::eRed);
}

LsNPPPParityMatcher::LsNPPPParityMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCColor::eBlack); // Cut black
  AddEdge(3, 4, LsBCCColor::eBlack);
  AddEdge(3, 2, LsBCCColor::eRed);
  AddEdge(3, 1, LsBCCColor::eRed); // Cut red
  AddEdge(4, 1, LsBCCColor::eRed); // Cut red
  AddEdge(4, 2, LsBCCColor::eRed);
}

LsNNPPParityMatcher::LsNNPPParityMatcher()
{
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eNegative);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCColor::eBlack);
  AddEdge(3, 4, LsBCCColor::eBlack);
  AddEdge(1, 4, LsBCCColor::eRed); // Cut red
  AddEdge(1, 3, LsBCCColor::eRed); // Cut red
  AddEdge(2, 4, LsBCCColor::eRed); // Cut red
  AddEdge(2, 3, LsBCCColor::eRed); // Cut red
}

