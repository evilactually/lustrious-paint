// Canvas attributes, see CANVAS in SPECIFICATION.md. Each pixel stores its
// position as the distance from the canvas camera centre along the unit length
// ray through the pixel centre, 0 while it has none, and a world space normal,
// NO_NORMAL while it has none.
//
// Each pixel also stores the parameters of its BRDF, the default material until
// one is painted, so materials blend like any other attribute, and a mask of
// the light rig painted on it, see Rig.
//
// paintMain transfers positions or normals from the construction geometry for
// one dab of a stroke, paints a material or erases all of them, blending from
// the attributes as they were when the stroke started by how much the stroke
// covers each pixel. blendMain smooths them instead for one dab of a blend
// stroke. rangeMain finds the range of the painted distances,
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
static const uint DISPLAY_RIGS      = 5;
static const uint DISPLAY_MATCAP    = 6;

static const uint DAB_DEPTH  = 0;
static const uint DAB_NORMAL = 1;
static const uint DAB_ERASE  = 2;
static const uint DAB_MATERIAL = 3;
// Paints the light rig, see paintMain
static const uint DAB_LIGHT = 4;
// Set with a mode for a blend dab, which smooths what the mode paints, or
// everything for DAB_ERASE
static const uint DAB_BLEND = 1u << 8;
// Set with DAB_NORMAL to paint the normal in dabMaterialColor instead of the
// construction geometry's, and with DAB_LIGHT to paint the one in
// dabMaterialSurface with the rig
static const uint DAB_FIXED_NORMAL = 1u << 9;
// Set with DAB_LIGHT to take the rig in dabMaterialColor off the pixels that
// have it, for removing a rig from every pixel
static const uint DAB_CLEAR_RIG = 1u << 10;

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
  // Packed material painted by DAB_MATERIAL, or left by DAB_ERASE. Two uints
  // rather than a uint2, which would be aligned to 8 bytes. A blend dab has the
  // bits of its edge threshold in the first instead, see blendEdgeThreshold.
  uint dabMaterialColor;
  uint dabMaterialSurface;
  float dabOpacity;  // coverage at the centre of the dab
  float dabFalloff;  // fraction of the radius over which the dab fades out
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
  uint localLightSlots;  // slots of localLights in use
  uint occluderSlots;    // slots of occluders in use
  uint pad0;
  uint pad1;
  uint pad2;
};

[[vk::binding(6)]]
ConstantBuffer<RenderParameters> rendering;

// BRDF parameters of each pixel, packed by packMaterial: the sRGB encoded base
// colour and metallic as 8 bit unorms in the first image, roughness and the IOR
// as 16 bit unorms in the second. Packed by hand like the normal.
[[vk::binding(7)]]
[format("r32ui")]
RWTexture2D<uint> materialColorImage;

[[vk::binding(8)]]
[format("r32ui")]
RWTexture2D<uint> materialSurfaceImage;

// The attributes as they were when the stroke started, and how much the stroke
// covers each pixel so far. Dabs blend from these, so overlapping dabs of a
// stroke don't add up past its opacity.
[[vk::binding(9)]]
[format("r32f")]
RWTexture2D<float> strokeDistanceImage;

[[vk::binding(10)]]
[format("r32ui")]
RWTexture2D<uint> strokeNormalImage;

[[vk::binding(11)]]
[format("r32ui")]
RWTexture2D<uint> strokeMaterialColorImage;

[[vk::binding(12)]]
[format("r32ui")]
RWTexture2D<uint> strokeMaterialSurfaceImage;

[[vk::binding(13)]]
[format("r32f")]
RWTexture2D<float> strokeCoverageImage;

// Local lights, which the artist places around the painting rather than in the
// scene: each lights from a direction at infinity, a small disk of sky of
// uniform radiance. A pixel is lit by the scene's emitters and the local lights
// of the light rig painted on it by light strokes, and shadowed by the rig's
// occluders. Each pixel has the index of its rig in rigs, 0 for none.
[[vk::binding(14)]]
[format("r32ui")]
RWTexture2D<uint> rigImage;

// Mirrors LsCanvasLocalLight in LsCanvas.cpp, one for each of 32 slots
struct LocalLight
{
  float4 direction; // xyz = world space unit vector towards the light, w = cosine of its angular radius
  float4 radiance;  // rgb = radiance of the disk, w = its solid angle
  uint shadowed;    // 0 when the scene's shadow casters don't block it
  uint pad0;
  uint pad1;
  uint pad2;
};

