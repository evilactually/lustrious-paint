// Canvas attributes, see CANVAS in SPECIFICATION.md. Each pixel stores its
// position as the distance from the canvas camera centre along the unit length
// ray through the pixel centre, 0 while it has none, and a world space normal,
// NO_NORMAL while it has none.
//
// Each pixel also refers to its BRDF by an index into a table of materials, 0
// (the first material) until a material is painted.
//
// paintMain transfers positions or normals from the construction geometry for
// one dab of a stroke, paints a material or erases all of them, rangeMain finds the range of the painted distances,
// renderMain adds samples of the light reflected towards the camera at each painted
// pixel and resolveMain then turns the attributes into the image the canvas shows.
// Push constants, render parameters and display modes must match LsCanvas.h and LsCanvas.cpp.

import scene;
import brdf;

static const uint DISPLAY_DEPTH    = 0;
static const uint DISPLAY_POSITION = 1;
static const uint DISPLAY_NORMAL   = 2;
static const uint DISPLAY_RENDERING = 3;
static const uint DISPLAY_MATERIAL  = 4;

static const uint DAB_DEPTH  = 0;
static const uint DAB_NORMAL = 1;
static const uint DAB_ERASE  = 2;
static const uint DAB_MATERIAL = 3;

struct CanvasPushConstants
{
  float4 origin;  // xyz = canvas camera position
  float4 right;   // scaled by tan(fov/2)*aspect
  float4 up;      // scaled by tan(fov/2)
  float4 forward; // unit length view direction plus the lens shift
  uint2 size;     // canvas size in pixels
  int2 offset;    // first pixel the dispatch covers
  float2 dabCenter; // in canvas pixels
  float dabRadius;  // in canvas pixels
  uint primitiveCount;
  float pixelAngle; // tan(fov/2)*2/height, footprint of a pixel one unit away
  uint display;     // DISPLAY_*
  float isolineSpacing; // view depth between isolines, 0 for none
  uint dabMode;     // DAB_*
  uint dabMaterial; // index into materials painted by DAB_MATERIAL
};

[[vk::push_constant]]
ConstantBuffer<CanvasPushConstants> canvas;

[[vk::binding(0)]]
[format("r32f")]
RWTexture2D<float> distanceImage;

[[vk::binding(1)]]
[format("rgba8")]
RWTexture2D<float4> displayImage;

[[vk::binding(2)]]
StructuredBuffer<ScenePrimitive> primitives;

// Nearest and furthest painted distance as float bits, which order like the
// floats themselves since distances are positive. Reset to ~0 and 0 before rangeMain.
[[vk::binding(3)]]
RWStructuredBuffer<uint> distanceRange;

// Octahedral encoded unit normal, x and y as 16 bit snorms in the low and high
// half. Two channel storage images are an optional feature, r32ui is not.
[[vk::binding(4)]]
[format("r32ui")]
RWTexture2D<uint> normalImage;

// Sum of the rendered samples of each pixel in rgb and their count in a, back to
// 0 whenever the pixel or what lights it changes
[[vk::binding(5)]]
[format("rgba32f")]
RWTexture2D<float4> radianceImage;

// Changed from the View panel, so kept out of the push constants
struct RenderParameters
{
  float exposure;      // multiplier of the rendered radiance
  uint samplesPerFrame;
  uint maxSamples;     // per pixel, rendering stops there
  uint pad0;
};

[[vk::binding(6)]]
ConstantBuffer<RenderParameters> rendering;

// Every distinct BRDF painted so far, mirrors LsCanvasMaterial in LsCanvas.cpp
struct Material
{
  float4 baseColor; // linear rgb, a = metallic
  float roughness;  // perceptual, squared for the Trowbridge-Reitz alpha
  float ior;        // of the dielectric coat
  float pad0;
  float pad1;
};

[[vk::binding(7)]]
StructuredBuffer<Material> materials;

// Index into materials of each pixel
[[vk::binding(8)]]
[format("r32ui")]
RWTexture2D<uint> materialImage;

Brdf materialBrdf(uint index)
{
  Material material = materials[index];
  Brdf brdf;
  brdf.baseColor = material.baseColor.rgb;
  brdf.metallic = saturate(material.baseColor.a);
  brdf.alpha = max(material.roughness*material.roughness, 1.0e-3);
  brdf.ior = material.ior;
  return brdf;
}

// -1 in both halves is never produced by encodeNormal, which rounds to +-32767
static const uint NO_NORMAL = 0x80008000;

