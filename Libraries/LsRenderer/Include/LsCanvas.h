#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
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
  Rigs = 5,      // colour of the light rig painted on each pixel
  Matcap = 6,    // the shading normals in red wax, like ZBrush's default MatCap Red Wax
};

// A light the artist places around the painting rather than in the scene, for
// lighting a region the way a painter would. It lights from a direction at
// infinity, a small disk of sky, so it gives soft shadows and highlights. The
// direction is in the canvas camera's view space, x right, y up and z towards
// the viewer, so it stays where it is relative to the painting as the camera
// moves, like the painted positions do.
struct LsLocalLight
{
  std::string name;
  glm::vec3 direction = glm::normalize(glm::vec3(-0.5f, 0.6f, 0.6f)); // towards the light
  glm::vec3 color = glm::vec3(1.0f); // linear
  // Irradiance on a surface facing the light, times the colour
  float intensity = 3.0f;
  float size = 3.0f; // angular radius in degrees
  // Blocked by the scene's shadow casters
  bool shadows = true;
};

// Local lights are kept in this many slots, a pixel has a bit for each
const uint32_t kLsMaxLocalLights = 32;

// Shadows a pixel from a cap of directions, as if something the scene doesn't
// have were in the way there, painted with the light rig like a local light.
// The direction is in the canvas camera's view space, like a local light's.
struct LsOccluder
{
  std::string name;
  glm::vec3 direction = glm::normalize(glm::vec3(0.6f, 0.4f, 0.5f)); // towards its centre
  float size = 30.0f;    // angular radius in degrees
  float softness = 0.3f; // fraction of the radius over which it fades out towards its edge
  float strength = 1.0f; // how much of the light within it is blocked
};

// Occluders are kept in this many slots, a rig has a bit for each
const uint32_t kLsMaxOccluders = 32;

// A light rig: the local lights that light the pixels it is painted on, on top
// of the scene's emitters, and the occluders that shadow them. Pixels refer to
// their rig by index, so changing a rig changes every pixel it is painted on.
struct LsLightRig
{
  std::string name;
  uint32_t lights = 0;    // bits of local light slots
  uint32_t occluders = 0; // bits of occluder slots
  glm::vec3 color = glm::vec3(1.0f); // sRGB, for showing where it is painted
};

// Rigs are kept in this many slots, slot 0 is no rig: only the scene's emitters
const uint32_t kLsMaxRigs = 256;

