//===============================================================================
// @ LsSpringMeshTypes.h
// 
// Type definitions for a spring mesh(host side)
//
//===============================================================================

typedef struct {
  size_t offset;    // Offset of referenced vertex in LsSpringMesh
  double length;    // Spring rest length
  double stiffness; // Spring stiffness
} LsSpring;

typedef struct {
  glm::vec3 position;
  std::vector<LsSpring> springs;
} LsSpringNode;

typedef std::vector<LsSpringNode> LsSpringMesh;
