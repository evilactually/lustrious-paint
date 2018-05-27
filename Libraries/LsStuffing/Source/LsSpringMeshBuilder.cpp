#include "LsSpringMeshBuilder.h"
#include "LsLatticeVertexRef.h"


// typedef struct {
//   double edgeAdjacentTetrahedronVolume;

// } LsTmpSpringInfo;

// typedef std::unordered_map<LsLatticeVertexRef, LsTmpSpringInfo, std::function<decltype(hash_lattice_vertex_ref)>> LsTmpSpringMeshNodeInfo;

void LsSpringMeshBuilder::OnTetrahedronEmitted(const LsLatticeVertexRef& ref1, const LsLatticeVertexRef& ref2, const LsLatticeVertexRef& ref3, const LsLatticeVertexRef& ref4) {
  temporarySpringMesh[ref1][ref2].edgeAdjacentTetrahedronVolume += 0.1;
  //temporarySpringMesh[ref2][ref1].edgeAdjacentTetrahedronVolume += volume;

}

// Flattening hash maps:
// iterate over first level, build an array of nodes, for each iterate over sub-hash, 

// pass1: iterate over first level, fill first level of vector AND create node -> offset map
// pass2: iterate first level again, look-up in offset map, open in vector, iterate second level, look-up offset and write