[[vk::binding(15)]]
StructuredBuffer<LocalLight> localLights;

// Occluders shadow a pixel from a cap of directions, as if something the scene
// doesn't have were in the way there, when they are in its rig. Mirrors
// LsCanvasOccluder in LsCanvas.cpp, one for each of 32 slots.
struct Occluder
{
  float4 direction; // xyz = world space unit vector towards its centre, w = cosine of its angular radius
  float4 shape;     // x = cosine of the radius it blocks fully within, y = strength
};

[[vk::binding(17)]]
StructuredBuffer<Occluder> occluders;

// A light rig: local lights and occluders, as bits of their slots. Mirrors
// LsCanvasRig in LsCanvas.cpp, rig 0 has nothing.
struct Rig
{
  uint lights;
  uint occluders;
  uint color; // for showing where it is painted, sRGB rgba8
  uint pad;
};

static const uint MAX_RIGS = 256;

[[vk::binding(18)]]
StructuredBuffer<Rig> rigs;

Rig pixelRig(int2 pixel)
{
  return rigs[min(rigImage[pixel], MAX_RIGS - 1)];
}

// How many painted pixels have each rig, gathered by rigCountMain
[[vk::binding(19)]]
RWStructuredBuffer<uint> rigCounts;

// Written by pickMain and read back by the host: xyz = sum of the normals, w = their count
[[vk::binding(16)]]
RWStructuredBuffer<float4> pickResult;

// The IOR range a material can store, mirrors LsCanvas.cpp
static const float MIN_IOR = 1.0;
static const float MAX_IOR = 3.0;

// BRDF parameters as stored, the base colour still sRGB encoded so that it
// blends the way paint is mixed in a colour picker
struct Material
{
  float3 baseColor; // sRGB encoded
  float metallic;
  float roughness;  // perceptual
  float ior;

  __init(uint2 packed)
  {
    float4 color = float4(packed.x & 0xFFu, (packed.x >> 8) & 0xFFu, (packed.x >> 16) & 0xFFu, packed.x >> 24)/255.0;
    baseColor = color.rgb;
    metallic = color.a;
    roughness = float(packed.y & 0xFFFFu)/65535.0;
    ior = lerp(MIN_IOR, MAX_IOR, float(packed.y >> 16)/65535.0);
  }

  uint2 pack()
  {
    uint4 color = uint4(round(saturate(float4(baseColor, metallic))*255.0));
    uint surface = uint(round(saturate(roughness)*65535.0)) |
                   (uint(round(saturate((ior - MIN_IOR)/(MAX_IOR - MIN_IOR))*65535.0)) << 16);
    return uint2(color.r | (color.g << 8) | (color.b << 16) | (color.a << 24), surface);
  }

  Material blend(Material other, float weight)
  {
    Material result = this;
    result.baseColor = lerp(baseColor, other.baseColor, weight);
    result.metallic = lerp(metallic, other.metallic, weight);
    result.roughness = lerp(roughness, other.roughness, weight);
    result.ior = lerp(ior, other.ior, weight);
    return result;
  }

  Brdf brdf()
  {
    Brdf result;
    result.baseColor = srgbToLinear(baseColor);
    result.metallic = metallic;
    result.alpha = max(roughness*roughness, 1.0e-3);
    result.ior = ior;
    return result;
  }
};

Material pixelMaterial(int2 pixel)
{
  return Material(uint2(materialColorImage[pixel], materialSurfaceImage[pixel]));
}

Material strokeMaterial(int2 pixel)
{
  return Material(uint2(strokeMaterialColorImage[pixel], strokeMaterialSurfaceImage[pixel]));
}

void writeMaterial(int2 pixel, uint2 packed)
{
  materialColorImage[pixel] = packed.x;
  materialSurfaceImage[pixel] = packed.y;
}

