// Canvas attributes, see CANVAS in SPECIFICATION.md. Each pixel stores its
// position as the distance from the canvas camera centre along the unit length
// ray through the pixel centre, 0 while it has none.
//
// paintMain transfers positions from the construction geometry for one dab of a
// stroke, rangeMain finds the range of the painted distances and resolveMain
// then turns the attributes into the image the canvas shows.
// Push constants and display modes must match LsCanvas.h and LsCanvas.cpp.

import scene;

static const uint DISPLAY_DEPTH    = 0;
static const uint DISPLAY_POSITION = 1;

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
  float pad2;
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

  // Pixels whose ray misses keep what they had
  float t = traceConstruction(canvas.origin.xyz, pixelRay(uint2(pixel)));
  if (t > 0.0) {
    distanceImage[pixel] = t;
  }
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
// format. Isolines are drawn over either. Pixels without a position are black.
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
  if (t > 0.0) {
    float z = viewDepth(pixel);
    if (canvas.display == DISPLAY_POSITION) {
      color = canvas.origin.xyz + pixelRay(globalId.xy)*t;
    }
    else {
      float nearest = asfloat(distanceRange[0]);
      float furthest = asfloat(distanceRange[1]);
      float depth = furthest > nearest ? (z - nearest)/(furthest - nearest) : 0.0;
      color = lerp(1.0, 0.15, depth);
    }
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
