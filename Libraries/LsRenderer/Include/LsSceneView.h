#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan_dynamic.hpp>
#include <imgui.h>
#include <LsCamera.h>
#include <LsScene.h>

class LsRenderer;

// Drawing flags the editor sets in LsScenePrimitive::roles, above the real roles.
// Values are shared with Shaders/viewport.slang.
const uint32_t kLsViewSelected   = 1u << 31;
const uint32_t kLsViewHovered    = 1u << 30;
// For shapes without any role, which would otherwise be invisible and unclickable
const uint32_t kLsViewUnassigned = 1u << 29;

// Drawn over the traced scene and hidden where the scene is in front of it
struct LsSceneLine
{
  glm::vec3 a = glm::vec3(0.0f);
  glm::vec3 b = glm::vec3(0.0f);
  glm::vec4 color = glm::vec4(1.0f); // display values with straight alpha
  float width = 1.0f;                // in pixels
  float fadeDistance = 0.0f;         // fades out towards this view depth, 0 never fades
};

struct LsSceneViewSettings
{
  // Light the scene with its emitters and shadow casters instead of studio lights
  bool lit = false;
  // Ground grid in the Y = 0 plane with the X and Z axes
  bool grid = true;
  // In stops, used when lit
  float exposure = 0.0f;
  // Outline every shape the way the selection is outlined: silhouettes against
  // whatever is behind them, not the inner edges of a shape
  bool outlines = false;
  // Only the outlines on a transparent image, without shading, grid or media
  bool outlinesOnly = false;
};

// Handle to a ray queued with LsSceneView::QueueRaycast, 0 is never one
using LsRaycastTicket = uint64_t;

enum class LsRaycastStatus
{
  Pending, // not traced yet, or traced in the frame still being recorded
  Hit,
  Miss,
  Unknown, // never queued, or its result was dropped
};

//-------------------------------------------------------------------------------
// @ LsSceneView
//-------------------------------------------------------------------------------
// Renders the scene primitives from a camera into an image the UI can show with
// ImGui::Image. A compute shader sphere traces the signed distance functions
// directly, there is no mesh conversion, and also writes each pixel's depth.
// Lines such as the grid are then rasterised on top, tested against that depth,
// which keeps them sharp where tracing them would blur them.
//-------------------------------------------------------------------------------
class LsSceneView
{
  std::shared_ptr<LsRenderer> renderer;
  VkDevice device = VK_NULL_HANDLE;

  VkShaderModule shader = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;

  // Lines over the traced image
  VkShaderModule lineVertexShader = VK_NULL_HANDLE;
  VkShaderModule lineFragmentShader = VK_NULL_HANDLE;
  VkDescriptorSetLayout lineDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet lineDescriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout linePipelineLayout = VK_NULL_HANDLE;
  VkPipeline linePipeline = VK_NULL_HANDLE;
  VkRenderPass lineRenderPass = VK_NULL_HANDLE;
  std::vector<LsSceneLine> lines;

  struct {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDescriptorSet texture = VK_NULL_HANDLE; // ImGui's descriptor for sampling it
    // View depth of the traced surfaces, for hiding lines behind them
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    uint32_t width = 0, height = 0;
  } target;

  struct {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    size_t capacity = 0; // in primitives
  } primitiveBuffer;

  std::vector<LsScenePrimitive> primitives;
  bool primitivesDirty = true;

  // Ray queries, traced on the GPU against the same primitives as the image
  VkShaderModule pickShader = VK_NULL_HANDLE;
  VkDescriptorSetLayout pickDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet pickDescriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout pickPipelineLayout = VK_NULL_HANDLE;
  VkPipeline pickPipeline = VK_NULL_HANDLE;
  // Rays in, hits out, both in host memory and small enough for a frame's queries
  struct HostBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
  };
  HostBuffer rayBuffer;
  HostBuffer hitBuffer;
  struct QueuedRay {
    LsRaycastTicket ticket;
    LsRay ray;
    LsSceneRoles roleMask;
    float maxDistance;
  };
  // Waiting for the next Render
  std::vector<QueuedRay> queuedRays;
  // Recorded by a Render, their hits are in hitBuffer once a later frame begins
  std::vector<LsRaycastTicket> tracedTickets;
  uint64_t tracedFrame = 0;
  // Read back from hitBuffer, kept until the next rays are traced
  struct Completed {
    bool hit;
    LsSceneHit sceneHit;
  };
  std::unordered_map<LsRaycastTicket, Completed> completed;
  LsRaycastTicket nextTicket = 1;

  void CreatePickPipeline();
  void CreateHostBuffer(VkDeviceSize size, HostBuffer& buffer);
  void DestroyHostBuffer(HostBuffer& buffer);
  // Moves the hits of the traced rays into the completed results once the GPU is done
  void CollectRaycasts();
  void TraceQueuedRays(VkCommandBuffer commandBuffer);

  uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
  void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                   VkImage& image, VkDeviceMemory& memory, VkImageView& view);
  void CreateLinePipeline();
  void AppendGrid(const LsCamera& camera);
  void DrawLines(VkCommandBuffer commandBuffer, const LsCamera& camera);
  void CreateTarget(uint32_t width, uint32_t height);
  void DestroyTarget();
  void CreatePrimitiveBuffer(size_t capacity);
  void DestroyPrimitiveBuffer();
  void UpdateDescriptors();
public:
  explicit LsSceneView(std::shared_ptr<LsRenderer> renderer);
  ~LsSceneView();
  LsSceneView(const LsSceneView&) = delete;
  LsSceneView& operator=(const LsSceneView&) = delete;

  // Copied, uploaded by the next Render
  void SetPrimitives(const std::vector<LsScenePrimitive>& primitives);
  // Extra lines for the next Render, after which they are cleared
  void AddLine(const LsSceneLine& line) { lines.push_back(line); }
  // Records the rendering into the current frame. Call between BeginFrame and
  // EndFrame, before the texture is drawn by the UI.
  void Render(uint32_t width, uint32_t height, const LsCamera& camera, const LsSceneViewSettings& settings);
  // Valid after the first Render, changes when the size does
  ImTextureID GetTexture() const;

  // Queues a ray to trace like LsRaycastPrimitives, against the primitives the
  // next Render uploads. Its result is ready in the frame after that Render.
  LsRaycastTicket QueueRaycast(const LsRay& ray, LsSceneRoles roleMask, float maxDistance);
  // The result of a queued ray, hit is set when it hit something. Results are
  // kept until the Render after they are ready, so read them before that.
  LsRaycastStatus GetRaycast(LsRaycastTicket ticket, LsSceneHit& hit);
};
