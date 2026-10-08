// Ray queries against the scene primitives for the editor: picking, placing the
// 3D cursor and finding what a camera looks at. Each thread traces one ray the
// way LsRaycastPrimitives does on the CPU, and the editor reads the hits back a
// frame later. Structs must match LsSceneView.cpp.

import scene;

struct PickPushConstants
{
  uint primitiveCount;
  uint rayCount;
  uint pad0;
  uint pad1;
};

struct PickRay
{
  float4 origin;    // w = max distance
  float4 direction; // unit length, w unused
  uint roleMask;
  uint pad0;
  uint pad1;
  uint pad2;
};

struct PickHit
{
  float4 position; // w = distance along the ray
  float4 normal;
  uint nodeId;
  uint hit;        // 0 when the ray hit nothing
  uint pad0;
  uint pad1;
};

[[vk::push_constant]]
ConstantBuffer<PickPushConstants> pick;

[[vk::binding(0)]]
StructuredBuffer<ScenePrimitive> primitives;

[[vk::binding(1)]]
StructuredBuffer<PickRay> rays;

[[vk::binding(2)]]
RWStructuredBuffer<PickHit> hits;

static const int MAX_STEPS = 512;

// Primitives containing the ray origin are skipped, so a ray from inside a room
// still hits its walls
bool traced(ScenePrimitive primitive, PickRay ray)
{
  return (primitive.roles & ray.roleMask) != 0 && primitiveDistance(primitive, ray.origin.xyz) > 0.0;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void pickMain(uint3 globalId : SV_DispatchThreadID)
{
  if (globalId.x >= pick.rayCount) {
    return;
  }
  PickRay ray = rays[globalId.x];
  PickHit result = (PickHit)0;

  float t = 0.0;
  for (int step = 0; step < MAX_STEPS && t < ray.origin.w; step++) {
    float3 p = ray.origin.xyz + ray.direction.xyz*t;
    float distance = 1.0e30;
    uint closest = ~0u;
    for (uint i = 0; i < pick.primitiveCount; i++) {
      ScenePrimitive primitive = primitives[i];
      if (!traced(primitive, ray)) {
        continue;
      }
      float d = primitiveDistance(primitive, p);
      if (d < distance) {
        distance = d;
        closest = i;
      }
    }
    if (closest == ~0u) {
      break;
    }

    // Tolerance grows with distance, far away a pixel covers more
    float epsilon = 1.0e-4*max(1.0, t);
    if (distance < epsilon) {
      ScenePrimitive primitive = primitives[closest];
      // Central differences of the hit primitive alone, the union would blur normals where shapes touch
      float h = epsilon;
      float3 gradient = float3(
        primitiveDistance(primitive, p + float3(h, 0.0, 0.0)) - primitiveDistance(primitive, p - float3(h, 0.0, 0.0)),
        primitiveDistance(primitive, p + float3(0.0, h, 0.0)) - primitiveDistance(primitive, p - float3(0.0, h, 0.0)),
        primitiveDistance(primitive, p + float3(0.0, 0.0, h)) - primitiveDistance(primitive, p - float3(0.0, 0.0, h)));
      float gradientLength = length(gradient);
      float3 normal = gradientLength > 0.0 ? gradient/gradientLength : -ray.direction.xyz;
      // The distance to a plane folds at its surface, so differences across it cancel.
      // Its normal is the transformed +Y instead, facing the side the ray came from.
      if (primitive.shapeType == SHAPE_PLANE) {
        normal = normalize(primitive.worldToObject[1].xyz);
        normal = dot(normal, ray.direction.xyz) > 0.0 ? -normal : normal;
      }
      result.position = float4(p, t);
      result.normal = float4(normal, 0.0);
      result.nodeId = primitive.nodeId;
      result.hit = 1;
      break;
    }
    t += distance;
  }
  hits[globalId.x] = result;
}
