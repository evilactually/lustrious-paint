#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#include <glm/glm.hpp>
#include <vector>

//-------------------------------------------------------------------------------
// @ LineSegmentHalfTest()
//-------------------------------------------------------------------------------
// Returns distance to line from a point. Function follows a right-hand rule: 
// if base of the index finger is considered as point a and tip as point b, then
// thumb points in direction of the line normal. Distance returned will be positive
// if the point is on the side of the normal, and negative otherwise.
//-------------------------------------------------------------------------------
float DistanceToLine(glm::vec2 a, glm::vec2 b, glm::vec2 p);

//-------------------------------------------------------------------------------
// @ PolygonInOutTest()
//-------------------------------------------------------------------------------
// Returns 1 if point is inside the polygon, -1 if otside and 0 if point is on one
// of the edges
//-------------------------------------------------------------------------------
int PolygonInOutTest(std::vector<glm::vec2> polygon, glm::vec2 p);

//-------------------------------------------------------------------------------
// @ TetrahedtaVolume(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4)
//-------------------------------------------------------------------------------
// 
// Returns volume of a tetrahedron defined by four vertecies 
//-------------------------------------------------------------------------------
double TetrahedronVolume(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4);
