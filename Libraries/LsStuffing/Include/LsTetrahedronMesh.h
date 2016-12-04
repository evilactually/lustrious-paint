//===============================================================================
// @ LsTetrahedronMesh.h
// 
// A general class for constructing tetrahedron mesh
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "LsVector3.h"
#include "LsOptional.h"
#include "LsTuple.h"
#include <vector>

//-------------------------------------------------------------------------------
//-- Classes --------------------------------------------------------------------
//------------------------------------------------------------------------------- 

class LsTetrahedronMesh
{
public:
  typedef LsTuple<int, 4> LsTetrahedron;
  LsTetrahedronMesh();
  ~LsTetrahedronMesh();
  void UseAbsoluteEq(bool); // TODO: We may be able to get away with bitwise float comparison in most practical cases
  int AddTetrahedron(int node1, int node2, int node3, int node4);
  int AddTetrahedron(const LsVector3& node1, const LsVector3& node2, const LsVector3& node3, const LsVector3& node4);
  void RemoveTetrahedron(int tetrahedronIndex);
  LsOptional<int> FindNodeIndex(const LsVector3& node);
  int AddNode(const LsVector3& node);
  void RemoveNode(int node);
  LsVector3 GetNodePosition(int node);
  void Optimize();
  const std::vector<LsVector3>& GetVertecies();
  const std::vector<LsTetrahedron>& GetIndecies();
private:
  void RemoveUnusedNode(int node);
  std::vector<LsVector3> vertexBuffer;
  std::vector<LsTetrahedron> indexBuffer;
};