float2 signNotZero(float2 v)
{
  return float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

// Folds the unit sphere onto an octahedron and that onto the [-1, 1] square
uint encodeNormal(float3 n)
{
  float2 p = n.xy/(abs(n.x) + abs(n.y) + abs(n.z));
  if (n.z < 0.0) {
    p = (1.0 - abs(p.yx))*signNotZero(p);
  }
  int2 q = int2(round(clamp(p, -1.0, 1.0)*32767.0));
  return (uint(q.x) & 0xFFFFu) | (uint(q.y) << 16);
}

float3 decodeNormal(uint bits)
{
  // Sign extended from each half
  float2 p = float2(float(int(bits << 16) >> 16), float(int(bits) >> 16))/32767.0;
  float3 n = float3(p, 1.0 - abs(p.x) - abs(p.y));
  if (n.z < 0.0) {
    n.xy = (1.0 - abs(n.yx))*signNotZero(n.xy);
  }
  return normalize(n);
}

static const float MAX_DISTANCE = 500.0;
static const int MAX_STEPS = 256;

// Through the pixel centre, canvas rows go down while the camera's up goes up
float3 pixelRay(uint2 pixel)
{
  float2 ndc = (float2(pixel) + 0.5)/float2(canvas.size)*2.0 - 1.0;
  ndc.y = -ndc.y;
  return normalize(canvas.forward.xyz + canvas.right.xyz*ndc.x + canvas.up.xyz*ndc.y);
}

static const int REFINE_STEPS = 16;

// Union of the construction geometry, skipping primitives that contain the
// camera, so a camera inside a room still sees its walls
float constructionDistance(float3 origin, float3 p)
{
  float distance = 1.0e30;
  for (uint i = 0; i < canvas.primitiveCount; i++) {
    ScenePrimitive primitive = primitives[i];
    if ((primitive.roles & ROLE_CONSTRUCTION) == 0 || primitiveDistance(primitive, origin) < 0.0) {
      continue;
    }
    distance = min(distance, primitiveDistance(primitive, p));
  }
  return distance;
}

// Sphere traces the construction geometry, returns the distance to the first hit
// or 0 for a miss
float traceConstruction(float3 origin, float3 direction)
{
  float t = 0.0;
  for (int step = 0; step < MAX_STEPS && t < MAX_DISTANCE; step++) {
    float distance = constructionDistance(origin, origin + direction*t);
    if (distance >= 1.0e29) {
      return 0.0; // nothing to hit
    }
    float footprint = max(t, 1.0e-3)*canvas.pixelAngle;
    if (distance < 0.25*footprint) {
      // Within a pixel is enough to find the surface, but the stored position
      // should be on it: neighbouring pixels on a plane would otherwise get
      // depths scattered by the tolerance, which shows as noise in isolines
      // and will in normals and shading. Signed distances step back out too.
      for (int refine = 0; refine < REFINE_STEPS; refine++) {
        distance = constructionDistance(origin, origin + direction*t);
        if (abs(distance) < 1.0e-6*max(t, 1.0)) {
          break;
        }
        t += distance;
      }
      return max(t, 1.0e-6);
    }
    t += distance;
  }
  return 0.0;
}

// Gradient of the construction distance just in front of a hit at distance t,
// facing the camera. Measured on the camera side since the field of a two sided
// plane has no gradient on the surface itself. The step is a fraction of the
// pixel footprint, so edges blend over about a pixel.
float3 constructionNormal(float3 origin, float3 direction, float t)
{
  float h = max(0.25*t*canvas.pixelAngle, 1.0e-5*max(t, 1.0));
  float3 p = origin + direction*(t - 2.0*h);
  // Tetrahedron of samples, four distances instead of six for central differences
  const float2 k = float2(1.0, -1.0);
  float3 n = k.xyy*constructionDistance(origin, p + k.xyy*h) +
             k.yyx*constructionDistance(origin, p + k.yyx*h) +
             k.yxy*constructionDistance(origin, p + k.yxy*h) +
             k.xxx*constructionDistance(origin, p + k.xxx*h);
  if (dot(n, n) < 1.0e-20) {
    return -direction;
  }
  n = normalize(n);
  return dot(n, direction) > 0.0 ? -n : n;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void paintMain(uint3 globalId : SV_DispatchThreadID)
{
  int2 pixel = canvas.offset + int2(globalId.xy);
  if (any(pixel < 0) || any(pixel >= int2(canvas.size))) {
    return;
  }
  float2 delta = float2(pixel) + 0.5 - canvas.dabCenter;
  if (dot(delta, delta) >= canvas.dabRadius*canvas.dabRadius) {
    return;
  }

  // Whatever a dab changes is rendered again from the start
  if (canvas.dabMode == DAB_ERASE) {
    distanceImage[pixel] = 0.0;
    normalImage[pixel] = NO_NORMAL;
    materialImage[pixel] = 0;
    radianceImage[pixel] = float4(0.0);
    return;
  }
  // Also where there is no position yet, so materials can be painted first
  if (canvas.dabMode == DAB_MATERIAL) {
    if (materialImage[pixel] != canvas.dabMaterial) {
      materialImage[pixel] = canvas.dabMaterial;
      radianceImage[pixel] = float4(0.0);
    }
    return;
  }

  // Pixels whose ray misses keep what they had
  float3 direction = pixelRay(uint2(pixel));
  float t = traceConstruction(canvas.origin.xyz, direction);
  if (t <= 0.0) {
    return;
  }
  if (canvas.dabMode == DAB_NORMAL) {
    normalImage[pixel] = encodeNormal(constructionNormal(canvas.origin.xyz, direction, t));
  }
  else {
    distanceImage[pixel] = t;
  }
  radianceImage[pixel] = float4(0.0);
}

// World position of a painted pixel
float3 pixelPosition(int2 pixel, float t)
{
  return canvas.origin.xyz + pixelRay(uint2(pixel))*t;
}

// For pixels with a position but no painted normal: the surface through the
// positions of the neighbouring pixels, facing the camera. Of the two neighbours
// along each axis the nearer one is used, so the normal doesn't bend across a
// depth step at an object edge.
float3 positionNormal(int2 pixel, float3 p, float3 direction)
{
  float3 tangents[2] = { float3(0.0), float3(0.0) };
  int2 steps[2] = { int2(1, 0), int2(0, 1) };
  for (int axis = 0; axis < 2; axis++) {
    float best = 1.0e30;
    for (int side = -1; side <= 1; side += 2) {
      int2 neighbour = pixel + steps[axis]*side;
      if (any(neighbour < 0) || any(neighbour >= int2(canvas.size))) {
        continue;
      }
      float t = distanceImage[neighbour];
      if (t <= 0.0) {
        continue;
      }
      float3 d = (pixelPosition(neighbour, t) - p)*float(side);
      if (dot(d, d) < best) {
        best = dot(d, d);
        tangents[axis] = d;
      }
    }
  }
  float3 n = cross(tangents[0], tangents[1]);
  if (dot(n, n) < 1.0e-20) {
    return -direction;
  }
  n = normalize(n);
  return dot(n, direction) > 0.0 ? -n : n;
}

//-------------------------------------------------------------------------------
// Rendering: the light reflected towards the camera at each painted point, from
// the scene's emitters with its shadow casters in the way. Only direct light,
// light that reaches the point after bouncing off anything else is left out.
// Each sample combines a direction towards an emitter with one from the BRDF by
// multiple importance sampling, see renderSample.
//-------------------------------------------------------------------------------

static const uint LIGHT_ROLES = ROLE_SHADOW_CASTER | ROLE_LIGHT_EMITTER;

// PCG hash, Jarzynski and Olano, "Hash Functions for GPU Rendering"
uint pcgHash(uint v)
{
  uint state = v*747796405u + 2891336453u;
  uint word = ((state >> ((state >> 28u) + 4u)) ^ state)*277803737u;
  return (word >> 22u) ^ word;
}

struct Random
{
  uint state;

  __init(uint2 pixel, uint sampleIndex)
  {
    state = pcgHash(pixel.x ^ pcgHash(pixel.y ^ pcgHash(sampleIndex)));
  }

  // Uniform in [0, 1)
  [mutating]
  float next()
  {
    state = pcgHash(state);
    return float(state >> 8)*(1.0/16777216.0);
  }
};

// Duff et al., "Building an Orthonormal Basis, Revisited"
void orthonormalBasis(float3 n, out float3 t, out float3 b)
{
  float s = n.z >= 0.0 ? 1.0 : -1.0;
  float a = -1.0/(s + n.z);
  float c = n.x*n.y*a;
  t = float3(1.0 + s*n.x*n.x*a, s*c, -s*n.x);
  b = float3(c, s + n.y*n.y*a, -n.y);
}

struct ShadingFrame
{
  float3 t, b, n;

  __init(float3 normal)
  {
    n = normal;
    orthonormalBasis(n, t, b);
  }

  float3 toLocal(float3 w) { return float3(dot(w, t), dot(w, b), dot(w, n)); }
  float3 toWorld(float3 w) { return t*w.x + b*w.y + n*w.z; }
};

// Emitted radiance arriving at origin from direction w: the emission of the
// first shadow caster or emitter along the ray, if that is an emitter. An
// emitter that is also a shadow caster still emits. Rays that leave the scene,
// or run out of steps grazing along a surface, bring no light.
float3 incidentRadiance(float3 origin, float3 w)
{
  float t = 0.0;
  for (int step = 0; step < MAX_STEPS && t < MAX_DISTANCE; step++) {
    uint closest;
    float distance = sceneDistance(primitives, canvas.primitiveCount, origin + w*t, LIGHT_ROLES, closest);
    if (closest == ~0u) {
      break;
    }
    if (distance < 1.0e-4*max(t, 1.0)) {
      ScenePrimitive primitive = primitives[closest];
      return (primitive.roles & ROLE_LIGHT_EMITTER) != 0 ? primitive.emission.rgb : float3(0.0);
    }
    t += distance;
  }
  return float3(0.0);
}

// Emitters are sampled uniformly within the cone of directions towards the
// sphere around them, which works for any shape. Directions in the cone that
// miss the emitter itself are wasted, but the density is known for any direction.
struct EmitterCone
{
  float3 axis;
  float cosMax;
  float solidAngle; // 0 for emitters without bounds, which can't be sampled

  __init(ScenePrimitive emitter, float3 p)
  {
    float radius = emitter.emission.a;
    float3 toCenter = primitiveCenter(emitter) - p;
    float distance2 = dot(toCenter, toCenter);
    axis = float3(0.0, 0.0, 1.0);
    cosMax = -1.0;
    solidAngle = 0.0;
    if (radius <= 0.0) {
      return;
    }
    if (distance2 <= radius*radius) {
      // Inside the sphere any direction may reach the emitter
      solidAngle = 4.0*PI;
      return;
    }
    axis = toCenter*rsqrt(distance2);
    float sin2Max = radius*radius/distance2;
    cosMax = sqrt(1.0 - sin2Max);
    // 1 - cosMax without the cancellation for small, distant emitters
    solidAngle = 2.0*PI*sin2Max/(1.0 + cosMax);
  }

  bool contains(float3 w)
  {
    return solidAngle > 0.0 && dot(w, axis) >= cosMax;
  }

  float3 sample(float2 u)
  {
    float cosTheta = 1.0 - u.x*solidAngle/(2.0*PI);
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta*cosTheta));
    float phi = 2.0*PI*u.y;
    float3 t, b;
    orthonormalBasis(axis, t, b);
    return normalize(t*(sinTheta*cos(phi)) + b*(sinTheta*sin(phi)) + axis*cosTheta);
  }
};

