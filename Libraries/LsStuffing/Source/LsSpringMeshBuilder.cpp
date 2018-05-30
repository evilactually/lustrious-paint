#include "LsSpringMeshBuilder.h"
#include "LsLatticeVertexRef.h"
#include "LsGeometry.h"
#include <vector>

//#include <iostream>
// typedef struct {
//   double edgeAdjacentTetrahedronVolume;

// } LsTmpSpringInfo;

// typedef std::unordered_map<LsLatticeVertexRef, LsTmpSpringInfo, std::function<decltype(hash_lattice_vertex_ref)>> LsTmpSpringMeshNodeInfo;

void LsSpringMeshBuilder::OnTetrahedronEmitted(const LsLatticeVertexRef& ref1, const LsLatticeVertexRef& ref2, const LsLatticeVertexRef& ref3, const LsLatticeVertexRef& ref4) {
  glm::vec3 p1 = ref1.GetPosition();
  glm::vec3 p2 = ref2.GetPosition();
  glm::vec3 p3 = ref3.GetPosition();
  glm::vec3 p4 = ref4.GetPosition();
  // Add the six edges of tetrahedron and store volume of the tetrahedron.
  double volume = TetrahedronVolume(ref1.GetPosition(), ref2.GetPosition(), ref3.GetPosition(), ref4.GetPosition());
  // 1
  temporarySpringMesh[ref1][ref2].edgeAdjacentTetrahedronVolume += volume;
  temporarySpringMesh[ref2][ref1].edgeAdjacentTetrahedronVolume += volume;
  
  // 2
  temporarySpringMesh[ref1][ref3].edgeAdjacentTetrahedronVolume += volume;
  temporarySpringMesh[ref3][ref1].edgeAdjacentTetrahedronVolume += volume;

  // 3
  temporarySpringMesh[ref1][ref4].edgeAdjacentTetrahedronVolume += volume;
  temporarySpringMesh[ref4][ref1].edgeAdjacentTetrahedronVolume += volume;

  // 4
  temporarySpringMesh[ref4][ref3].edgeAdjacentTetrahedronVolume += volume;
  temporarySpringMesh[ref3][ref4].edgeAdjacentTetrahedronVolume += volume;

  // 5 
  temporarySpringMesh[ref3][ref2].edgeAdjacentTetrahedronVolume += volume;
  temporarySpringMesh[ref2][ref3].edgeAdjacentTetrahedronVolume += volume;

  // 6
  temporarySpringMesh[ref2][ref4].edgeAdjacentTetrahedronVolume += volume;
  temporarySpringMesh[ref4][ref2].edgeAdjacentTetrahedronVolume += volume;
}

typedef struct {
  size_t offset; // Offset of referenced vertex
  double length;
  double stiffness;
} LsSpring;

typedef struct {
  glm::vec3 position;
  std::vector<LsSpring> springs;
} LsSpringNode;

#define E 1.5 // Material stiffness coefficient

void LsSpringMeshBuilder::Build() {
  std::vector<LsSpringNode> nodes;
  std::unordered_map<LsLatticeVertexRef, size_t> offset_map; // Used to resolve references to nodes in second pass

  // Phase 1: Add node positions and fill in look-up map
  for (auto it = temporarySpringMesh.begin(); it != temporarySpringMesh.end(); ++it)
  {
    LsSpringNode node = {it->first.GetPosition(), std::vector<LsSpring>()}; // Position, and empty vector of springs
    nodes.push_back(node);
    offset_map[it->first] = nodes.size() - 1;
  }

  // Phase 2
  for (auto it = temporarySpringMesh.begin(); it != temporarySpringMesh.end(); ++it)
  {
    LsLatticeVertexRef current_node_ref = it->first;
    size_t offset = offset_map[current_node_ref];
    LsSpringNode& current_spring_node = nodes[offset];
    LsTmpSpringMeshNodeInfo const& src_tmp_spring_node_info = temporarySpringMesh[current_node_ref];
    // Iterate over springs
    for (auto it2 = src_tmp_spring_node_info.begin(); it2 != src_tmp_spring_node_info.end(); ++it2){
      size_t referenced_spring_offset = offset_map[it2->first]; // Look-up where in the vector the referenced node ended up
      LsSpring new_spring_info;
      new_spring_info.offset = referenced_spring_offset;
      new_spring_info.length = glm::length(nodes[referenced_spring_offset].position - current_spring_node.position);
      // Calculate stiffness
      new_spring_info.stiffness = E*it2->second.edgeAdjacentTetrahedronVolume/(new_spring_info.length*new_spring_info.length);
      current_spring_node.springs.push_back(new_spring_info);
    }
  }

}

// Flattening hash maps:
// iterate over first level, build an array of nodes, for each iterate over sub-hash, 

// pass1: iterate over first level, fill first level of vector AND create node -> offset map
// pass2: iterate first level again, look-up in offset map, open in vector, iterate second level, look-up offset and write


