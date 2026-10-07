#pragma once

#include <glm/glm.hpp>

struct LsRay
{
  glm::vec3 origin = glm::vec3(0.0f);
  // Unit length
  glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);

  glm::vec3 At(float t) const { return origin + direction*t; }
};

//-------------------------------------------------------------------------------
// @ LsCamera
//-------------------------------------------------------------------------------
// Pinhole perspective camera. It looks down -Z of its own frame with +Y up,
// the OpenGL convention, so a camera node placed with zero rotation looks
// towards -Z in world space.
//
// Normalized device coordinates span [-1, 1] on both axes with +Y up, so x
// covers the horizontal field of view and y the vertical one. A lens shift
// moves the image off the view axis, like a shift lens or an off-centre crop
// of a larger image, without turning the camera.
//-------------------------------------------------------------------------------
class LsCamera
{
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
  glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
  float tanHalfFovY = 0.0f;
  float aspect = 1.0f;
  glm::vec2 lensShift = glm::vec2(0.0f);
public:
  LsCamera() = default;
  // Scale and shear in cameraToWorld are ignored, only position and orientation are kept
  LsCamera(const glm::mat4& cameraToWorld, float verticalFovDegrees, float aspect);
  // Up only has to be roughly perpendicular to the view direction
  static LsCamera LookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up, float verticalFovDegrees, float aspect);

  glm::vec3 GetPosition() const { return position; }
  glm::vec3 GetRight() const { return right; }
  glm::vec3 GetUp() const { return up; }
  glm::vec3 GetForward() const { return forward; }
  float GetAspect() const { return aspect; }
  float GetTanHalfFovY() const { return tanHalfFovY; }
  float GetVerticalFov() const; // in degrees
  // Offset of the image centre from the view axis, in the same tangent units as
  // the half field of view: a shift of 1 puts the image centre 45 degrees off axis
  glm::vec2 GetLensShift() const { return lensShift; }
  void SetLensShift(glm::vec2 shift) { lensShift = shift; }

  // Orthonormal frame without scale, columns are right, up, -forward, position
  glm::mat4 GetCameraToWorld() const;

  LsRay GenerateRay(glm::vec2 ndc) const;

  // Distance along the view direction, negative behind the camera
  float GetDepth(glm::vec3 world) const;
  // Projects to normalized device coordinates, false for points at or behind the camera
  bool Project(glm::vec3 world, glm::vec2& ndc) const;

  // Point of the image plane at the given depth, ndc picks where on it
  glm::vec3 GetImagePlanePoint(glm::vec2 ndc, float depth) const;
};
