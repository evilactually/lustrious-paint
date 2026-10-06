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

struct ScenePrimitive
{
  // Rows of the 3x4 affine world-to-object matrix
  float4 worldToObject[3];
  float4 shapeParams;
  // rgb = emitted radiance
  float4 emission;
  // rgb = albedo, a = density
  float4 medium;
  uint shapeType;
  uint roles;
  // Object space distances times this are safe world space distances
  float distanceScale;
  uint nodeId;
};

// Signed distance in object space, see LsShape::Distance
float shapeDistance(uint shapeType, float4 params, float3 p)
{
  switch (shapeType) {
    case SHAPE_SPHERE:
      return length(p) - params.x;
    case SHAPE_BOX: {
      float3 q = abs(p) - (params.xyz - params.w);
      return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - params.w;
    }
    case SHAPE_CAPSULE:
      p.y -= clamp(p.y, -params.x, params.x);
      return length(p) - params.y;
    case SHAPE_CYLINDER: {
      float2 d = abs(float2(length(p.xz), p.y)) - float2(params.y, params.x);
      return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
    }
    case SHAPE_TORUS: {
      float2 q = float2(length(p.xz) - params.x, p.y);
      return length(q) - params.y;
    }
    case SHAPE_PLANE: {
      // Two-sided square, see LsShape::Distance
      float2 outside = max(abs(p.xz) - params.x, 0.0);
      return length(float3(outside.x, p.y, outside.y));
    }
    default:
      return 0.0;
  }
}

float3 worldToObjectPoint(ScenePrimitive primitive, float3 p)
{
  float4 h = float4(p, 1.0);
  return float3(dot(primitive.worldToObject[0], h),
                dot(primitive.worldToObject[1], h),
                dot(primitive.worldToObject[2], h));
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
