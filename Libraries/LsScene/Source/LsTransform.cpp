#include <LsTransform.h>

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

LsTransform::LsTransform() : m(1.0f), mInv(1.0f) { }

LsTransform::LsTransform(const glm::mat4& m) : m(m), mInv(glm::inverse(m)) { }

LsTransform::LsTransform(const glm::mat4& m, const glm::mat4& mInv) : m(m), mInv(mInv) { }

LsTransform LsTransform::Translate(glm::vec3 delta) {
  return LsTransform(glm::translate(glm::mat4(1.0f), delta),
                     glm::translate(glm::mat4(1.0f), -delta));
}

LsTransform LsTransform::Scale(glm::vec3 scale) {
  return LsTransform(glm::scale(glm::mat4(1.0f), scale),
                     glm::scale(glm::mat4(1.0f), 1.0f/scale));
}

LsTransform LsTransform::Rotate(float radians, glm::vec3 axis) {
  glm::mat4 r = glm::rotate(glm::mat4(1.0f), radians, axis);
  // Rotations are orthonormal, the inverse is the transpose
  return LsTransform(r, glm::transpose(r));
}

LsTransform LsTransform::Inverse() const {
  return LsTransform(mInv, m);
}

LsTransform LsTransform::operator*(const LsTransform& t) const {
  return LsTransform(m*t.m, t.mInv*mInv);
}

glm::vec3 LsTransform::ApplyToPoint(glm::vec3 p) const {
  return glm::vec3(m*glm::vec4(p, 1.0f));
}

glm::vec3 LsTransform::ApplyToVector(glm::vec3 v) const {
  return glm::vec3(m*glm::vec4(v, 0.0f));
}

glm::vec3 LsTransform::ApplyToNormal(glm::vec3 n) const {
  // Normals transform by the inverse transpose
  return glm::vec3(glm::transpose(mInv)*glm::vec4(n, 0.0f));
}

float LsTransform::GetMinScale() const {
  // Singular values of A are square roots of the eigenvalues of the symmetric
  // matrix A^T*A, which have a closed form for 3x3 (Smith, 1961)
  glm::dmat3 a = glm::dmat3(glm::mat3(m));
  glm::dmat3 b = glm::transpose(a)*a;

  double p1 = b[0][1]*b[0][1] + b[0][2]*b[0][2] + b[1][2]*b[1][2];
  double minEigenvalue;
  if (p1 == 0.0) {
    minEigenvalue = std::min({ b[0][0], b[1][1], b[2][2] });
  }
  else {
    double q = (b[0][0] + b[1][1] + b[2][2])/3.0;
    double p2 = (b[0][0] - q)*(b[0][0] - q) + (b[1][1] - q)*(b[1][1] - q) + (b[2][2] - q)*(b[2][2] - q) + 2.0*p1;
    double p = std::sqrt(p2/6.0);
    double r = glm::determinant((b - glm::dmat3(q))*(1.0/p))/2.0;
    double phi = std::acos(std::max(-1.0, std::min(1.0, r)))/3.0;
    const double twoThirdsPi = 2.0943951023931954923;
    minEigenvalue = q + 2.0*p*std::cos(phi + twoThirdsPi);
  }
  return static_cast<float>(std::sqrt(std::max(0.0, minEigenvalue)));
}

LsTransform LsPlacement::ToTransform() const {
  glm::vec3 radians = glm::radians(rotation);
  return LsTransform::Translate(translation)*
         LsTransform::Rotate(radians.z, glm::vec3(0.0f, 0.0f, 1.0f))*
         LsTransform::Rotate(radians.y, glm::vec3(0.0f, 1.0f, 0.0f))*
         LsTransform::Rotate(radians.x, glm::vec3(1.0f, 0.0f, 0.0f))*
         LsTransform::Scale(scale);
}

LsPlacement LsPlacement::FromMatrix(const glm::mat4& m) {
  LsPlacement placement;
  placement.translation = glm::vec3(m[3]);

  glm::mat3 r(m);
  for (int i = 0; i < 3; i++) {
    placement.scale[i] = glm::length(r[i]);
    if (placement.scale[i] > 0.0f) {
      r[i] /= placement.scale[i];
    }
  }
  // A mirroring transform is represented as a negative scale on X
  if (glm::determinant(r) < 0.0f) {
    placement.scale.x = -placement.scale.x;
    r[0] = -r[0];
  }

  // r = Rz*Ry*Rx, glm indexes as r[column][row]
  float x, y, z;
  y = std::asin(std::max(-1.0f, std::min(1.0f, -r[0][2])));
  if (std::abs(r[0][2]) < 0.9999f) {
    x = std::atan2(r[1][2], r[2][2]);
    z = std::atan2(r[0][1], r[0][0]);
  }
  else {
    // Gimbal lock, X and Z rotate about the same axis, so put it all on X
    x = std::atan2(-r[2][1], r[1][1]);
    z = 0.0f;
  }
  placement.rotation = glm::degrees(glm::vec3(x, y, z));
  // Rounding leaves tiny or negative zero angles that show up as -0.0 when edited
  for (int i = 0; i < 3; i++) {
    if (std::abs(placement.rotation[i]) < 1.0e-4f) {
      placement.rotation[i] = 0.0f;
    }
  }
  return placement;
}

// The same angle, plus whatever whole turns bring it closest to the hint
static float NearestTurn(float degrees, float hint) {
  return degrees + 360.0f*std::round((hint - degrees)/360.0f);
}

static bool NearlyEqual(const glm::mat4& a, const glm::mat4& b) {
  float scale = 1.0f;
  for (int i = 0; i < 4; i++) {
    scale = std::max(scale, glm::length(glm::vec3(b[i])));
  }
  for (int i = 0; i < 4; i++) {
    if (glm::length(a[i] - b[i]) > 1.0e-5f*scale) {
      return false;
    }
  }
  return true;
}

LsPlacement LsPlacement::FromMatrix(const glm::mat4& m, const LsPlacement& hint) {
  LsPlacement placement = FromMatrix(m);

  // Only moved, keep rotation and scale exactly as they were
  LsPlacement moved = hint;
  moved.translation = placement.translation;
  if (NearlyEqual(moved.ToTransform().GetMatrix(), m)) {
    return moved;
  }

  // Rz*Ry*Rx has a second solution (x + 180, 180 - y, z + 180), and each angle can
  // gain whole turns, pick the combination closest to the hint
  glm::vec3 a = placement.rotation;
  glm::vec3 b(a.x + 180.0f, 180.0f - a.y, a.z + 180.0f);
  for (int i = 0; i < 3; i++) {
    a[i] = NearestTurn(a[i], hint.rotation[i]);
    b[i] = NearestTurn(b[i], hint.rotation[i]);
  }
  placement.rotation = glm::length(a - hint.rotation) <= glm::length(b - hint.rotation) ? a : b;

  // Rounding noise in what didn't change
  for (int i = 0; i < 3; i++) {
    if (std::abs(placement.rotation[i] - hint.rotation[i]) < 1.0e-3f) {
      placement.rotation[i] = hint.rotation[i];
    }
    if (std::abs(placement.scale[i] - hint.scale[i]) < 1.0e-6f) {
      placement.scale[i] = hint.scale[i];
    }
  }
  return placement;
}
