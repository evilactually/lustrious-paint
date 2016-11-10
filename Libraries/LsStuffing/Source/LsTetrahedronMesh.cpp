//===============================================================================
// @ LsTetrahedronMesh.cpp
// 
// A general class for constructing tetrahedron mesh
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsTetrahedronMesh.h>
#include <LsStdAlgorithms.h>

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

bool doesReferenceNode(LsTetrahedronMesh::LsTetrahedron tetrahedron, int node) {
  return tetrahedron[0] == node ||
         tetrahedron[1] == node ||
         tetrahedron[2] == node ||
         tetrahedron[3] == node;
}

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

LsTetrahedronMesh::LsTetrahedronMesh() {

}

LsTetrahedronMesh::~LsTetrahedronMesh() { }

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::AddTetrahedron()
//-------------------------------------------------------------------------------
// Add tetrahedron by specifying indecies of it's nodes, duplicate tetrahedra are allowed.
//-------------------------------------------------------------------------------
int LsTetrahedronMesh::AddTetrahedron(int node1, int node2, int node3, int node4) {
  indexBuffer.push_back(LsTetrahedron(node1, node2, node3, node4));
  return indexBuffer.size() - 1;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::AddTetrahedron()
//-------------------------------------------------------------------------------
// Add tetrahedron by specifying coordinates of it's vertecies, duplicate tetrahedra are allowed.
//-------------------------------------------------------------------------------
int LsTetrahedronMesh::AddTetrahedron(const LsVector3& node1, 
                                      const LsVector3& node2, 
                                      const LsVector3& node3, 
                                      const LsVector3& node4) {
  indexBuffer.push_back(LsTetrahedron(AddNode(node1), AddNode(node2), AddNode(node3), AddNode(node4)));
  return indexBuffer.size() - 1;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::RemoveTetrahedron()
//-------------------------------------------------------------------------------
// Remove tetrahedron by index, may leave some vertecies unused and require a
// call to LsTetrahedronMesh::Optimize() to prune them.
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::RemoveTetrahedron(int tetrahedronIndex) {
  // This might leave some nodes unused, but it's legal to have unused vertecies
  indexBuffer.erase(indexBuffer.begin() + tetrahedronIndex);
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::FindNodeIndex(const LsVector3&)
//-------------------------------------------------------------------------------
// Try to get index of a node approximately equal to given coordinates
//-------------------------------------------------------------------------------
LsOptional<int> LsTetrahedronMesh::FindNodeIndex(const LsVector3& node) {
  return find_index( vertexBuffer.begin(), vertexBuffer.end(), node );
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::AddNode(const LsVector3&)
//-------------------------------------------------------------------------------
// Add a new vertex to vertex buffer and return it's index, duplicates are not added.
//-------------------------------------------------------------------------------
int LsTetrahedronMesh::AddNode(const LsVector3& node) {
  LsOptional<int> index = FindNodeIndex(node);
  if (!index)
  {
    vertexBuffer.push_back(node);
    index = vertexBuffer.size() - 1;
  }
  return index;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::RemoveNode(int)
//-------------------------------------------------------------------------------
// Remove node and possibly any tetrahedra referencing it
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::RemoveNode(int node) {
  // Find all tetrahedra referencing the node and remove them (erase-remove idiom)
  indexBuffer.erase(std::remove_if(indexBuffer.begin(),
                                   indexBuffer.end(),
                                   [&](LsTetrahedron tetrahedron) { return doesReferenceNode(tetrahedron, node); }),
                    indexBuffer.end());
  RemoveUnusedNode(node);
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::GetNodePosition(int)
//-------------------------------------------------------------------------------
// Look-up node position
//-------------------------------------------------------------------------------
LsVector3 LsTetrahedronMesh::GetNodePosition(int node) {
  return vertexBuffer[node];
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::RemoveUnusedNode(int)
//-------------------------------------------------------------------------------
// Remove vertex from vertex buffer and update index buffer. 
// Requirement: node must not be referenced by any tetrahedra in the index buffer.
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::RemoveUnusedNode(int node) {
  // Remove node(shifts all nodes higher than the removed node to the left)
  vertexBuffer.erase(vertexBuffer.begin() + node);
  // If referenced node is higher than the node that was removed subtract 1 from it
  for(LsTetrahedron& tetrahedron:indexBuffer) {
    if (tetrahedron[0] > node) tetrahedron[0]--;
    if (tetrahedron[1] > node) tetrahedron[1]--;
    if (tetrahedron[2] > node) tetrahedron[2]--;
    if (tetrahedron[3] > node) tetrahedron[3]--;
  }
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::Optimize()
//-------------------------------------------------------------------------------
// Prune vertex buffer from unused vertecies
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::Optimize() {
  // Count how many times each node is referenced
  std::vector<int> nodeUseCount(vertexBuffer.size(), 0);
  for(LsTetrahedron& tetrahedron:indexBuffer) {
    nodeUseCount[tetrahedron[0]]++;
    nodeUseCount[tetrahedron[1]]++;
    nodeUseCount[tetrahedron[2]]++;
    nodeUseCount[tetrahedron[3]]++;
  }
  // Remove all unused nodes
  int i = 0;
  while(i < vertexBuffer.size()) {
    if (!nodeUseCount[i])
    {
      RemoveUnusedNode(i);
      nodeUseCount.erase(nodeUseCount.begin() + i);
      // Do not increment i!
    } else {
      i++;
    }
  }
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::GetVertecies()
//-------------------------------------------------------------------------------
// Get constant reference to vertex buffer of tetrahedron mesh.
// Some vertecies may be unneeded.
//-------------------------------------------------------------------------------
const std::vector<LsVector3>& LsTetrahedronMesh::GetVertecies() {
  return vertexBuffer;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::GetIndecies()
//-------------------------------------------------------------------------------
// Get constant reference to index buffer of tetrahedron mesh.
// Every tetrahedron is valid at all times.
//-------------------------------------------------------------------------------
const std::vector<LsTetrahedronMesh::LsTetrahedron>& LsTetrahedronMesh::GetIndecies() {
  return indexBuffer;
}
