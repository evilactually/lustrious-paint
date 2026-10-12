// Scene primitives as produced by LsScene::Flatten (Libraries/LsScene).
// Import with `import scene;` and bind the primitives as a StructuredBuffer.
// Shape types, roles and the struct layout must match LsShape.h and LsScene.h.

static const uint SHAPE_SPHERE   = 0;
static const uint SHAPE_BOX      = 1;
static const uint SHAPE_CAPSULE  = 2;
static const uint SHAPE_CYLINDER = 3;
static const uint SHAPE_TORUS    = 4;
static const uint SHAPE_PLANE    = 5;

static const uint ROLE_CONSTRUCTION       = 1u << 0;
static const uint ROLE_SHADOW_CASTER      = 1u << 1;
static const uint ROLE_LIGHT_EMITTER      = 1u << 2;
static const uint ROLE_PARTICIPATING_MEDIA = 1u << 3;
// The canvas tool, blended into the construction geometry, see LsScene.h
static const uint ROLE_TOOL               = 1u << 4;
static const uint ROLE_TOOL_ONLY          = 1u << 5;
static const uint ROLE_TOOL_CARVE         = 1u << 6;

struct ScenePrimitive
{
  // Rows of the 3x4 affine world-to-object matrix
  float4 worldToObject[3];
  float4 shapeParams;
  // rgb = emitted radiance, a = radius of a sphere around primitiveCenter
  // enclosing the shape in world space
  float4 emission;
  // rgb = albedo, a = density
  float4 medium;
  uint shapeType;
  uint roles;
  // Object space distances times this are safe world space distances
  float distanceScale;
  uint nodeId;
};

// A shape built from its packed ScenePrimitive::shapeParams
interface IShape
{
  __init(float4 params);
  // Signed distance in object space, see LsShape::Distance
  float distance(float3 p);
};

struct Sphere : IShape
{
  float radius;

  __init(float4 params) { radius = params.x; }

  float distance(float3 p)
  {
    return length(p) - radius;
  }
};

struct Box : IShape
{
  float3 halfExtents;
  float rounding;

  __init(float4 params) { halfExtents = params.xyz; rounding = params.w; }

  float distance(float3 p)
  {
    float3 q = abs(p) - (halfExtents - rounding);
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - rounding;
  }
};

struct Capsule : IShape
{
  float halfHeight;
  float radius;

  __init(float4 params) { halfHeight = params.x; radius = params.y; }

  float distance(float3 p)
  {
    p.y -= clamp(p.y, -halfHeight, halfHeight);
    return length(p) - radius;
  }
};

struct Cylinder : IShape
{
  float halfHeight;
  float radius;

  __init(float4 params) { halfHeight = params.x; radius = params.y; }

  float distance(float3 p)
  {
    float2 d = abs(float2(length(p.xz), p.y)) - float2(radius, halfHeight);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
  }
};

struct Torus : IShape
{
  float majorRadius;
  float minorRadius;

  __init(float4 params) { majorRadius = params.x; minorRadius = params.y; }

  float distance(float3 p)
  {
    float2 q = float2(length(p.xz) - majorRadius, p.y);
    return length(q) - minorRadius;
  }
};

// Two-sided square, see LsShape::Distance
struct Plane : IShape
{
  float halfSize;

  __init(float4 params) { halfSize = params.x; }

  float distance(float3 p)
  {
    float2 outside = max(abs(p.xz) - halfSize, 0.0);
    return length(float3(outside.x, p.y, outside.y));
  }
};

float shapeDistance<S : IShape>(float4 params, float3 p)
{
  return S(params).distance(p);
}

// Dispatches on the runtime shape type
float shapeDistance(uint shapeType, float4 params, float3 p)
{
  switch (shapeType) {
    case SHAPE_SPHERE:   return shapeDistance<Sphere>(params, p);
    case SHAPE_BOX:      return shapeDistance<Box>(params, p);
    case SHAPE_CAPSULE:  return shapeDistance<Capsule>(params, p);
    case SHAPE_CYLINDER: return shapeDistance<Cylinder>(params, p);
    case SHAPE_TORUS:    return shapeDistance<Torus>(params, p);
    case SHAPE_PLANE:    return shapeDistance<Plane>(params, p);
    default:             return 0.0;
  }
}

float3 worldToObjectPoint(ScenePrimitive primitive, float3 p)
{
  float4 h = float4(p, 1.0);
  return float3(dot(primitive.worldToObject[0], h),
                dot(primitive.worldToObject[1], h),
                dot(primitive.worldToObject[2], h));
}

// World position of the primitive's object space origin
float3 primitiveCenter(ScenePrimitive primitive)
{
  float3 r0 = primitive.worldToObject[0].xyz;
  float3 r1 = primitive.worldToObject[1].xyz;
  float3 r2 = primitive.worldToObject[2].xyz;
  float3 b = float3(primitive.worldToObject[0].w, primitive.worldToObject[1].w, primitive.worldToObject[2].w);
  // Columns of the inverse are cross products of the rows over the determinant
  float3 c0 = cross(r1, r2);
  float3 c1 = cross(r2, r0);
  float3 c2 = cross(r0, r1);
  float determinant = dot(r0, c0);
  return -(c0*b.x + c1*b.y + c2*b.z)/determinant;
}

// Conservative world space distance to a single primitive
float primitiveDistance(ScenePrimitive primitive, float3 p)
{
  float3 objectP = worldToObjectPoint(primitive, p);
  return shapeDistance(primitive.shapeType, primitive.shapeParams, objectP)*primitive.distanceScale;
}

// Union of all primitives having any of the roles in roleMask, returns the
// index of the closest one in closest (or ~0 if none matched)
float sceneDistance(StructuredBuffer<ScenePrimitive> primitives, uint count, float3 p, uint roleMask, out uint closest)
{
  float distance = 1.0e30;
  closest = ~0u;
  for (uint i = 0; i < count; i++) {
    ScenePrimitive primitive = primitives[i];
    if ((primitive.roles & roleMask) == 0) {
      continue;
    }
    float d = primitiveDistance(primitive, p);
    if (d < distance) {
      distance = d;
      closest = i;
    }
  }
  return distance;
}
