// Editor 3D view of the scene: sphere traces the scene primitives into a storage
// image that the UI shows. This is a preview for placing things, not the canvas
// renderer, so lighting is either a fixed studio setup or a quick approximation
// of the light rig with the emitters treated as point lights.
// Push constants and flags must match LsSceneView.h.

import scene;

// Set by the editor in ScenePrimitive.roles, above the real roles
static const uint PRIMITIVE_SELECTED = 1u << 31;
static const uint PRIMITIVE_HOVERED  = 1u << 30;
static const uint PRIMITIVE_UNASSIGNED = 1u << 29;

static const uint VIEW_LIT  = 1u << 0;
static const uint VIEW_GRID = 1u << 1;

static const uint SURFACE_ROLES = ROLE_CONSTRUCTION | ROLE_SHADOW_CASTER | ROLE_LIGHT_EMITTER | PRIMITIVE_UNASSIGNED;

struct ViewportPushConstants
{
  float4 origin;  // xyz = camera position, w = exposure multiplier
  float4 right;   // scaled by tan(fov/2)*aspect
  float4 up;      // scaled by tan(fov/2)
  float4 forward; // unit length
  uint2 size;     // in pixels
  uint primitiveCount;
  uint flags;
  float pixelAngle; // tan(fov/2)*2/height, footprint of a pixel one unit away
  float pad0;
  float pad1;
  float pad2;
};

[[vk::push_constant]]
ConstantBuffer<ViewportPushConstants> view;

[[vk::binding(0)]]
[format("rgba8")]
RWTexture2D<float4> outputImage;

[[vk::binding(1)]]
StructuredBuffer<ScenePrimitive> primitives;

static const float MAX_DISTANCE = 500.0;
static const int MAX_STEPS = 256;

struct Hit
{
  float t;
  uint index; // ~0 when nothing was hit
};

// Union of primitives with any role in roleMask, skipping one primitive
float distanceTo(float3 p, uint roleMask, uint skip, out uint closest)
{
  float distance = 1.0e30;
  closest = ~0u;
  for (uint i = 0; i < view.primitiveCount; i++) {
    if (i == skip || (primitives[i].roles & roleMask) == 0) {
      continue;
    }
    float d = primitiveDistance(primitives[i], p);
    if (d < distance) {
      distance = d;
      closest = i;
    }
  }
  return distance;
}

// Closest approach of the ray to the selection, in pixels, so the outline keeps
// its width at any distance. The selection counts whatever its roles are, so a
// selected medium gets an outline from the surface pass too.
Hit trace(float3 origin, float3 direction, uint roleMask, out float selectionPixels)
{
  Hit hit;
  hit.t = MAX_DISTANCE;
  hit.index = ~0u;
  selectionPixels = 1.0e30;

  float t = 0.0;
  for (int step = 0; step < MAX_STEPS && t < MAX_DISTANCE; step++) {
    float3 p = origin + direction*t;
    float distance = 1.0e30;
    float selectedDistance = 1.0e30;
    uint closest = ~0u;
    for (uint i = 0; i < view.primitiveCount; i++) {
      uint roles = primitives[i].roles;
      bool traced = (roles & roleMask) != 0;
      bool selected = (roles & PRIMITIVE_SELECTED) != 0;
      if (!traced && !selected) {
        continue;
      }
      float d = primitiveDistance(primitives[i], p);
      if (traced && d < distance) {
        distance = d;
        closest = i;
      }
      if (selected) {
        selectedDistance = min(selectedDistance, d);
      }
    }
    float footprint = max(t, 1.0e-3)*view.pixelAngle;
    selectionPixels = min(selectionPixels, selectedDistance/footprint);
    if (closest != ~0u && distance < 0.25*footprint) {
      hit.t = t;
      hit.index = closest;
      break;
    }
    // Stepping no further than the selection is away keeps its closest approach from being skipped
    float step = distance;
    if (selectedDistance > 0.0) {
      step = min(step, max(selectedDistance, 0.5*footprint));
    }
    if (step >= 1.0e29) {
      break; // nothing left to trace
    }
    t += step;
  }
  return hit;
}

