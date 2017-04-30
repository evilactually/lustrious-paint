#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <LsIsosurfaceStuffer.h>
#include <LsBCCLatticeTypes.h>
#include <LsIsosphere.h>
#include <LsMath.h>

LsIsosphere::LsIsosphere() :position({ 0.0f, 0.0f, 0.0f }), radius(0.0f) { };

LsIsosphere::LsIsosphere(glm::vec3 position, float radius):position(position), radius(radius) { };

LsIsosphere::~LsIsosphere() {};

LsDomain LsIsosphere::GetDomain() const {
  LsDomain domain;
  domain.x1 = position.x - radius;
  domain.y1 = position.y - radius;
  domain.z1 = position.z - radius;
  domain.x2 = position.x + radius;
  domain.y2 = position.y + radius;
  domain.z2 = position.z + radius;
  return domain;
}
#include <iostream>
/*
  Evaluate inequality:
  (x-x0)^2 + (y-y0)^2 + (z-z0)^2 < r^2
*/
LsBCCValue LsIsosphere::EvaluateAt(glm::vec3 vertex) const {
  float cutFunction = pow(radius, 2) - (pow(vertex.x - position.x, 2) + pow(vertex.y - position.y, 2) + pow(vertex.z - position.z, 2));
  if ( IsZero(cutFunction) )
  {
    return LsBCCValue::eZero;
  } else if ( cutFunction < 0.0f ) {
    return LsBCCValue::eNegative;
  } else {
    return LsBCCValue::ePositive;
  }
}

/*
  Solve this for t:
  x1 + t*(x2 - x1) = x
  y1 + t*(y2 - y1) = y
  z1 + t*(z2 - z1) = z
  x^2 + y^2 + z^2 = r^2 OR
  (x-x0)^2 + (y-y0)^2 + (z-z0)^2 = r^2
*/
glm::vec3 LsIsosphere::IntersectByEdge(glm::vec3 p1, glm::vec3 p2) const {
  float a = pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2) + pow(p2.z - p1.z, 2);
  float b = 2*((p1.x - position.x)*(p2.x - p1.x) + (p1.y - position.y)*(p2.y - p1.y) + (p1.z - position.z)*(p2.z - p1.z));
  float c = pow(p1.x - position.x, 2) + pow(p1.y - position.y, 2) + pow(p1.z - position.z, 2) - pow(radius, 2);
  float t1 = (-b + sqrt(pow(b,2) - 4*a*c))/(2*a);
  float t2 = (-b - sqrt(pow(b,2) - 4*a*c))/(2*a);
  //std::cout << "Params:" << std::endl;
  //std::cout << t1 << std::endl;
  //std::cout << t2 << std::endl;
  //assert((t1 > 0.0f) || (t2 > 0.0f));
  float t;
  if ( t1 > 0.0f && t1 < 1.0f ) { // Choose positive root that is in 0 to 1 range
    t = t1;
  } else {
    t = t2;
  }
  return p1 + t*(p2 - p1);
}
