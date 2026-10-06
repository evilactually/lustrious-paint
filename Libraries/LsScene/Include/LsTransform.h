#pragma once

#include <glm/glm.hpp>

//-------------------------------------------------------------------------------
// @ LsTransform
//-------------------------------------------------------------------------------
// Affine transform that keeps its inverse alongside, in the manner of pbrt's
// Transform. Points, vectors and normals transform differently, and SDF
// evaluation needs the world-to-object direction, so the inverse is computed
// once rather than on every query.
//-------------------------------------------------------------------------------
class LsTransform
{
  glm::mat4 m;
  glm::mat4 mInv;
public:
  LsTransform();
  explicit LsTransform(const glm::mat4& m);
  LsTransform(const glm::mat4& m, const glm::mat4& mInv);

  static LsTransform Translate(glm::vec3 delta);
  static LsTransform Scale(glm::vec3 scale);
  static LsTransform Rotate(float radians, glm::vec3 axis);

  const glm::mat4& GetMatrix() const { return m; }
  const glm::mat4& GetInverseMatrix() const { return mInv; }
  LsTransform Inverse() const;

  // Applies t first, then this transform
  LsTransform operator*(const LsTransform& t) const;

  glm::vec3 ApplyToPoint(glm::vec3 p) const;
  glm::vec3 ApplyToVector(glm::vec3 v) const;
  glm::vec3 ApplyToNormal(glm::vec3 n) const;

  // Smallest singular value of the linear part: the most an object-space
  // distance can shrink when mapped to world space. Multiplying an object-space
  // SDF value by it gives a distance bound that is safe to sphere-trace with,
  // even under non-uniform scale.
  float GetMinScale() const;
};

//-------------------------------------------------------------------------------
// @ LsPlacement
//-------------------------------------------------------------------------------
// Editable translation / rotation / scale of a scene node relative to its parent.
// Rotation is in Euler degrees, applied about X first, then Y, then Z.
//-------------------------------------------------------------------------------
struct LsPlacement
{
  glm::vec3 translation = glm::vec3(0.0f);
  glm::vec3 rotation = glm::vec3(0.0f);
  glm::vec3 scale = glm::vec3(1.0f);

  LsTransform ToTransform() const;

  // Inverse of ToTransform. Exact unless the matrix has shear (which a chain of
  // rotated, non-uniformly scaled parents can produce), then the closest
  // rotation and per-axis scale are returned.
  static LsPlacement FromMatrix(const glm::mat4& m);
};