bool isSampledEmitter(ScenePrimitive primitive)
{
  return (primitive.roles & ROLE_LIGHT_EMITTER) != 0 && primitive.emission.a > 0.0;
}

uint sampledEmitterCount()
{
  uint count = 0;
  for (uint i = 0; i < canvas.primitiveCount; i++) {
    if (isSampledEmitter(primitives[i])) {
      count++;
    }
  }
  return count;
}

// Density over directions from p of picking an emitter uniformly and then a
// direction uniformly in its cone
float emitterPdf(float3 p, float3 w, uint emitterCount)
{
  if (emitterCount == 0) {
    return 0.0;
  }
  float pdf = 0.0;
  for (uint i = 0; i < canvas.primitiveCount; i++) {
    if (!isSampledEmitter(primitives[i])) {
      continue;
    }
    EmitterCone cone = EmitterCone(primitives[i], p);
    if (cone.contains(w)) {
      pdf += 1.0/cone.solidAngle;
    }
  }
  return pdf/float(emitterCount);
}

// u.z picks the emitter
float3 sampleEmitters(float3 p, uint emitterCount, float3 u)
{
  uint picked = min(uint(u.z*float(emitterCount)), emitterCount - 1);
  for (uint i = 0; i < canvas.primitiveCount; i++) {
    if (!isSampledEmitter(primitives[i])) {
      continue;
    }
    if (picked == 0) {
      return EmitterCone(primitives[i], p).sample(u.xy);
    }
    picked--;
  }
  return float3(0.0, 0.0, 1.0);
}

