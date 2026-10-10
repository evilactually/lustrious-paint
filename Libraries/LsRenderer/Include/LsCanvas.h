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
  Erase = 2,  // back to no position, no normal and the first material
  Material = 3, // the brush BRDF, see LsCanvas::SetBrushBrdf
};

//-------------------------------------------------------------------------------
// @ LsCanvas
//-------------------------------------------------------------------------------
// The canvas attributes and the brush that paints them. Each pixel stores its
// position as the distance from the canvas camera centre along the ray through
// the pixel (see CANVAS in SPECIFICATION.md), so a point can never leave its
// pixel. The world position is reconstructed with the current canvas camera.
// Each pixel also stores a world space surface normal, painted separately from
// the position, so the two can disagree, and the index of its BRDF in the
// material table.
//
// The material table holds every distinct BRDF painted so far. A material
// stroke paints the brush BRDF, which is looked up in the table and added to it
// when no material has exactly its parameters, and the pixels it covers store
// that index. Pixels start with the first material, the brush BRDF the canvas
// was created with. Materials are only ever added, so indices stay valid.
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
  VkShaderModule rangeShader = VK_NULL_HANDLE;
  VkShaderModule renderShader = VK_NULL_HANDLE;
  VkShaderModule resolveShader = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline paintPipeline = VK_NULL_HANDLE;
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

  // Index into materials of each pixel
  AttributeImage material;
  // Distinct BRDFs painted so far, the first is what unpainted pixels have
  std::vector<LsBrdfParameters> materials;
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    size_t capacity = 0; // in materials
  } materialBuffer;
  bool materialsDirty = true;
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
    float radius;     // in canvas pixels
    LsCanvasDabMode mode;
    uint32_t material; // index painted by a material dab
  };
  std::vector<Dab> dabs;

  uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
  VkPipeline CreatePipeline(VkShaderModule shader);
  AttributeImage CreateAttributeImage(VkFormat format, const char* name);
  void DestroyAttributeImage(AttributeImage& attribute);
  void CreateRangeBuffer();
  void CreateParameterBuffer();
  void CreateMaterialBuffer(size_t capacity);
  void DestroyMaterialBuffer();
  // Index of the material with exactly these parameters, added when there is none
  uint32_t FindOrAddMaterial(const LsBrdfParameters& brdf);
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
  const std::vector<LsBrdfParameters>& GetMaterials() const { return materials; }
  const LsCanvasRenderSettings& GetRenderSettings() const { return renderSettings; }
  void SetRenderSettings(const LsCanvasRenderSettings& settings);
  // Samples per pixel so far while showing the rendering
  uint32_t GetRenderedSamples() const { return renderedSamples; }
  // True while the rendering is shown and has samples left to render, the
  // application keeps drawing frames until it is done
  bool IsRendering() const;
  // Queues a dab for each centre in order, positions and radius in canvas pixels
  void Paint(const std::vector<glm::vec2>& centers, float radius, LsCanvasDabMode mode = LsCanvasDabMode::Depth);
  // Records the queued dabs and, when anything changed, the canvas image into
  // the current frame. Call between BeginFrame and EndFrame, before the canvas
  // is drawn.
  void Render();
};
