#pragma once

#include <glm/glm.hpp>

//-------------------------------------------------------------------------------
// @ LineSegmentHalfTest()
//-------------------------------------------------------------------------------
// Returns distance to line from a point. Function follows a right-hand rule: 
// if base of the index finger is considered as point a and tip as point b, then
// thumb points in direction of the line normal. Distance returned will be positive
// if the point is on the side of the normal, and negative otherwise.
//-------------------------------------------------------------------------------
float DistanceToLine(glm::vec2 a, glm::vec2 b, glm::vec2 p) {
  glm::vec2 w = b - a;
  glm::vec2 n = glm::normalize(glm::vec2(w.y, -w.x));
  glm::vec2 v = p - a;
  float distance = glm::dot(n,v); 
  return distance;
}