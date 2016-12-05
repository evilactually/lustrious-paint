//===============================================================================
// @ LsTetrahedronMesh.h
// 
// A general class for constructing tetrahedron mesh
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <glm/glm.hpp>
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
  int AddTetrahedron(const glm::vec3& node1, const glm::vec3& node2, const glm::vec3& node3, const glm::vec3& node4);
  void RemoveTetrahedron(int tetrahedronIndex);
  LsOptional<int> FindNodeIndex(const glm::vec3& node);
  int AddNode(const glm::vec3& node);
  void RemoveNode(int node);
  glm::vec3 GetNodePosition(int node);
  void Optimize();
  const std::vector<glm::vec3>& GetVertecies();
  const std::vector<LsTetrahedron>& GetIndecies();
private:
  void RemoveUnusedNode(int node);
  std::vector<glm::vec3> vertexBuffer;
  std::vector<LsTetrahedron> indexBuffer;
  bool useAbsoluteEq = false;
};