// The painted attributes of every pixel as stored on the GPU, row by row, for
// saving: distance along the ray, octahedral normal, the two packed halves of
// the BRDF and the light rig index, see canvas.slang
struct LsCanvasAttributes
{
  uint32_t width = 0, height = 0;
  std::vector<float> distance;
  std::vector<uint32_t> normal;
  std::vector<uint32_t> materialColor;
  std::vector<uint32_t> materialSurface;
  std::vector<uint32_t> rig;
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
  Light = 4,    // a light rig, see LsCanvasBrush::rig
  Sculpt = 5,   // positions pulled towards the camera, or pushed away, see LsCanvasBrush::push
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
  // Normal strokes paint this view space normal instead of the construction
  // geometry's when set, and light strokes paint it with the rig, see
  // LsLocalLight for the view space
  bool fixedNormal = false;
  glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
  // The slot of the light rig a light stroke paints, 0 for none. With clearRig
  // the pixels it covers that have this rig go back to none instead.
  uint32_t rig = 0;
  bool clearRig = false;
  // Sculpt strokes push positions away from the camera instead of pulling them
  // towards it. Their dabs build up like blending, by the opacity and falloff.
  bool push = false;
  // Sculpt strokes move the surface along its normal rather than along the
  // camera rays, as far as a pixel's position can, see canvas.slang
  bool alongNormal = false;
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
// Every pixel is lit by the scene's emitters, and by the local lights of the
// light rig painted on it, shadowed by the rig's occluders: each pixel has the
// index of its rig, which a light stroke sets, and the rigs, lights and
// occluders are kept in buffers, so changing any of them changes every pixel
// they reach. Soft edges of light strokes are dithered like erasing. After any
// stroke the painted pixels of each rig are counted, see GetRigPixelCount.
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
  VkShaderModule pickShader = VK_NULL_HANDLE;
  VkShaderModule rigCountShader = VK_NULL_HANDLE;
  VkShaderModule rangeShader = VK_NULL_HANDLE;
  VkShaderModule renderShader = VK_NULL_HANDLE;
  VkShaderModule resolveShader = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline paintPipeline = VK_NULL_HANDLE;
  VkPipeline blendPipeline = VK_NULL_HANDLE;
  VkPipeline pickPipeline = VK_NULL_HANDLE;
  VkPipeline rigCountPipeline = VK_NULL_HANDLE;
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
  // Index of the light rig painted on each pixel
  AttributeImage rigImage;
  // Slots of rigs, those in use are set in rigUsed, slot 0 is always no rig
  std::array<LsLightRig, kLsMaxRigs> rigs;
  std::array<bool, kLsMaxRigs> rigUsed = {};
  // Mirrors Rig in canvas.slang, rewritten every Render
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
  } rigBuffer;
  // Painted pixels of each rig, counted on the GPU after strokes and read back
  // once that frame has finished
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
  } rigCountBuffer;
  std::array<uint32_t, kLsMaxRigs> rigPixelCounts = {};
  uint32_t paintedPixelCount = 0;
  bool rigCountRequested = true;
  bool rigCountRecorded = false;
  uint64_t rigCountFrame = 0;
  void CollectRigCounts();
  std::array<LsOccluder, kLsMaxOccluders> occluders;
  uint32_t occluderSlots = 0;
  // Mirrors Occluder in canvas.slang, in world space for the camera, rewritten every Render
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
  } occluderBuffer;
  // Slots of local lights, those in use have their bit set in localLightSlots
  std::array<LsLocalLight, kLsMaxLocalLights> localLights;
  uint32_t localLightSlots = 0;
  // Mirrors LocalLight in canvas.slang, in world space for the camera, rewritten every Render
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
  } lightBuffer;
  // Picking the normal under the brush: requested, recorded into a frame, then
  // read back once that frame has finished
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
  } pickBuffer;
  bool pickRequested = false;
  glm::vec2 pickCenter = glm::vec2(0.0f);
  float pickRadius = 1.0f;
  bool pickRecorded = false;
  uint64_t pickFrame = 0;
  // Read back from the pick buffer, taken by TakePickedNormal and TakePickedRig
  bool pickedNormalReady = false;
  glm::vec3 pickedNormal = glm::vec3(0.0f, 0.0f, 1.0f);
  bool pickedRigReady = false;
  uint32_t pickedRig = 0;
  void CollectPick();
  // Reading the attributes back: requested, copied into the staging buffer in
  // a frame, then read once that frame has finished
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
  } readbackBuffer;
  bool readbackRequested = false;
  bool readbackRecorded = false;
  uint64_t readbackFrame = 0;
  void RecordReadback(VkCommandBuffer commandBuffer);
  void DestroyReadbackBuffer();
  // Uploading attributes: filled by SetAttributes, copied to the images in the
  // next Render and freed once that frame has finished
  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
  } uploadBuffer;
  bool uploadPending = false;
  bool uploadRecorded = false;
  uint64_t uploadFrame = 0;
  void RecordUpload(VkCommandBuffer commandBuffer);
  void DestroyUploadBuffer();
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
  LsCanvasDisplay display = LsCanvasDisplay::Rendering;
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
  void CreateHostBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const char* name,
                        VkBuffer& buffer, VkDeviceMemory& memory, void*& mapped);
  // View space of the canvas camera to world space
  glm::vec3 ViewToWorld(glm::vec3 direction) const;
  void UpdateLightBuffer();
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
  // In pixels
  glm::uvec2 GetSize() const { return glm::uvec2(width, height); }
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
  // Returns the slot of the new light, or -1 when every slot is in use
  int AddLocalLight(const LsLocalLight& light);
  // Also takes the light out of every rig
  void RemoveLocalLight(uint32_t slot);
  // Null for slots not in use
  const LsLocalLight* GetLocalLight(uint32_t slot) const;
  void SetLocalLight(uint32_t slot, const LsLocalLight& light);
  // Bits of the slots in use
  uint32_t GetLocalLightSlots() const { return localLightSlots; }
  // Occluders, kept like the local lights
  int AddOccluder(const LsOccluder& occluder);
  void RemoveOccluder(uint32_t slot);
  const LsOccluder* GetOccluder(uint32_t slot) const;
  void SetOccluder(uint32_t slot, const LsOccluder& occluder);
  uint32_t GetOccluderSlots() const { return occluderSlots; }
  // Light rigs: returns the slot of the new rig, or -1 when every slot is in use
  int AddRig(const LsLightRig& rig);
  // Also takes the rig off every pixel it was painted on
  void RemoveRig(uint32_t slot);
  // Null for slot 0 and slots not in use
  const LsLightRig* GetRig(uint32_t slot) const;
  void SetRig(uint32_t slot, const LsLightRig& rig);
  // Painted pixels with this rig, slot 0 for those with none, as counted after
  // the last stroke
  uint32_t GetRigPixelCount(uint32_t slot) const { return slot < kLsMaxRigs ? rigPixelCounts[slot] : 0; }
  // Pixels with a position, as counted after the last stroke
  uint32_t GetPaintedPixelCount() const { return paintedPixelCount; }
  // Picks the normal the canvas is rendered with, averaged over the pixels with
  // a position within radius of center, in canvas pixels. Only the latest
  // request is picked.
  void RequestNormalPick(glm::vec2 center, float radius);
  // The picked normal in the canvas camera's view space, see LsLocalLight, true
  // once when a pick has finished and found any normal
  bool TakePickedNormal(glm::vec3& normal);
  // The rig of the pixel under the centre of the latest pick, 0 for none, true once when it has finished
  bool TakePickedRig(uint32_t& rig);
  // Copies every pixel's attributes from the GPU, as the next Render has them
  void RequestAttributes();
  // True once the requested attributes have been copied, a frame after the request
  bool TakeAttributes(LsCanvasAttributes& attributes);
  // Replaces every pixel's attributes, as the next Render. The size must be the canvas's.
  void SetAttributes(const LsCanvasAttributes& attributes);
  // Removes every light, occluder and rig, without touching the pixels, for
  // putting back those of a document in their slots with the Set...At calls
  void ClearLighting();
  void SetLocalLightAt(uint32_t slot, const LsLocalLight& light);
  void SetOccluderAt(uint32_t slot, const LsOccluder& occluder);
  void SetRigAt(uint32_t slot, const LsLightRig& rig);
  void SetPaintedBrdfs(const std::vector<LsBrdfParameters>& brdfs) { paintedBrdfs = brdfs; }
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
