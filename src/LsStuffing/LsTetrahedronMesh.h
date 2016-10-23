//===============================================================================
// @ LsTetrahedronMesh.h
// 
// A class for constructing tetrahedron mesh
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "LsVector3.h"
#include "LsOptional.h"
#include <vector>

//-------------------------------------------------------------------------------
//-- Classes --------------------------------------------------------------------
//-------------------------------------------------------------------------------

class LsTetrahedronMesh
{
public:
  LsTetrahedronMesh();
  ~LsTetrahedronMesh();
  void AddTetrahedron(int node1, int node2, int node3, int node4);
  void AddTetrahedron(const LsVector3& node1, const LsVector3& node2, const LsVector3& node3, const LsVector3& node4);
  void RemoveTetrahedron(int node1, int node2, int node3, int node4);
  LsOptional<int> FindNodeIndex(const LsVector3& node);
  int AddNode(const LsVector3& node);
  void RemoveNode(int node);
  void GetNodePosition(int node);
  void Optimize();
  const std::vector<int>& GetIndecies();
  const std::vector<LsVector3>& GetVertecies();
private:
  std::vector<LsVector3> vertexBuffer;
  std::vector<int> indexBuffer;
};