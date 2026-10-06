#pragma once

#include <cstdint>
#include <glm/glm.hpp>

// Values are shared with Shaders/scene.slang, append new types at the end
enum class LsShapeType : uint32_t
{
  Sphere,   // params.x = radius
  Box,      // params.xyz = half extents, params.w = edge rounding radius
  Capsule,  // params.x = half height of the segment along Y, params.y = radius
  Cylinder, // params.x = half height along Y, params.y = radius
  Torus,    // params.x = major radius in the XZ plane, params.y = minor radius
  Plane,    // Y = 0 plane facing +Y, no params
  Count
};

const char* LsShapeTypeName(LsShapeType type);

//-------------------------------------------------------------------------------
// @ LsShape
//-------------------------------------------------------------------------------
// Signed distance primitive defined in its own object space, centred at the
// origin. Placement in the scene comes from the node transform, the same split
// between Shape and Transform that pbrt makes.
//-------------------------------------------------------------------------------
struct LsShape
{
  LsShapeType type = LsShapeType::Sphere;
  glm::vec4 params = glm::vec4(0.5f, 0.0f, 0.0f, 0.0f);

  static LsShape Sphere(float radius);
  static LsShape Box(glm::vec3 halfExtents, float rounding = 0.0f);
  static LsShape Capsule(float halfHeight, float radius);
  static LsShape Cylinder(float halfHeight, float radius);
  static LsShape Torus(float majorRadius, float minorRadius);
  static LsShape Plane();
  // Unit-sized shape of the given type
  static LsShape Default(LsShapeType type);

  // Signed distance in object space, negative inside
  float Distance(glm::vec3 p) const;

  // Object space bounding box, returns false for unbounded shapes
  bool GetBounds(glm::vec3& min, glm::vec3& max) const;
};
