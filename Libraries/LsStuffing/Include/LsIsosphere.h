#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LsIsosurfaceStuffer.h"
#include "LsBCCLatticeTypes.h"

class LsIsosphere: public LsIsosurface
{
private:
  float radius;
  glm::vec3 position;
public:
  LsIsosphere(glm::vec3 position, float radius);
  ~LsIsosphere();
  LsDomain GetDomain() const override;
  LsBCCValue EvaluateAt(glm::vec3 vertex) const override;
  glm::vec3 IntersectByEdge(glm::vec3 e1, glm::vec3 e2) const override;
};
