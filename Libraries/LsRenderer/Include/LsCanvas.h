#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan_dynamic.hpp>
#include <glm/glm.hpp>
#include <LsCamera.h>
#include <LsScene.h>

class LsRenderer;

// What the canvas shows of its attributes. Values are shared with Shaders/canvas.slang.
enum class LsCanvasDisplay : uint32_t
{
  Depth = 0,    // distance from the camera in grey, near is bright
  Position = 1, // absolute world coordinates in RGB
  Normal = 2,   // world space normal in RGB, mapped from [-1, 1] to [0, 1]
  Rendering = 3, // the light reflected towards the camera, see LsCanvas
  Material = 4,  // base colour of each pixel's material
};

// Reflectance of a painted point, the metallic-roughness model of glTF 2.0 with
// pbrt-v4's Trowbridge-Reitz microfacets, see Shaders/brdf.slang
struct LsBrdfParameters
{
  glm::vec3 baseColor = glm::vec3(0.8f); // linear
  float metallic = 0.0f;
  float roughness = 0.5f; // perceptual, squared for the microfacet alpha
  float ior = 1.5f;       // of the dielectric coat over the diffuse base

  bool operator==(const LsBrdfParameters& other) const {
    return baseColor == other.baseColor && metallic == other.metallic &&
           roughness == other.roughness && ior == other.ior;
  }
  bool operator!=(const LsBrdfParameters& other) const { return !(*this == other); }
};

struct LsCanvasRenderSettings
{
  float exposure = 0.0f; // in stops
  uint32_t samplesPerFrame = 4;
  uint32_t maxSamples = 512; // per pixel, rendering stops there
};

// What a dab does to the pixels it covers. Values are shared with Shaders/canvas.slang.
enum class LsCanvasDabMode : uint32_t
{
  Depth = 0,  // positions from the construction geometry
  Normal = 1, // normals from the construction geometry
  Erase = 2,  // back to no position, no normal and the default material
  Material = 3, // the brush BRDF, see LsCanvas::SetBrushBrdf
};

// How a stroke paints
struct LsCanvasBrush
{
  float radius = 10.0f; // in canvas pixels
  // How much the stroke covers a pixel at most, overlapping dabs don't add up past it
  float opacity = 1.0f;
  // Fraction of the radius over which a dab fades out towards its edge, 0 for a hard edge
  float falloff = 0.5f;
  LsCanvasDabMode mode = LsCanvasDabMode::Depth;
  // Smooths what the mode paints instead of painting it, everything for Erase
  bool blend = false;
  // A blend leaves out neighbours whose view depth differs from the pixel's by
  // more than this fraction of it, so the edge of an object in front isn't
  // blended with what is behind it. 0 blends across everything.
  float blendEdgeThreshold = 0.1f;
};

//-------------------------------------------------------------------------------
// @ LsCanvas
//-------------------------------------------------------------------------------
// The canvas attributes and the brush that paints them. Each pixel stores its
// position as the distance from the canvas camera centre along the ray through
// the pixel (see CANVAS in SPECIFICATION.md), so a point can never leave its
// pixel. The world position is reconstructed with the current canvas camera.
// Each pixel also stores a world space surface normal, painted separately from
// the position, so the two can disagree, and the parameters of its BRDF, which
// start as the default LsBrdfParameters. A material stroke paints the brush BRDF.
//
// Strokes blend: each pixel is covered by a stroke as much as the strongest of
// its dabs covered it, the brush opacity shaped by the falloff, and the pixel
// becomes the attributes it had when the stroke started blended towards the
// dab's by that coverage. Positions blend along the ray, normals are normalised
// after blending and materials blend their parameters, the base colour in sRGB
// as colours mix in a picker. A pixel without a position or normal takes the
// new one outright, and erasing dithers since a position is there or not.
//
// A blend stroke smooths instead, the attribute its mode paints or all of them
// for erasing: each dab blends the pixels it covers towards the average of their
// neighbourhood by its coverage, and its dabs compound so that going over an
// area again smooths it further. Positions are averaged as inverse view depth,
// which keeps planes flat. Neighbours on another surface, too far in front or
// behind, are left out of the average of every attribute.
//
// A depth stroke transfers positions and a normal stroke normals from the
// construction geometry: every dab sphere traces the scene from the canvas
// camera through the pixels it covers. The image the renderer draws as the
// canvas is then resolved from the attributes, for now showing the positions
// as depth or as world coordinates, the normals or the materials' base colours,
// optionally with isolines of the depth over them, or rendered.
//
// Rendering solves the rendering equation at every painted point for the light
// that reaches it straight from the scene's emitters, past its shadow casters,
// and is reflected towards the camera by the pixel's BRDF. There are no further
// bounces. Samples accumulate over frames until each pixel has maxSamples of
// them. A dab starts the pixels it changes over, and a change to the camera or
// the scene starts every pixel over.
//
// Dabs are queued and recorded into the frame by Render, with the scene and
// camera as they are then.
//-------------------------------------------------------------------------------
class LsCanvas
{
  std::shared_ptr<LsRenderer> renderer;
  VkDevice device = VK_NULL_HANDLE;
  uint32_t width = 0, height = 0;

  VkShaderModule paintShader = VK_NULL_HANDLE;
  VkShaderModule blendShader = VK_NULL_HANDLE;
  VkShaderModule rangeShader = VK_NULL_HANDLE;
  VkShaderModule renderShader = VK_NULL_HANDLE;
  VkShaderModule resolveShader = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline paintPipeline = VK_NULL_HANDLE;
  VkPipeline blendPipeline = VK_NULL_HANDLE;
  VkPipeline rangePipeline = VK_NULL_HANDLE;
  VkPipeline renderPipeline = VK_NULL_HANDLE;
  VkPipeline resolvePipeline = VK_NULL_HANDLE;

