//===============================================================================
// @ LsTetrahedronMesh.cpp
// 
// A class for constructing tetrahedron mesh
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

}

void LsTetrahedronMesh::AddTetrahedron(const LsVector3& node1, const LsVector3& node2, const LsVector3& node3, const LsVector3& node4) {

}

void LsTetrahedronMesh::RemoveTetrahedron(int node1, int node2, int node3, int node4) {

}

LsOptional<int> LsTetrahedronMesh::FindNodeIndex(const LsVector3& node) {
  throw 0;
}

int LsTetrahedronMesh::AddNode(const LsVector3& node) {
  throw 0;
}

void LsTetrahedronMesh::RemoveNode(int node) {

}

void LsTetrahedronMesh::GetNodePosition(int node) {

}

void LsTetrahedronMesh::Optimize() {

}

const std::vector<int>& LsTetrahedronMesh::GetIndecies() {
  throw 0;
}

const std::vector<LsVector3>& LsTetrahedronMesh::GetVertecies() {
  throw 0;
}