// One sample of the light reflected towards the camera, the rendering equation
// without bounces
//
//   Lo(wo) = integral over the hemisphere of f(wo, wi) Le(wi) cos(wi) dwi
//
// estimated with one direction from each of the two strategies, weighted by the
// balance heuristic. With one direction each, a direction's weighted
// contribution is f Le cos/(pdfEmitters + pdfBrdf), whichever strategy drew it.
// Sampling emitters finds small lights, sampling the BRDF finds glossy
// reflections of large ones.
float3 renderSample(float3 p, float3 origin, ShadingFrame frame, float3 wo, Brdf brdf, uint emitterCount, inout Random random)
{
  float3 radiance = float3(0.0);
  float3 woLocal = frame.toLocal(wo);

  for (int strategy = 0; strategy < 2; strategy++) {
    float3 u = float3(random.next(), random.next(), random.next());
    float3 wi;
    if (strategy == 0) {
      if (emitterCount == 0) {
        continue;
      }
      wi = sampleEmitters(p, emitterCount, u);
    }
    else {
      wi = normalize(frame.toWorld(sampleBrdf(brdf, woLocal, u)));
    }
    float3 wiLocal = frame.toLocal(wi);
    if (wiLocal.z <= 0.0) {
      continue;
    }
    float3 f = evaluateBrdf(brdf, woLocal, wiLocal);
    float pdf = emitterPdf(p, wi, emitterCount) + brdfPdf(brdf, woLocal, wiLocal);
    if (any(f > 0.0) && pdf > 0.0) {
      radiance += f*incidentRadiance(origin, wi)*(wiLocal.z/pdf);
    }
  }
  return radiance;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void renderMain(uint3 globalId : SV_DispatchThreadID)
{
  if (any(globalId.xy >= canvas.size)) {
    return;
  }
  int2 pixel = int2(globalId.xy);
  float t = distanceImage[pixel];
  float4 accumulated = radianceImage[pixel];
  // Pixels without a position stay black
  if (t <= 0.0 || accumulated.a >= float(rendering.maxSamples)) {
    return;
  }

  float3 direction = pixelRay(globalId.xy);
  float3 p = canvas.origin.xyz + direction*t;
  uint bits = normalImage[pixel];
  float3 n = bits != NO_NORMAL ? decodeNormal(bits) : positionNormal(pixel, p, direction);
  // A painted normal can face away from the camera, but the point is seen from
  // the camera anyway, so it is shaded as two sided
  float3 wo = -direction;
  if (dot(n, wo) < 0.0) {
    n = -n;
  }
  ShadingFrame frame = ShadingFrame(n);

  Brdf brdf = materialBrdf(materialImage[pixel]);

  // Rays leave from just above the surface, so they don't find it again
  float3 origin = p + n*(1.0e-3*max(t, 1.0));
  uint emitterCount = sampledEmitterCount();
  uint first = uint(accumulated.a);
  uint count = min(rendering.samplesPerFrame, rendering.maxSamples - first);
  float3 sum = float3(0.0);
  for (uint i = 0; i < count; i++) {
    Random random = Random(globalId.xy, first + i);
    sum += renderSample(p, origin, frame, wo, brdf, emitterCount, random);
  }
  radianceImage[pixel] = accumulated + float4(sum, float(count));
}

float3 linearToSrgb(float3 c)
{
  c = saturate(c);
  return select(c <= 0.0031308, c*12.92, 1.055*pow(c, 1.0/2.4) - 0.055);
}

// Planar depth along the view axis, -1 without a position. Unlike the stored
// distance it is constant over a wall facing the camera, so depth shading and
// isolines show form rather than circles around the view centre.
float viewDepth(int2 pixel)
{
  float t = distanceImage[pixel];
  if (t <= 0.0) {
    return -1.0;
  }
  // right and up are perpendicular, scaled by the field of view, and forward
  // also carries the lens shift, so the view axis is taken from right and up
  float3 axis = normalize(cross(canvas.up.xyz, canvas.right.xyz));
  return t*dot(pixelRay(uint2(pixel)), axis);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void rangeMain(uint3 globalId : SV_DispatchThreadID)
{
  if (any(globalId.xy >= canvas.size)) {
    return;
  }
  float z = viewDepth(int2(globalId.xy));
  if (z > 0.0) {
    InterlockedMin(distanceRange[0], asuint(z));
    InterlockedMax(distanceRange[1], asuint(z));
  }
}

// How much of the pixel an isoline covers. Lines lie where the depth crosses a
// multiple of the spacing and stay about a pixel wide however steep the surface
// is, by measuring the distance to the line in pixels with the depth gradient.
// Every tenth line is a major line. Lines don't cross into unpainted pixels.
float isolineCoverage(int2 pixel, float z, out bool major)
{
  major = false;
  if (canvas.isolineSpacing <= 0.0) {
    return 0.0;
  }
  float f = z/canvas.isolineSpacing;
  // Central differences where both neighbours are painted, one sided otherwise
  float2 gradient = float2(0.0);
  int2 steps[2] = { int2(1, 0), int2(0, 1) };
  for (int axis = 0; axis < 2; axis++) {
    int2 step = steps[axis];
    int2 next = clamp(pixel + step, int2(0), int2(canvas.size) - 1);
    int2 previous = clamp(pixel - step, int2(0), int2(canvas.size) - 1);
    float zNext = viewDepth(next);
    float zPrevious = viewDepth(previous);
    float d = 0.0;
    if (zNext > 0.0 && zPrevious > 0.0) {
      d = 0.5*(zNext - zPrevious);
    }
    else if (zNext > 0.0) {
      d = zNext - z;
    }
    else if (zPrevious > 0.0) {
      d = z - zPrevious;
    }
    gradient[axis] = d/canvas.isolineSpacing;
  }
  float perPixel = length(gradient);
  // A surface lying in a contour plane, such as a wall facing the camera at a
  // multiple of the spacing, has no meaningful line and would show float noise.
  // Steps such as an object edge in front of a wall would draw solid bands.
  if (perPixel < 2.0e-3 || perPixel > 0.5) {
    return 0.0;
  }
  float nearestLine = round(f);
  major = fmod(abs(nearestLine), 10.0) < 0.5;
  float pixels = abs(f - nearestLine)/perPixel;
  float halfWidth = major ? 0.9 : 0.6;
  return saturate(halfWidth + 0.5 - pixels);
}

// Depth shows the planar depth in grey, white at the nearest painted pixel and
// dark grey at the furthest, so it stays readable at any scene scale. Position
// shows the absolute world coordinates in RGB, clamped to [0, 1] by the image
// format. Normal shows the world space normal mapped to [0, 1]. Material shows
// the base colour of each pixel's material, also where there is no position. Rendering shows
// the mean of the rendered samples, exposed and sRGB encoded. Isolines are drawn
// over any of them but the rendering where there is a position. Pixels without
// the shown attribute are black.
[shader("compute")]
[numthreads(8, 8, 1)]
void resolveMain(uint3 globalId : SV_DispatchThreadID)
{
  if (any(globalId.xy >= canvas.size)) {
    return;
  }
  int2 pixel = int2(globalId.xy);
  float t = distanceImage[pixel];
  float3 color = float3(0.0);
  if (canvas.display == DISPLAY_RENDERING) {
    float4 accumulated = radianceImage[pixel];
    if (accumulated.a > 0.0) {
      color = linearToSrgb(accumulated.rgb/accumulated.a*rendering.exposure);
    }
    displayImage[globalId.xy] = float4(color, 1.0);
    return;
  }
  if (canvas.display == DISPLAY_MATERIAL) {
    color = linearToSrgb(materials[materialImage[pixel]].baseColor.rgb);
  }
  else if (canvas.display == DISPLAY_NORMAL) {
    uint bits = normalImage[pixel];
    if (bits != NO_NORMAL) {
      color = decodeNormal(bits)*0.5 + 0.5;
    }
  }
  else if (t > 0.0) {
    if (canvas.display == DISPLAY_POSITION) {
      color = canvas.origin.xyz + pixelRay(globalId.xy)*t;
    }
    else {
      float nearest = asfloat(distanceRange[0]);
      float furthest = asfloat(distanceRange[1]);
      float depth = furthest > nearest ? (viewDepth(pixel) - nearest)/(furthest - nearest) : 0.0;
      color = lerp(1.0, 0.15, depth);
    }
  }
  if (t > 0.0) {
    float z = viewDepth(pixel);
    bool major;
    float coverage = isolineCoverage(pixel, z, major);
    // A faint shade of what is under the line, darker over light surfaces and
    // lighter over dark ones, so the lines read without competing with the depth
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    float shift = (luminance > 0.5 ? -1.0 : 1.0)*(major ? 0.14 : 0.08);
    color = saturate(color + shift*coverage);
  }
  displayImage[globalId.xy] = float4(color, 1.0);
}
