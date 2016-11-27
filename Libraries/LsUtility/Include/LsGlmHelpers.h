#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::tmat2x2<float> tmat2x2_rotation(float angle) {
  return { cos(angle), sin(angle), 
          -sin(angle), cos(angle) };
}