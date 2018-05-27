#pragma once

#include "LsITetrahedronProcessor.h"
#include "LsLatticeVertexRef.h"
#include <unordered_map>
#include <functional>

class LsSpringMeshBuilder: LsITetrahedronProcessor {
private:
	typedef struct {
 		double edgeAdjacentTetrahedronVolume;
	} LsTmpSpringInfo;
	typedef std::unordered_map<LsLatticeVertexRef, LsTmpSpringInfo, std::function<decltype(hash_lattice_vertex_ref)>> LsTmpSpringMeshNodeInfo;
	std::unordered_map<LsLatticeVertexRef, LsTmpSpringMeshNodeInfo, std::function<decltype(hash_lattice_vertex_ref)>> temporarySpringMesh;
public:
  void LsSpringMeshBuilder::OnTetrahedronEmitted(const LsLatticeVertexRef& ref1, const LsLatticeVertexRef& ref2, const LsLatticeVertexRef& ref3, const LsLatticeVertexRef& ref4);
};

