//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#include <LsGeometry.h>
#include <glm/glm.hpp>
#include <LsMath.h>
#include <vector>

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

//-------------------------------------------------------------------------------
// @ PolygonInOutTest()
//-------------------------------------------------------------------------------
// Returns 1 if point is inside the polygon, -1 if otside and 0 if point is on one
// of the edges
//-------------------------------------------------------------------------------
int PolygonInOutTest(std::vector<glm::vec2> polygon, glm::vec2 p) {
  for(auto it = polygon.begin(); it != (polygon.end() - 1); it++) {
    glm::vec2 a = *it;
    glm::vec2 b = *(it + 1);
    float d = DistanceToLine(a, b, p);
    if ( d > -kEpsilon ) {
        return -1; // if at least one half-test fails, point must be outside
    } else if ( !(d > kEpsilon ) ) {
        return 0;  // if point is not inside and not outside of the half-space, it must be on the edge
    }
  }
  return 1; // if it got thus far, the point was not outside or on the edge of any of the half-spaces
}

double TetrahedronVolume(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4) {
  glm::vec3 a = p2 - p1;
  glm::vec3 b = p3 - p1;
  glm::vec3 c = p4 - p1;
  double volume = (1.0/6.0)*glm::abs(glm::dot(a, glm::cross(c,b)));
  return volume;
}
