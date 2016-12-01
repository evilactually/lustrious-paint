
// compute sign +, -, 0, virtual
// compute intersection with zero surface given an edge, virtual
// compute signs, private
// compute cut points, private
// warp, private
// fill, private
// set alpha and beta, public
// void Stuff(LsTetrahedronMesh& mesh), public // stuff the surface and output resulting mesh into provided instance of LsTetrahedronMesh

#include <tuple>
#include <memory>
#include <glm/glm.hpp>
#include "LsBCCLattice.h"
#include "LsTetrahedronMesh.h"

class LsTetrahedronStuffer
{
public:
  LsTetrahedronStuffer();
  ~LsTetrahedronStuffer();
  void Stuff(LsTetrahedronMesh& mesh);
  void SetAlphaLong(float a);
  void SetAlphaShort(float a);
protected:
  virtual std::tuple<int,int,int> GetBounds() = 0;
  virtual LsBCCValue GetValueAtVertex(glm::vec3 vertex) = 0;
  virtual glm::vec3 GetEdgeCutPoint(glm::vec3 e1, glm::vec3 e2) = 0;
private:
  std::unique_ptr<LsBCCLattice> bccLattice;
  void UpdateValues();
  void UpdateCutPoints();
  void Warp();
  void Fill(LsTetrahedronMesh& mesh);
};