float3 srgbToLinear(float3 c)
{
  return select(c <= 0.04045, c/12.92, pow((c + 0.055)/1.055, 2.4));
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
  float r = length(delta)/canvas.dabRadius;
  if (r >= 1.0) {
    return;
  }
  // The stroke covers each pixel as much as its strongest dab did, so a dab
  // that doesn't cover a pixel more than that leaves it as it is
  float coverage = dabCoverage(r);
  if (coverage <= strokeCoverageImage[pixel]) {
    return;
  }

  // Whatever a dab changes is rendered again from the start
  if (canvas.dabMode == DAB_ERASE) {
    strokeCoverageImage[pixel] = coverage;
    // A position can't be partly there: a pixel is erased where the coverage
    // passes its own threshold, which dithers soft edges
    if (coverage > ditherThreshold(pixel)) {
      distanceImage[pixel] = 0.0;
      normalImage[pixel] = NO_NORMAL;
      writeMaterial(pixel, dabMaterial());
      rigImage[pixel] = 0;
      radianceImage[pixel] = float4(0.0);
    }
    return;
  }
  // The pixel takes the rig in dabMaterialColor, which can't be partly there,
  // so soft edges are dithered like erasing. With DAB_CLEAR_RIG the pixels
  // that have that rig go back to none instead, for removing a rig from every
  // pixel. With DAB_FIXED_NORMAL the rig also carries the normal in
  // dabMaterialSurface, which blends like a normal stroke.
  if ((canvas.dabMode & 0xFFu) == DAB_LIGHT) {
    strokeCoverageImage[pixel] = coverage;
    if (coverage > ditherThreshold(pixel)) {
      uint before = rigImage[pixel];
      uint after = canvas.dabMaterialColor;
      if ((canvas.dabMode & DAB_CLEAR_RIG) != 0) {
        after = before == canvas.dabMaterialColor ? 0u : before;
      }
      if (after != before) {
        rigImage[pixel] = after;
        radianceImage[pixel] = float4(0.0);
      }
    }
    if ((canvas.dabMode & DAB_FIXED_NORMAL) != 0) {
      paintNormal(pixel, decodeNormal(canvas.dabMaterialSurface), coverage);
    }
    return;
  }
  // A normal chosen by the artist, also where there is no position yet
  if (canvas.dabMode == (DAB_NORMAL | DAB_FIXED_NORMAL)) {
    strokeCoverageImage[pixel] = coverage;
    paintNormal(pixel, decodeNormal(canvas.dabMaterialColor), coverage);
    return;
  }
  // Also where there is no position yet, so materials can be painted first
  if (canvas.dabMode == DAB_MATERIAL) {
    strokeCoverageImage[pixel] = coverage;
    Material target = Material(dabMaterial());
    writeMaterial(pixel, strokeMaterial(pixel).blend(target, coverage).pack());
    radianceImage[pixel] = float4(0.0);
    return;
  }

  // Pixels whose ray misses keep what they had
  float3 direction = pixelRay(uint2(pixel));
  float t = traceConstruction(canvas.origin.xyz, direction);
  if (t <= 0.0) {
    return;
  }
  strokeCoverageImage[pixel] = coverage;
  if (canvas.dabMode == DAB_NORMAL) {
    paintNormal(pixel, constructionNormal(canvas.origin.xyz, direction, t), coverage);
    return;
  }
  // Blended along the ray, a pixel without a position takes the new one
  float before = strokeDistanceImage[pixel];
  distanceImage[pixel] = before > 0.0 ? lerp(before, t, coverage) : t;
  radianceImage[pixel] = float4(0.0);
}

// Blended from the normal at the start of the stroke and normalised. A pixel
// without a painted normal blends from the one its position and its
// neighbours' imply, as it is rendered with, so soft edges stay soft. Without a
// position either it takes the new one.
void paintNormal(int2 pixel, float3 n, float coverage)
{
  uint before = strokeNormalImage[pixel];
  float t = distanceImage[pixel];
  bool blend = before != NO_NORMAL || t > 0.0;
  if (blend) {
    float3 direction = pixelRay(uint2(pixel));
    float3 from = before != NO_NORMAL ? decodeNormal(before) : positionNormal(pixel, canvas.origin.xyz + direction*t, direction);
    float3 blended = lerp(from, n, coverage);
    n = dot(blended, blended) > 1.0e-8 ? normalize(blended) : n;
  }
  normalImage[pixel] = encodeNormal(n);
  radianceImage[pixel] = float4(0.0);
}

