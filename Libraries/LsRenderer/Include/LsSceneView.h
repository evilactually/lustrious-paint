#pragma once

#include <cstdint>
#include <memory>
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

struct LsSceneViewSettings
{
  // Light the scene with its emitters and shadow casters instead of studio lights
  bool lit = false;
  bool grid = true;
  // In stops, used when lit
  float exposure = 0.0f;
};

//-------------------------------------------------------------------------------
// @ LsSceneView
//-------------------------------------------------------------------------------
// Renders the scene primitives from a camera into an image the UI can show with
// ImGui::Image. A compute shader sphere traces the signed distance functions
// directly, there is no mesh conversion.
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

  struct {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDescriptorSet texture = VK_NULL_HANDLE; // ImGui's descriptor for sampling it
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

  uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
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
  // Records the rendering into the current frame. Call between BeginFrame and
  // EndFrame, before the texture is drawn by the UI.
  void Render(uint32_t width, uint32_t height, const LsCamera& camera, const LsSceneViewSettings& settings);
  // Valid after the first Render, changes when the size does
  ImTextureID GetTexture() const;
};
