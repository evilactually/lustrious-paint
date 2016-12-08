
#include "LsTetrahedronMesh.h"
#include <glm/glm.hpp>

TEST_CASE( "tetmesh01" ) {
  LsTetrahedronMesh mesh;
  REQUIRE ( mesh.GetIndecies().size() == 0 );
  mesh.AddNode(glm::vec3(0.0f, 0.0f, 0.0f));
  mesh.AddNode(glm::vec3(1.0f, 0.0f, 0.0f));
  mesh.AddNode(glm::vec3(2.0f, 0.0f, 0.0f));
  mesh.AddNode(glm::vec3(3.0f, 0.0f, 0.0f));
  REQUIRE ( mesh.GetVertecies().size() == 4 );
  mesh.AddTetrahedron(0,1,2,3);
  REQUIRE ( mesh.GetIndecies().size() == 1 );
  mesh.AddTetrahedron(glm::vec3(0.0f, 0.0f, 0.0f),
                      glm::vec3(1.0f, 0.0f, 0.0f),
                      glm::vec3(2.0f, 0.0f, 0.0f),
                      glm::vec3(3.0f, 0.0f, 0.0f));
  REQUIRE ( mesh.GetIndecies().size() == 2 );
  REQUIRE ( mesh.GetIndecies()[0][0] == 0 );
  REQUIRE ( mesh.GetIndecies()[0][1] == 1 );
  REQUIRE ( mesh.GetIndecies()[0][2] == 2 );
  REQUIRE ( mesh.GetIndecies()[0][3] == 3 );
  mesh.RemoveNode(0);
  REQUIRE ( mesh.GetIndecies().size() == 0 );
  REQUIRE ( mesh.GetVertecies().size() == 3 );

  mesh.AddNode(glm::vec3(0.0f, 0.0f, 0.0f));
  mesh.AddNode(glm::vec3(1.0f, 0.0f, 0.0f));
  mesh.AddNode(glm::vec3(2.0f, 0.0f, 0.0f));
  mesh.AddNode(glm::vec3(3.0f, 0.0f, 0.0f));
  mesh.AddNode(glm::vec3(0.0f, 1.0f, 0.0f));
  mesh.AddNode(glm::vec3(0.0f, 2.0f, 0.0f));
  mesh.AddNode(glm::vec3(0.0f, 3.0f, 0.0f));
  mesh.AddNode(glm::vec3(0.0f, 4.0f, 0.0f));
  REQUIRE ( mesh.GetVertecies().size() == 8 );

  mesh.AddTetrahedron(0, 1, 2, 3);
  mesh.AddTetrahedron(4, 5, 6, 7);
  REQUIRE ( mesh.GetIndecies().size() == 2 );

  mesh.Optimize();
  REQUIRE ( mesh.GetVertecies().size() == 8 );

  mesh.RemoveTetrahedron(1);
  REQUIRE ( mesh.GetIndecies().size() == 1 );

  mesh.Optimize();
  REQUIRE ( mesh.GetVertecies().size() == 4 );

  //REQUIRE ( 0 == 1 );
}
