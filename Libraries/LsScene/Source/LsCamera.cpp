#include <LsCamera.h>

#include <cmath>

// Falls back to another axis when v is too short to normalise
static glm::vec3 SafeNormalize(glm::vec3 v, glm::vec3 fallback) {
  float length = glm::length(v);
  return length > 1.0e-8f ? v/length : fallback;
}

LsCamera::LsCamera(const glm::mat4& cameraToWorld, float verticalFovDegrees, float aspect) :
  aspect(aspect) {
  position = glm::vec3(cameraToWorld[3]);
  // Gram-Schmidt keeps the view direction and makes up perpendicular to it
  forward = SafeNormalize(-glm::vec3(cameraToWorld[2]), glm::vec3(0.0f, 0.0f, -1.0f));
  glm::vec3 upHint = glm::vec3(cameraToWorld[1]);
  up = SafeNormalize(upHint - forward*glm::dot(upHint, forward), glm::vec3(0.0f, 1.0f, 0.0f));
  right = glm::cross(forward, up);
  tanHalfFovY = std::tan(glm::radians(verticalFovDegrees)*0.5f);
}

LsCamera LsCamera::LookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up, float verticalFovDegrees, float aspect) {
  glm::vec3 forward = SafeNormalize(target - eye, glm::vec3(0.0f, 0.0f, -1.0f));
  // Looking straight along up, any perpendicular will do
  glm::vec3 right = glm::cross(forward, up);
  if (glm::length(right) < 1.0e-6f) {
    right = glm::cross(forward, std::abs(forward.z) < 0.9f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f));
  }
  right = glm::normalize(right);
  glm::mat4 cameraToWorld(1.0f);
  cameraToWorld[0] = glm::vec4(right, 0.0f);
  cameraToWorld[1] = glm::vec4(glm::cross(right, forward), 0.0f);
  cameraToWorld[2] = glm::vec4(-forward, 0.0f);
  cameraToWorld[3] = glm::vec4(eye, 1.0f);
  return LsCamera(cameraToWorld, verticalFovDegrees, aspect);
}

float LsCamera::GetVerticalFov() const {
  return glm::degrees(2.0f*std::atan(tanHalfFovY));
}

glm::mat4 LsCamera::GetCameraToWorld() const {
  glm::mat4 m(1.0f);
  m[0] = glm::vec4(right, 0.0f);
  m[1] = glm::vec4(up, 0.0f);
  m[2] = glm::vec4(-forward, 0.0f);
  m[3] = glm::vec4(position, 1.0f);
  return m;
}

LsRay LsCamera::GenerateRay(glm::vec2 ndc) const {
  LsRay ray;
  ray.origin = position;
  ray.direction = glm::normalize(forward + right*(ndc.x*tanHalfFovY*aspect) + up*(ndc.y*tanHalfFovY));
  return ray;
}

float LsCamera::GetDepth(glm::vec3 world) const {
  return glm::dot(world - position, forward);
}

bool LsCamera::Project(glm::vec3 world, glm::vec2& ndc) const {
  glm::vec3 d = world - position;
  float depth = glm::dot(d, forward);
  if (depth <= 1.0e-6f) {
    return false;
  }
  ndc.x = glm::dot(d, right)/(depth*tanHalfFovY*aspect);
  ndc.y = glm::dot(d, up)/(depth*tanHalfFovY);
  return true;
}

glm::vec3 LsCamera::GetImagePlanePoint(glm::vec2 ndc, float depth) const {
  return position + (forward + right*(ndc.x*tanHalfFovY*aspect) + up*(ndc.y*tanHalfFovY))*depth;
}
