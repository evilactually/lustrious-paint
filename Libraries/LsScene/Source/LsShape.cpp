#include <LsShape.h>

#include <algorithm>

// Distance functions follow Inigo Quilez's formulations and are mirrored in
// Shaders/scene.slang, keep both in sync

const char* LsShapeTypeName(LsShapeType type) {
  switch (type) {
    case LsShapeType::Sphere:   return "Sphere";
    case LsShapeType::Box:      return "Box";
    case LsShapeType::Capsule:  return "Capsule";
    case LsShapeType::Cylinder: return "Cylinder";
    case LsShapeType::Torus:    return "Torus";
    case LsShapeType::Plane:    return "Plane";
    default:                    return "Unknown";
  }
}

LsShape LsShape::Sphere(float radius) {
  return { LsShapeType::Sphere, glm::vec4(radius, 0.0f, 0.0f, 0.0f) };
}

LsShape LsShape::Box(glm::vec3 halfExtents, float rounding) {
  return { LsShapeType::Box, glm::vec4(halfExtents, rounding) };
}

LsShape LsShape::Capsule(float halfHeight, float radius) {
  return { LsShapeType::Capsule, glm::vec4(halfHeight, radius, 0.0f, 0.0f) };
}

LsShape LsShape::Cylinder(float halfHeight, float radius) {
  return { LsShapeType::Cylinder, glm::vec4(halfHeight, radius, 0.0f, 0.0f) };
}

LsShape LsShape::Torus(float majorRadius, float minorRadius) {
  return { LsShapeType::Torus, glm::vec4(majorRadius, minorRadius, 0.0f, 0.0f) };
}

LsShape LsShape::Plane() {
  return { LsShapeType::Plane, glm::vec4(0.0f) };
}

LsShape LsShape::Default(LsShapeType type) {
  switch (type) {
    case LsShapeType::Box:      return Box(glm::vec3(0.5f));
    case LsShapeType::Capsule:  return Capsule(0.5f, 0.25f);
    case LsShapeType::Cylinder: return Cylinder(0.5f, 0.5f);
    case LsShapeType::Torus:    return Torus(0.5f, 0.15f);
    case LsShapeType::Plane:    return Plane();
    default:                    return Sphere(0.5f);
  }
}

float LsShape::Distance(glm::vec3 p) const {
  switch (type) {
    case LsShapeType::Sphere:
      return glm::length(p) - params.x;
    case LsShapeType::Box: {
      // Rounding shrinks the core box so the outer extents stay as specified
      glm::vec3 q = glm::abs(p) - (glm::vec3(params) - params.w);
      return glm::length(glm::max(q, 0.0f)) + std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f) - params.w;
    }
    case LsShapeType::Capsule:
      p.y -= glm::clamp(p.y, -params.x, params.x);
      return glm::length(p) - params.y;
    case LsShapeType::Cylinder: {
      glm::vec2 d = glm::abs(glm::vec2(glm::length(glm::vec2(p.x, p.z)), p.y)) - glm::vec2(params.y, params.x);
      return std::min(std::max(d.x, d.y), 0.0f) + glm::length(glm::max(d, 0.0f));
    }
    case LsShapeType::Torus: {
      glm::vec2 q(glm::length(glm::vec2(p.x, p.z)) - params.x, p.y);
      return glm::length(q) - params.y;
    }
    case LsShapeType::Plane:
      return p.y;
    default:
      return 0.0f;
  }
}

bool LsShape::GetBounds(glm::vec3& min, glm::vec3& max) const {
  glm::vec3 extent;
  switch (type) {
    case LsShapeType::Sphere:   extent = glm::vec3(params.x); break;
    case LsShapeType::Box:      extent = glm::vec3(params); break;
    case LsShapeType::Capsule:  extent = glm::vec3(params.y, params.x + params.y, params.y); break;
    case LsShapeType::Cylinder: extent = glm::vec3(params.y, params.x, params.y); break;
    case LsShapeType::Torus:    extent = glm::vec3(params.x + params.y, params.y, params.x + params.y); break;
    default:                    return false;
  }
  min = -extent;
  max = extent;
  return true;
}
