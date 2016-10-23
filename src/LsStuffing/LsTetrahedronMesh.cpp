//===============================================================================
// @ LsTetrahedronMesh.cpp
// 
// A general class for constructing tetrahedron mesh
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "LsTetrahedronMesh.h"
#include "LsStdAlgorithms.h"

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

LsTetrahedronMesh::LsTetrahedronMesh() {

}

LsTetrahedronMesh::~LsTetrahedronMesh() { }

void LsTetrahedronMesh::AddTetrahedron(int node1, int node2, int node3, int node4) {
  // same, but by index
}

void LsTetrahedronMesh::AddTetrahedron(const LsVector3& node1, const LsVector3& node2, const LsVector3& node3, const LsVector3& node4) {
  // add each vertex, write indecies into index buffers, unless they are already there
}

void LsTetrahedronMesh::RemoveTetrahedron(int node1, int node2, int node3, int node4) {
  // remove and compact index buffer
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
// 
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

bool doesReferenceNode(LsTuple<int, 4> tetrahedron, int node) {
  return tetrahedron[0] == node ||
         tetrahedron[1] == node ||
         tetrahedron[2] == node ||
         tetrahedron[3] == node;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::RemoveNode(int)
//-------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::RemoveNode(int node) {
  // Invalidate node in vertex buffer
  vertexBuffer[node] = LsOptional<LsVector3>::None();
  // Find all tetrahedra referencing the node and invalidate them
  for(LsOptional<LsTuple<int, 4>>& maybeTetrahedra:indexBuffer) {
    if (maybeTetrahedra && doesReferenceNode(maybeTetrahedra, node))
    {
      maybeTetrahedra = LsOptional<LsTuple<int, 4>>::None();
    }
  }
}

void LsTetrahedronMesh::GetNodePosition(int node) {
  // look-up position
}

void LsTetrahedronMesh::Optimize() {
  // remove unused nodes
  // rebuild index buffer
}

const std::vector<LsOptional<LsVector3>>& GetVertecies() {
  throw 0;
}

const std::vector<LsOptional<LsTuple<int, 4>>>& GetIndecies() {
  throw 0;  
}
