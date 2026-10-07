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
};

//-------------------------------------------------------------------------------
// @ LsCanvas
//-------------------------------------------------------------------------------
// The canvas attributes and the brush that paints them. Each pixel stores its
// position as the distance from the canvas camera centre along the ray through
// the pixel (see CANVAS in SPECIFICATION.md), so a point can never leave its
// pixel. The world position is reconstructed with the current canvas camera.
//
// A stroke transfers positions from the construction geometry: every dab
// sphere traces the scene from the canvas camera through the pixels it covers.
// The image the renderer draws as the canvas is then resolved from the
// attributes, for now showing the positions as depth or as world coordinates,
// optionally with isolines of the depth over them.
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
  VkShaderModule resolveShader = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline paintPipeline = VK_NULL_HANDLE;
  VkPipeline rangePipeline = VK_NULL_HANDLE;
  VkPipeline resolvePipeline = VK_NULL_HANDLE;

  // Distance along each pixel's ray, 0 without a position
  struct {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    bool cleared = false;
  } distance;

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
  };
  std::vector<Dab> dabs;

  uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
  VkPipeline CreatePipeline(VkShaderModule shader);
  void CreateDistanceImage();
  void CreateRangeBuffer();
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
  // Queues a dab for each centre in order, positions and radius in canvas pixels
  void Paint(const std::vector<glm::vec2>& centers, float radius);
  // Records the queued dabs and, when anything changed, the canvas image into
  // the current frame. Call between BeginFrame and EndFrame, before the canvas
  // is drawn.
  void Render();
};
