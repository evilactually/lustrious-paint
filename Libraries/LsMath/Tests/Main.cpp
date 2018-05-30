#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "LsGeometry.h"
#include <glm/glm.hpp>
#include <iostream>

TEST_CASE( "ccc", "bbb" ) {
  double v = TetrahedronVolume(glm::vec3(0.0, 0.0, 0.0),
  	                           glm::vec3(0.0, 10.0, 0.0),
  	                           glm::vec3(10.0, 0.0, 0.0),
  	                           glm::vec3(10.0, 10.0, 10.0));
  REQUIRE((int)v == 166);
  v = TetrahedronVolume(glm::vec3(0.0, 0.0, 0.0),
  	                    glm::vec3(0.0, 10.0, 0.0),
  	                    glm::vec3(10.0, 0.0, 0.0),
  	                    glm::vec3(100.0, 100.0, 100.0));
  REQUIRE((int)v == 1666);
  v = TetrahedronVolume(glm::vec3(0.0, 0.0, 0.0),
  	                    glm::vec3(0.0, 10.0, 0.0),
  	                    glm::vec3(10.0, 0.0, 0.0),
  	                    glm::vec3(-100.0, -100.0, -100.0));
  REQUIRE((int)v == 1666);
}