// Blends each pixel a dab covers towards a weighted average of its
// neighbourhood, by the dab's coverage. Unlike painting, dabs compound, so
// going over an area again smooths it further. Only attributes a pixel has are
// blended, blending never adds a position. Normals blend as they are rendered,
// so a pixel with a position but no painted normal gets the blended one.
//
// The neighbours are read while the dab blends them, so every pixel of the dab's
// rectangle is written to the stroke images, blended or as it was, and the
// rectangle is copied back afterwards.
[shader("compute")]
[numthreads(8, 8, 1)]
void blendMain(uint3 globalId : SV_DispatchThreadID)
{
  int2 pixel = canvas.offset + int2(globalId.xy);
  if (any(pixel < 0) || any(pixel >= int2(canvas.size))) {
    return;
  }
  float t = distanceImage[pixel];
  uint normalBits = normalImage[pixel];
  uint2 material = uint2(materialColorImage[pixel], materialSurfaceImage[pixel]);

  float r = length(float2(pixel) + 0.5 - canvas.dabCenter)/canvas.dabRadius;
  float coverage = r < 1.0 ? dabCoverage(r) : 0.0;
  uint mode = canvas.dabMode & ~DAB_BLEND;
  bool depth = mode == DAB_DEPTH || mode == DAB_ERASE;
  bool normals = mode == DAB_NORMAL || mode == DAB_ERASE;
  bool materials = mode == DAB_MATERIAL || mode == DAB_ERASE;
  if (coverage > 0.0) {
    // 9 by 9 taps with Gaussian weights, spread with the radius so that a large
    // brush smooths large forms. Sparser taps alias across depth steps.
    float spacing = max(1.0, canvas.dabRadius/16.0);
    // Weighted least squares sums for fitting a plane to the inverse depth
    PlaneFit plane = PlaneFit();
    float3 normalSum = float3(0.0);
    float normalWeight = 0.0;
    float4 colorSum = float4(0.0);
    float2 surfaceSum = float2(0.0);
    float materialWeight = 0.0;
    // Neighbours further in front or behind than this fraction of the pixel's
    // view depth are on another surface, such as the wall behind an object's
    // edge, and are left out, as are neighbours without a position. They fade
    // out from half the threshold, a hard cut would flip neighbours in and out
    // between adjacent pixels. 0 blends across everything.
    float edgeThreshold = blendEdgeThreshold();
    float zPixel = t > 0.0 ? viewDepth(pixel) : -1.0;
    for (int y = -4; y <= 4; y++) {
      for (int x = -4; x <= 4; x++) {
        int2 tap = clamp(pixel + int2(round(float2(x, y)*spacing)), int2(0), int2(canvas.size) - 1);
        float w = exp(-0.125*float(x*x + y*y));
        float z = viewDepth(tap);
        if (edgeThreshold > 0.0 && zPixel > 0.0) {
          if (z <= 0.0) {
            continue;
          }
          float difference = abs(z - zPixel)/(edgeThreshold*zPixel);
          w *= 1.0 - smoothstep(0.5, 1.0, difference);
          if (w <= 0.0) {
            continue;
          }
        }
        if (depth && z > 0.0) {
          plane.add(float2(tap - pixel)/spacing, 1.0/z, w);
        }
        if (normals) {
          // As rendered, painted or from the positions, so the shading of shapes
          // blends without normals painted on them first
          float3 n;
          if (renderedNormal(tap, n)) {
            normalSum += w*n;
            normalWeight += w;
          }
        }
        if (materials) {
          Material m = pixelMaterial(tap);
          colorSum += w*float4(m.baseColor, m.metallic);
          surfaceSum += w*float2(m.roughness, m.ior);
          materialWeight += w;
        }
      }
    }

    bool changed = false;
    if (t > 0.0 && plane.weight > 0.0) {
      // Inverse view depth is affine over the image for a plane, so the fitted
      // plane through the neighbours keeps planes flat, also where neighbours
      // on one side are left out, which would pull a plain average towards the
      // depth of the others. Curved surfaces are smoothed as by the average.
      float z = viewDepth(pixel);
      float inverseDepth = lerp(1.0/z, plane.valueAtCenter(), coverage);
      // Along the pixel's ray, so the distance scales with the view depth
      t *= 1.0/(inverseDepth*z);
      changed = true;
    }
    float3 normalBefore;
    if (renderedNormal(pixel, normalBefore) && normalWeight > 0.0 && dot(normalSum, normalSum) > 1.0e-12) {
      float3 blended = lerp(normalBefore, normalize(normalSum), coverage);
      if (dot(blended, blended) > 1.0e-8) {
        normalBits = encodeNormal(normalize(blended));
        changed = true;
      }
    }
    if (materialWeight > 0.0) {
      Material average = Material(material);
      float4 color = colorSum/materialWeight;
      float2 surface = surfaceSum/materialWeight;
      average.baseColor = color.rgb;
      average.metallic = color.a;
      average.roughness = surface.x;
      average.ior = surface.y;
      material = Material(material).blend(average, coverage).pack();
      changed = true;
    }
    if (changed) {
      radianceImage[pixel] = float4(0.0);
    }
  }
  strokeDistanceImage[pixel] = t;
  strokeNormalImage[pixel] = normalBits;
  strokeMaterialColorImage[pixel] = material.x;
  strokeMaterialSurfaceImage[pixel] = material.y;
}