  struct AttributeImage {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
  };
  // Distance along each pixel's ray, 0 without a position
  AttributeImage distance;
  // World space normal, octahedral encoded as two 16 bit snorms, see canvas.slang
  AttributeImage normal;
  // Sum of the rendered samples in RGB and their count in alpha
  AttributeImage radiance;
  // The attribute images and the radiance start without anything painted
  bool attributesCleared = false;
  // Every pixel's samples are out of date and are cleared before rendering more
  bool radianceStale = true;
  // Samples per pixel rendered since everything or the last dab started over,
  // pixels a dab started over have at most this many
  uint32_t renderedSamples = 0;

  // BRDF parameters of each pixel in two packed halves, see Material in canvas.slang
  AttributeImage materialColor;
  AttributeImage materialSurface;
  // The attributes when the current stroke started, and how much it covers each
  // pixel. Blend strokes write each dab's result to them before copying it back.
  AttributeImage strokeDistance;
  AttributeImage strokeNormal;
  AttributeImage strokeMaterialColor;
  AttributeImage strokeMaterialSurface;
  AttributeImage strokeCoverage;
  // Distinct BRDFs painted so far, for picking them again
  std::vector<LsBrdfParameters> paintedBrdfs;
  LsBrdfParameters brushBrdf;

  LsCanvasRenderSettings renderSettings;
  // Mirrors RenderParameters in canvas.slang, rewritten every Render
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
  } parameterBuffer;

  // Nearest and furthest painted distance, for showing depth
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
  } rangeBuffer;

  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    size_t capacity = 0; // in primitives
  } primitiveBuffer;

  std::vector<LsScenePrimitive> primitives;
  bool primitivesDirty = true;

  bool hasCamera = false;
  LsCamera camera;
  // Origin and ray basis of the camera the canvas image was last resolved with
  glm::vec4 resolvedFrame[4] = {};
  bool resolveNeeded = true;
  LsCanvasDisplay display = LsCanvasDisplay::Depth;
  bool isolines = true;
  float isolineSpacing = 0.1f; // in world units of view depth

  struct Dab {
    glm::vec2 center; // in canvas pixels
    LsCanvasBrush brush;
    LsBrdfParameters brdf; // painted by a material dab
    bool startsStroke;
  };
  std::vector<Dab> dabs;

  uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
  VkPipeline CreatePipeline(VkShaderModule shader);
  AttributeImage CreateAttributeImage(VkFormat format, const char* name);
  void DestroyAttributeImage(AttributeImage& attribute);
  void CreateRangeBuffer();
  void CreateParameterBuffer();
  // Copies the attributes for the stroke to blend from and clears its coverage
  void RecordStrokeStart(VkCommandBuffer commandBuffer);
  // Copies a blend dab's result from the stroke images back to the attributes
  void RecordBlendCopy(VkCommandBuffer commandBuffer, glm::ivec2 first, glm::ivec2 last);
  void CreatePrimitiveBuffer(size_t capacity);
  void DestroyPrimitiveBuffer();
  void UpdateDescriptors();
public:
  explicit LsCanvas(std::shared_ptr<LsRenderer> renderer);
  ~LsCanvas();
  LsCanvas(const LsCanvas&) = delete;
  LsCanvas& operator=(const LsCanvas&) = delete;

  // Copied, uploaded by the next Render. The construction geometry in them is
  // what strokes transfer positions from.
  void SetPrimitives(const std::vector<LsScenePrimitive>& primitives);
  // The camera the canvas is painted from, with the canvas aspect
  void SetCamera(const LsCamera& camera);
  // Without a canvas camera strokes paint nothing
  void ClearCamera() { hasCamera = false; }
  LsCanvasDisplay GetDisplay() const { return display; }
  void SetDisplay(LsCanvasDisplay display);
  // Contour lines where the view depth crosses a multiple of the spacing
  bool GetIsolines() const { return isolines; }
  void SetIsolines(bool shown);
  float GetIsolineSpacing() const { return isolineSpacing; }
  void SetIsolineSpacing(float spacing);
  // What material strokes paint. Changing it changes no pixel until it is painted.
  const LsBrdfParameters& GetBrushBrdf() const { return brushBrdf; }
  void SetBrushBrdf(const LsBrdfParameters& brdf) { brushBrdf = brdf; }
  // Every distinct BRDF painted so far, in the order they were first painted
  const std::vector<LsBrdfParameters>& GetPaintedBrdfs() const { return paintedBrdfs; }
  const LsCanvasRenderSettings& GetRenderSettings() const { return renderSettings; }
  void SetRenderSettings(const LsCanvasRenderSettings& settings);
  // Samples per pixel so far while showing the rendering
  uint32_t GetRenderedSamples() const { return renderedSamples; }
  // True while the rendering is shown and has samples left to render, the
  // application keeps drawing frames until it is done
  bool IsRendering() const;
  // Queues a dab for each centre in order, in canvas pixels. The first one starts
  // a new stroke when startsStroke is set, the others continue the stroke.
  void Paint(const std::vector<glm::vec2>& centers, const LsCanvasBrush& brush, bool startsStroke);
  // Records the queued dabs and, when anything changed, the canvas image into
  // the current frame. Call between BeginFrame and EndFrame, before the canvas
  // is drawn.
  void Render();
};