float3 primitiveNormal(ScenePrimitive primitive, float3 p, float h)
{
  // Tetrahedral differences need four evaluations instead of six
  const float2 k = float2(1.0, -1.0);
  return normalize(k.xyy*primitiveDistance(primitive, p + k.xyy*h) +
                   k.yyx*primitiveDistance(primitive, p + k.yyx*h) +
                   k.yxy*primitiveDistance(primitive, p + k.yxy*h) +
                   k.xxx*primitiveDistance(primitive, p + k.xxx*h));
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

// Shadow casters between p and the light, with a soft penumbra
float shadow(float3 p, float3 direction, float maxT, uint light)
{
  float result = 1.0;
  float t = 0.01;
  for (int step = 0; step < 96 && t < maxT; step++) {
    uint closest;
    float d = distanceTo(p + direction*t, ROLE_SHADOW_CASTER, light, closest);
    if (closest == ~0u) {
      break;
    }
    if (d < 1.0e-4) {
      return 0.0;
    }
    result = min(result, 16.0*d/t);
    t += clamp(d, 0.005, 0.5);
  }
  return saturate(result);
}

float3 surfaceColor(ScenePrimitive primitive)
{
  // Planes are usually the ground, keep them behind the objects standing on them
  float shade = primitive.shapeType == SHAPE_PLANE ? 0.55 : 1.0;
  if ((primitive.roles & ROLE_CONSTRUCTION) != 0) {
    return float3(0.50, 0.50, 0.48)*shade;
  }
  if ((primitive.roles & ROLE_SHADOW_CASTER) != 0) {
    return float3(0.30, 0.34, 0.42)*shade;
  }
  // Shapes that do nothing yet
  return float3(0.22, 0.22, 0.22)*shade;
}

float3 studioLighting(float3 albedo, float3 n, float3 direction)
{
  // Key light over the viewer's left shoulder, so shapes read the same from any angle
  float3 key = normalize(-view.forward.xyz + normalize(view.up.xyz)*0.6 - normalize(view.right.xyz)*0.4);
  float diffuse = saturate(dot(n, key));
  float sky = 0.5 + 0.5*n.y;
  float3 ambient = lerp(float3(0.06, 0.055, 0.05), float3(0.17, 0.18, 0.20), sky);
  float rim = pow(1.0 - saturate(dot(n, -direction)), 4.0)*0.06;
  return albedo*(ambient + 0.5*diffuse) + rim;
}

float3 rigLighting(float3 albedo, float3 p, float3 n)
{
  float3 radiance = albedo*0.02;
  for (uint i = 0; i < view.primitiveCount; i++) {
    ScenePrimitive light = primitives[i];
    if ((light.roles & ROLE_LIGHT_EMITTER) == 0) {
      continue;
    }
    float3 toLight = primitiveCenter(light) - p;
    float distance2 = max(dot(toLight, toLight), 1.0e-4);
    float distance = sqrt(distance2);
    float3 l = toLight/distance;
    float cosine = dot(n, l);
    if (cosine <= 0.0) {
      continue;
    }
    // A sphere of radius r and radiance L gives irradiance L*pi*r^2/d^2 far away
    float radius = light.shapeParams.x*light.distanceScale;
    float3 irradiance = light.emission.rgb*(3.14159265*radius*radius/distance2)*cosine;
    // The light's own surface is where the shadow ray ends
    float visibility = shadow(p + n*1.0e-3, l, max(distance - radius, 0.0), i);
    radiance += albedo/3.14159265*irradiance*visibility;
  }
  return radiance;
}

float3 background(float3 direction)
{
  float h = direction.y*0.5 + 0.5;
  return lerp(float3(0.025, 0.026, 0.03), float3(0.075, 0.08, 0.095), h);
}

// Coverage of grid lines spaced cellSize apart, footprint is the pixel size on the plane
float gridLines(float2 p, float cellSize, float footprint)
{
  float2 distance = abs(frac(p/cellSize + 0.5) - 0.5)*cellSize;
  float2 coverage = 1.0 - smoothstep(0.0, footprint, distance);
  return max(coverage.x, coverage.y);
}

float4 grid(float3 origin, float3 direction, float t)
{
  float3 p = origin + direction*t;
  // Lines are as wide as a pixel, which grows at grazing angles
  float footprint = t*view.pixelAngle/max(abs(direction.y), 0.05);
  // Fade out where lines would be thinner than they are apart
  float minor = gridLines(p.xz, 1.0, footprint)*saturate(1.0 - footprint/0.25);
  float major = gridLines(p.xz, 10.0, footprint*1.5)*saturate(1.0 - footprint/2.5);
  float fade = saturate(1.0 - t/150.0);

  float4 color = float4(0.5, 0.5, 0.5, max(minor*0.25, major*0.45)*fade);
  // World axes along the grid: X red, Z blue
  float axisWidth = footprint*1.5;
  float xAxis = 1.0 - smoothstep(0.0, axisWidth, abs(p.z));
  float zAxis = 1.0 - smoothstep(0.0, axisWidth, abs(p.x));
  if (xAxis > 0.0) {
    color = lerp(color, float4(0.85, 0.25, 0.25, 0.8*fade), xAxis);
  }
  if (zAxis > 0.0) {
    color = lerp(color, float4(0.25, 0.45, 0.9, 0.8*fade), zAxis);
  }
  return color;
}

float3 linearToSrgb(float3 c)
{
  c = saturate(c);
  return select(c <= 0.0031308, c*12.92, 1.055*pow(c, 1.0/2.4) - 0.055);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 globalId : SV_DispatchThreadID)
{
  if (globalId.x >= view.size.x || globalId.y >= view.size.y) {
    return;
  }

  // Pixel centres in NDC with +Y up, image rows go down
  float2 ndc = (float2(globalId.xy) + 0.5)/float2(view.size)*2.0 - 1.0;
  ndc.y = -ndc.y;
  float3 origin = view.origin.xyz;
  float3 direction = normalize(view.forward.xyz + view.right.xyz*ndc.x + view.up.xyz*ndc.y);
  bool lit = (view.flags & VIEW_LIT) != 0;

  float selectionPixels;
  Hit hit = trace(origin, direction, SURFACE_ROLES, selectionPixels);

  float3 color;
  if (hit.index != ~0u) {
    ScenePrimitive primitive = primitives[hit.index];
    float3 p = origin + direction*hit.t;
    float3 n = primitiveNormal(primitive, p, max(hit.t, 1.0)*1.0e-3);
    if ((primitive.roles & ROLE_LIGHT_EMITTER) != 0) {
      // Emitters show their colour, scaled into range so bright ones don't clip to white
      float3 emission = primitive.emission.rgb;
      float peak = max(max(emission.r, emission.g), max(emission.b, 1.0e-6));
      color = lit ? emission*view.origin.w : emission/peak*(0.75 + 0.25*saturate(dot(n, -direction)));
    }
    else {
      float3 albedo = surfaceColor(primitive);
      color = lit ? rigLighting(albedo, p, n)*view.origin.w : studioLighting(albedo, n, direction);
    }
    if ((primitive.roles & PRIMITIVE_SELECTED) != 0) {
      color = lerp(color, float3(1.0, 0.55, 0.15), 0.15);
    }
    else if ((primitive.roles & PRIMITIVE_HOVERED) != 0) {
      color = lerp(color, float3(1.0, 1.0, 1.0), 0.08);
    }
  }
  else {
    color = background(direction);
  }

  if ((view.flags & VIEW_GRID) != 0 && direction.y*origin.y < 0.0) {
    float t = -origin.y/direction.y;
    // A ground plane coincides with the grid, keep the grid on top of it
    if (t <= hit.t*1.001 + 1.0e-3) {
      float4 lines = grid(origin, direction, t);
      color = lerp(color, lines.rgb, lines.a);
    }
  }

  // Participating media composite over everything behind them
  float mediaPixels;
  Hit media = trace(origin, direction, ROLE_PARTICIPATING_MEDIA, mediaPixels);
  if (media.index != ~0u && media.t < hit.t) {
    ScenePrimitive medium = primitives[media.index];
    // Thickness of the medium in front of the surface, by fixed steps through it
    float stepSize = 0.05;
    float thickness = 0.0;
    float t = media.t;
    for (int step = 0; step < 64 && t < hit.t; step++) {
      if (primitiveDistance(medium, origin + direction*t) < 0.0) {
        thickness += stepSize;
      }
      t += stepSize;
    }
    float opacity = 1.0 - exp(-medium.medium.a*thickness);
    float3 fog = medium.medium.rgb*0.5;
    if ((medium.roles & PRIMITIVE_SELECTED) != 0) {
      fog = lerp(fog, float3(1.0, 0.55, 0.15), 0.3);
    }
    color = lerp(color, fog, opacity*0.8);
  }

  // Outline around the visible part of the selection, from rays that came close
  // but went past it (rays through a selected medium went inside instead)
  if (selectionPixels > 0.0 && selectionPixels < 2.0 &&
      (hit.index == ~0u || (primitives[hit.index].roles & PRIMITIVE_SELECTED) == 0)) {
    color = lerp(color, float3(1.0, 0.55, 0.15), 1.0 - smoothstep(1.0, 2.0, selectionPixels));
  }

  outputImage[globalId.xy] = float4(linearToSrgb(color), 1.0);
}