// Weighted least squares fit of v = a + b x + c y to samples around a centre
// at (0, 0). The slopes are regularised towards 0, so a fit to samples along a
// line, such as a strip one pixel wide, falls back to their average.
struct PlaneFit
{
  float weight;
  float2 sumX;   // sum of w x and w y
  float3 sumXX;  // sum of w x x, w x y, w y y
  float3 sumV;   // sum of w v, w x v, w y v

  __init()
  {
    weight = 0.0;
    sumX = float2(0.0);
    sumXX = float3(0.0);
    sumV = float3(0.0);
  }

  [mutating]
  void add(float2 x, float v, float w)
  {
    weight += w;
    sumX += w*x;
    sumXX += w*float3(x.x*x.x, x.x*x.y, x.y*x.y);
    sumV += w*float3(v, v*x.x, v*x.y);
  }

  float valueAtCenter()
  {
    float ridge = 1.0e-3*weight;
    float3x3 m = float3x3(weight, sumX.x, sumX.y,
                          sumX.x, sumXX.x + ridge, sumXX.y,
                          sumX.y, sumXX.y, sumXX.z + ridge);
    float d = determinant(m);
    if (abs(d) < 1.0e-12*weight*weight*weight) {
      return sumV.x/weight;
    }
    // Cramer's rule for a alone
    float3x3 ma = m;
    ma[0][0] = sumV.x;
    ma[1][0] = sumV.y;
    ma[2][0] = sumV.z;
    return determinant(ma)/d;
  }
};

// Blend dabs paint no material, so the material constant carries this instead,
// see blendMain
float blendEdgeThreshold()
{
  return asfloat(canvas.dabMaterialColor);
}

uint2 dabMaterial()
{
  return uint2(canvas.dabMaterialColor, canvas.dabMaterialSurface);
}

// Coverage of a dab at a distance from its centre in units of its radius: the
// opacity inside, fading to 0 at the edge over the falloff fraction of the radius
float dabCoverage(float r)
{
  float falloff = canvas.dabFalloff;
  float shape = falloff > 1.0e-4 ? 1.0 - smoothstep(1.0 - falloff, 1.0, r) : 1.0;
  return canvas.dabOpacity*shape;
}

// Fixed for each pixel and uniform in [0, 1)
float ditherThreshold(int2 pixel)
{
  return float(pcgHash(uint(pixel.x) ^ pcgHash(uint(pixel.y))) >> 8)*(1.0/16777216.0);
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

// Shadow casters and emitters that contain p, as bits for the first 64
// primitives. A painted point needn't lie on the construction geometry it came
// from: blending a convex edge round moves points inside the shape, and a
// caster would then shadow them completely. Like the canvas camera inside a
// room, which sees its walls, a point inside a caster isn't shadowed by it.
uint2 containingPrimitives(float3 p)
{
  uint2 mask = uint2(0);
  for (uint i = 0; i < min(canvas.primitiveCount, 64u); i++) {
    if ((primitives[i].roles & LIGHT_ROLES) != 0 && primitiveDistance(primitives[i], p) < 0.0) {
      mask[i/32] |= 1u << (i%32);
    }
  }
  return mask;
}

bool isContaining(uint2 mask, uint i)
{
  return i < 64 && (mask[i/32] & (1u << (i%32))) != 0;
}

// What lights a pixel: the scene's emitters and its local lights
struct Lighting
{
  uint emitterCount;    // scene emitters that can be sampled
  uint localMask;       // local lights, as bits of their slots
  uint localCount;
  uint occluderMask;    // occluders, as bits of their slots
  uint2 containing;     // see containingPrimitives

  uint count() { return emitterCount + localCount; }
};

// Emitted radiance arriving at origin from direction w. From the scene, the
// emission of the first shadow caster or emitter along the ray, if that is an
// emitter, leaving out those that contain the origin. An emitter that is also a
// shadow caster still emits. Rays that run out of steps grazing along a surface
// bring no light from the scene. Then the local lights whose disk w points
// into, if the ray leaves the scene or they aren't shadowed. All of it is
// dimmed by the occluders w points into.
float3 incidentRadiance(float3 origin, float3 w, Lighting lighting)
{
  float visibility = occluderVisibility(w, lighting.occluderMask);
  if (visibility <= 0.0) {
    return float3(0.0);
  }
  float3 radiance = float3(0.0);
  bool escaped = false;
  float3 sceneRadiance = traceEmission(origin, w, lighting.containing, escaped);
  for (uint mask = lighting.localMask; mask != 0; mask &= mask - 1) {
    LocalLight light = localLights[firstbitlow(mask)];
    if (dot(w, light.direction.xyz) >= light.direction.w && (escaped || light.shadowed == 0)) {
      radiance += light.radiance.rgb;
    }
  }
  return (radiance + sceneRadiance)*visibility;
}

// How much light from direction w the occluders let through: each blocks its
// strength within its inner radius, fading out towards its outer one
float occluderVisibility(float3 w, uint mask)
{
  float visibility = 1.0;
  for (; mask != 0; mask &= mask - 1) {
    Occluder occluder = occluders[firstbitlow(mask)];
    float cosine = dot(w, occluder.direction.xyz);
    float inside = occluder.shape.x > occluder.direction.w ?
                   smoothstep(occluder.direction.w, occluder.shape.x, cosine) : (cosine >= occluder.direction.w ? 1.0 : 0.0);
    visibility *= 1.0 - occluder.shape.y*inside;
  }
  return visibility;
}

float3 traceEmission(float3 origin, float3 w, uint2 containing, out bool escaped)
{
  escaped = false;
  float t = 0.0;
  for (int step = 0; step < MAX_STEPS && t < MAX_DISTANCE; step++) {
    float3 p = origin + w*t;
    float distance = 1.0e30;
    uint closest = ~0u;
    for (uint i = 0; i < canvas.primitiveCount; i++) {
      if ((primitives[i].roles & LIGHT_ROLES) == 0 || isContaining(containing, i)) {
        continue;
      }
      float d = primitiveDistance(primitives[i], p);
      if (d < distance) {
        distance = d;
        closest = i;
      }
    }
    if (closest == ~0u) {
      escaped = true;
      return float3(0.0);
    }
    if (distance < 1.0e-4*max(t, 1.0)) {
      ScenePrimitive primitive = primitives[closest];
      return (primitive.roles & ROLE_LIGHT_EMITTER) != 0 ? primitive.emission.rgb : float3(0.0);
    }
    t += distance;
  }
  escaped = t >= MAX_DISTANCE;
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

  __init(LocalLight light)
  {
    axis = light.direction.xyz;
    cosMax = light.direction.w;
    solidAngle = light.radiance.w;
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

// Density over directions from p of picking an emitter or a local light
// uniformly and then a direction uniformly in its cone
float emitterPdf(float3 p, float3 w, Lighting lighting)
{
  if (lighting.count() == 0) {
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
  for (uint mask = lighting.localMask; mask != 0; mask &= mask - 1) {
    EmitterCone cone = EmitterCone(localLights[firstbitlow(mask)]);
    if (cone.contains(w)) {
      pdf += 1.0/cone.solidAngle;
    }
  }
  return pdf/float(lighting.count());
}

// u.z picks the emitter or local light
float3 sampleEmitters(float3 p, Lighting lighting, float3 u)
{
  uint count = lighting.count();
  uint picked = min(uint(u.z*float(count)), count - 1);
  if (picked >= lighting.emitterCount) {
    uint mask = lighting.localMask;
    for (uint skipped = picked - lighting.emitterCount; skipped > 0; skipped--) {
      mask &= mask - 1;
    }
    return EmitterCone(localLights[firstbitlow(mask)]).sample(u.xy);
  }
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
float3 renderSample(float3 p, float3 origin, Lighting lighting, ShadingFrame frame, float3 wo, Brdf brdf,
                    inout Random random)
{
  float3 radiance = float3(0.0);
  float3 woLocal = frame.toLocal(wo);

  for (int strategy = 0; strategy < 2; strategy++) {
    float3 u = float3(random.next(), random.next(), random.next());
    float3 wi;
    if (strategy == 0) {
      if (lighting.count() == 0) {
        continue;
      }
      wi = sampleEmitters(p, lighting, u);
    }
    else {
      wi = normalize(frame.toWorld(sampleBrdf(brdf, woLocal, u)));
    }
    float3 wiLocal = frame.toLocal(wi);
    if (wiLocal.z <= 0.0) {
      continue;
    }
    float3 f = evaluateBrdf(brdf, woLocal, wiLocal);
    float pdf = emitterPdf(p, wi, lighting) + brdfPdf(brdf, woLocal, wiLocal);
    if (any(f > 0.0) && pdf > 0.0) {
      radiance += f*incidentRadiance(origin, wi, lighting)*(wiLocal.z/pdf);
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
  // the camera anyway. It is turned just far enough to face it rather than
  // flipped, so a normal field that passes edge on, as blended normals on a
  // wall seen at a grazing angle do, stays continuous instead of flipping from
  // pixel to pixel.
  float3 wo = -direction;
  float facing = dot(n, wo);
  if (facing < 0.02) {
    n = normalize(n + wo*(0.02 - facing));
  }
  ShadingFrame frame = ShadingFrame(n);

  Brdf brdf = pixelMaterial(pixel).brdf();

  // Rays leave from just above the surface, so they don't find it again. Above
  // the surface the positions form rather than along the painted normal, which
  // can lie almost along the surface, where rays would start on it and every
  // one of them would be shadowed.
  float3 origin = p + positionNormal(pixel, p, direction)*(1.0e-3*max(t, 1.0));
  Lighting lighting;
  lighting.emitterCount = sampledEmitterCount();
  Rig rig = pixelRig(pixel);
  lighting.localMask = rig.lights & rendering.localLightSlots;
  lighting.localCount = countbits(lighting.localMask);
  lighting.occluderMask = rig.occluders & rendering.occluderSlots;
  lighting.containing = containingPrimitives(origin);
  uint first = uint(accumulated.a);
  uint count = min(rendering.samplesPerFrame, rendering.maxSamples - first);
  float3 sum = float3(0.0);
  for (uint i = 0; i < count; i++) {
    Random random = Random(globalId.xy, first + i);
    sum += renderSample(p, origin, lighting, frame, wo, brdf, random);
  }
  radianceImage[pixel] = accumulated + float4(sum, float(count));
}

// The normal a pixel is rendered with, as shadingNormal, or its painted normal
// where it has no position. False with neither.
bool renderedNormal(int2 pixel, out float3 n)
{
  if (shadingNormal(pixel, n)) {
    return true;
  }
  uint bits = normalImage[pixel];
  n = bits != NO_NORMAL ? decodeNormal(bits) : float3(0.0, 0.0, 1.0);
  return bits != NO_NORMAL;
}

// The normal a pixel is rendered with: the painted one, or the one its
// position and its neighbours' imply, facing the camera. False without a position.
bool shadingNormal(int2 pixel, out float3 n)
{
  n = float3(0.0, 0.0, 1.0);
  float t = distanceImage[pixel];
  if (t <= 0.0) {
    return false;
  }
  float3 direction = pixelRay(uint2(pixel));
  uint bits = normalImage[pixel];
  n = bits != NO_NORMAL ? decodeNormal(bits) : positionNormal(pixel, canvas.origin.xyz + direction*t, direction);
  if (dot(n, direction) > 0.0) {
    n = -n;
  }
  return true;
}

static const uint PICK_GROUP_SIZE = 16;
groupshared float4 pickSums[PICK_GROUP_SIZE*PICK_GROUP_SIZE];

// Sums the normals the pixels within dabRadius of dabCenter are rendered with,
// for picking the normal under the brush. One group strides over the disk and
// adds up its threads' sums.
[shader("compute")]
[numthreads(16, 16, 1)]
void pickMain(uint3 threadId : SV_GroupThreadID)
{
  uint index = threadId.y*PICK_GROUP_SIZE + threadId.x;
  int2 first = int2(floor(canvas.dabCenter - canvas.dabRadius));
  int2 last = int2(ceil(canvas.dabCenter + canvas.dabRadius));
  float4 sum = float4(0.0);
  for (int y = first.y + int(threadId.y); y <= last.y; y += int(PICK_GROUP_SIZE)) {
    for (int x = first.x + int(threadId.x); x <= last.x; x += int(PICK_GROUP_SIZE)) {
      int2 pixel = int2(x, y);
      if (any(pixel < 0) || any(pixel >= int2(canvas.size))) {
        continue;
      }
      float2 delta = float2(pixel) + 0.5 - canvas.dabCenter;
      float3 n;
      if (dot(delta, delta) <= canvas.dabRadius*canvas.dabRadius && shadingNormal(pixel, n)) {
        sum += float4(n, 1.0);
      }
    }
  }
  pickSums[index] = sum;
  GroupMemoryBarrierWithGroupSync();
  for (uint stride = PICK_GROUP_SIZE*PICK_GROUP_SIZE/2; stride > 0; stride /= 2) {
    if (index < stride) {
      pickSums[index] += pickSums[index + stride];
    }
    GroupMemoryBarrierWithGroupSync();
  }
  if (index == 0) {
    pickResult[0] = pickSums[0];
    // And the rig of the pixel under the centre, to select it
    int2 center = clamp(int2(floor(canvas.dabCenter)), int2(0), int2(canvas.size) - 1);
    pickResult[1] = float4(asfloat(rigImage[center]), 0.0, 0.0, 0.0);
  }
}

// Counts the painted pixels of each rig into rigCounts, reset to 0 before
[shader("compute")]
[numthreads(8, 8, 1)]
void rigCountMain(uint3 globalId : SV_DispatchThreadID)
{
  if (any(globalId.xy >= canvas.size)) {
    return;
  }
  int2 pixel = int2(globalId.xy);
  if (distanceImage[pixel] > 0.0) {
    InterlockedAdd(rigCounts[min(rigImage[pixel], MAX_RIGS - 1)], 1u);
  }
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
// the base colour of each pixel, also where there is no position. Rigs shows
// the colour of the light rig painted on each pixel. Matcap shades the
// normals each pixel is rendered with as red wax, see redWax. Rendering shows
// the mean of the rendered samples, exposed and sRGB encoded. Isolines are drawn
// over any of them but the rendering and the matcap where there is a position. Pixels without
// the shown attribute are black.
// A world space normal in the canvas camera's view space: x right, y up and z
// towards the viewer, along the view axis, without the lens shift
float3 toViewSpace(float3 n)
{
  float3 right = normalize(canvas.right.xyz);
  float3 up = normalize(canvas.up.xyz);
  float3 back = normalize(cross(right, up));
  return float3(dot(n, right), dot(n, up), dot(n, back));
}

// Shades a view space normal like ZBrush's default material, MatCap Red Wax,
// for judging the form of a surface without lights or materials: a key light
// close to the viewer, up and to the left, takes a saturated red from a deep
// red shadow to a warmer lit red, under a broad waxy sheen with a softer core
// and a warm rim at grazing angles. In display values, like a matcap image.
float3 redWax(float3 n)
{
  float3 key = normalize(float3(-0.35, 0.45, 0.82));
  float d = saturate((dot(n, key) + 0.15)/1.15);
  float3 color = lerp(float3(0.16, 0.022, 0.016), float3(0.62, 0.08, 0.05), smoothstep(0.0, 0.55, d));
  color = lerp(color, float3(0.90, 0.24, 0.14), smoothstep(0.45, 1.0, d));
  float specular = saturate(dot(n, normalize(key + float3(0.0, 0.0, 1.0))));
  color += float3(1.0, 0.86, 0.74)*(0.38*pow(specular, 10.0) + 0.22*pow(specular, 60.0));
  color += float3(0.60, 0.18, 0.10)*0.7*pow(saturate(1.0 - n.z), 4.0);
  return saturate(color);
}

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
  if (canvas.display == DISPLAY_MATCAP) {
    float3 n;
    if (shadingNormal(pixel, n)) {
      color = redWax(toViewSpace(n));
    }
    displayImage[globalId.xy] = float4(color, 1.0);
    return;
  }
  if (canvas.display == DISPLAY_MATERIAL) {
    color = pixelMaterial(pixel).baseColor;
  }
  else if (canvas.display == DISPLAY_RIGS) {
    // Dark grey where a position has no rig
    uint index = rigImage[pixel];
    if (index != 0) {
      uint packed = pixelRig(pixel).color;
      color = float3(packed & 0xFFu, (packed >> 8) & 0xFFu, (packed >> 16) & 0xFFu)/255.0;
    }
    else if (t > 0.0) {
      color = float3(0.12);
    }
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
