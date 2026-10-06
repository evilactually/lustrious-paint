#include <LsSceneView.h>

#include <cmath>
#include <cstring>
#include <string>
#include <LsRenderer.h>
#include <LsVulkanCommon.h>
#include <imgui_impl_vulkan.h>

// Mirrors ViewportPushConstants in Shaders/viewport.slang
struct LsViewportPushConstants
{
  glm::vec4 origin;  // xyz = camera position, w = exposure multiplier
  glm::vec4 right;   // scaled by tan(fov/2)*aspect
  glm::vec4 up;      // scaled by tan(fov/2)
  glm::vec4 forward; // unit length
  uint32_t size[2];
  uint32_t primitiveCount;
  uint32_t flags;
  float pixelAngle;
  float pad[3];
};
static_assert(sizeof(LsViewportPushConstants) == 96, "LsViewportPushConstants must match viewport.slang");

static const uint32_t kViewLit  = 1u << 0;
static const uint32_t kViewGrid = 1u << 1;
static const uint32_t kWorkGroupSize = 8;

LsSceneView::LsSceneView(std::shared_ptr<LsRenderer> renderer) : renderer(renderer), device(renderer->device) {
  shader = CreateShaderModule(device, "Shaders/viewport.comp.spv");

  VkDescriptorSetLayoutBinding bindings[2] = {};
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 2;
  layoutInfo.pBindings = bindings;
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::string("Could not create scene view descriptor set layout!");
  }

  VkDescriptorPoolSize poolSizes[2] = {
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
  };
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 2;
  poolInfo.pPoolSizes = poolSizes;
  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::string("Could not create scene view descriptor pool!");
  }

  VkDescriptorSetAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &descriptorSetLayout;
  if (vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet) != VK_SUCCESS) {
    throw std::string("Could not allocate scene view descriptor set!");
  }

  VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(LsViewportPushConstants) };
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::string("Could not create scene view pipeline layout!");
  }

  VkComputePipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipelineInfo.stage.module = shader;
  pipelineInfo.stage.pName = "main";
  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
    throw std::string("Could not create scene view pipeline!");
  }

  // The image is shown at its own size, nearest keeps it crisp
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.maxLod = 0.0f;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
    throw std::string("Could not create scene view sampler!");
  }

  CreatePrimitiveBuffer(64);
}

LsSceneView::~LsSceneView() {
  vkDeviceWaitIdle(device);
  DestroyTarget();
  DestroyPrimitiveBuffer();
  vkDestroySampler(device, sampler, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  vkDestroyShaderModule(device, shader, nullptr);
}

uint32_t LsSceneView::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(renderer->physicalDevice, &memoryProperties);
  int32_t type = ::FindMemoryType(memoryProperties, typeBits, properties);
  if (type < 0) {
    throw std::string("No suitable memory type for the scene view!");
  }
  return static_cast<uint32_t>(type);
}

void LsSceneView::CreateTarget(uint32_t width, uint32_t height) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.extent = { width, height, 1 };
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(device, &imageInfo, nullptr, &target.image) != VK_SUCCESS) {
    throw std::string("Could not create scene view image!");
  }

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(device, target.image, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &target.memory) != VK_SUCCESS) {
    throw std::string("Could not allocate scene view image memory!");
  }
  vkBindImageMemory(device, target.image, target.memory, 0);

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = target.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = imageInfo.format;
  viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  if (vkCreateImageView(device, &viewInfo, nullptr, &target.view) != VK_SUCCESS) {
    throw std::string("Could not create scene view image view!");
  }

  // Written by compute and sampled by the UI without changing layout
  target.texture = ImGui_ImplVulkan_AddTexture(sampler, target.view, VK_IMAGE_LAYOUT_GENERAL);
  target.width = width;
  target.height = height;
  UpdateDescriptors();
}

void LsSceneView::DestroyTarget() {
  if (target.texture) {
    ImGui_ImplVulkan_RemoveTexture(target.texture);
  }
  if (target.view) {
    vkDestroyImageView(device, target.view, nullptr);
  }
  if (target.image) {
    vkDestroyImage(device, target.image, nullptr);
  }
  if (target.memory) {
    vkFreeMemory(device, target.memory, nullptr);
  }
  target = {};
}

void LsSceneView::CreatePrimitiveBuffer(size_t capacity) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = capacity*sizeof(LsScenePrimitive);
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(device, &bufferInfo, nullptr, &primitiveBuffer.buffer) != VK_SUCCESS) {
    throw std::string("Could not create scene view primitive buffer!");
  }

  // Small and rewritten whenever the scene changes, so it stays in host memory
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, primitiveBuffer.buffer, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &primitiveBuffer.memory) != VK_SUCCESS) {
    throw std::string("Could not allocate scene view primitive buffer memory!");
  }
  vkBindBufferMemory(device, primitiveBuffer.buffer, primitiveBuffer.memory, 0);
  vkMapMemory(device, primitiveBuffer.memory, 0, VK_WHOLE_SIZE, 0, &primitiveBuffer.mapped);
  primitiveBuffer.capacity = capacity;
  primitivesDirty = true;
  UpdateDescriptors();
}

void LsSceneView::DestroyPrimitiveBuffer() {
  if (primitiveBuffer.mapped) {
    vkUnmapMemory(device, primitiveBuffer.memory);
  }
  if (primitiveBuffer.buffer) {
    vkDestroyBuffer(device, primitiveBuffer.buffer, nullptr);
  }
  if (primitiveBuffer.memory) {
    vkFreeMemory(device, primitiveBuffer.memory, nullptr);
  }
  primitiveBuffer = {};
}

void LsSceneView::UpdateDescriptors() {
  VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, target.view, VK_IMAGE_LAYOUT_GENERAL };
  VkDescriptorBufferInfo bufferInfo = { primitiveBuffer.buffer, 0, VK_WHOLE_SIZE };

  VkWriteDescriptorSet writes[2] = {};
  uint32_t writeCount = 0;
  if (target.view) {
    writes[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[writeCount].dstSet = descriptorSet;
    writes[writeCount].dstBinding = 0;
    writes[writeCount].descriptorCount = 1;
    writes[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[writeCount].pImageInfo = &imageInfo;
    writeCount++;
  }
  if (primitiveBuffer.buffer) {
    writes[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[writeCount].dstSet = descriptorSet;
    writes[writeCount].dstBinding = 1;
    writes[writeCount].descriptorCount = 1;
    writes[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[writeCount].pBufferInfo = &bufferInfo;
    writeCount++;
  }
  vkUpdateDescriptorSets(device, writeCount, writes, 0, nullptr);
}

void LsSceneView::SetPrimitives(const std::vector<LsScenePrimitive>& primitives) {
  this->primitives = primitives;
  primitivesDirty = true;
}

ImTextureID LsSceneView::GetTexture() const {
  return reinterpret_cast<ImTextureID>(target.texture);
}

void LsSceneView::Render(uint32_t width, uint32_t height, const LsCamera& camera, const LsSceneViewSettings& settings) {
  if (width == 0 || height == 0) {
    return;
  }

  // Replacing resources the previous frame used, it must have finished with them
  if (width != target.width || height != target.height) {
    vkDeviceWaitIdle(device);
    DestroyTarget();
    CreateTarget(width, height);
  }
  if (primitives.size() > primitiveBuffer.capacity) {
    vkDeviceWaitIdle(device);
    DestroyPrimitiveBuffer();
    CreatePrimitiveBuffer(primitives.size()*2);
  }
  if (primitivesDirty) {
    // Host writes before the submission are visible to it without a barrier
    if (!primitives.empty()) {
      std::memcpy(primitiveBuffer.mapped, primitives.data(), primitives.size()*sizeof(LsScenePrimitive));
    }
    primitivesDirty = false;
  }

  // Dispatches can't be recorded inside a render pass
  if (renderer->drawingContext.drawing) {
    renderer->EndDrawing();
  }
  VkCommandBuffer commandBuffer = renderer->commandBuffer;

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  // The old contents are overwritten completely
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = target.image;
  barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 0, nullptr, 0, nullptr, 1, &barrier);

  LsViewportPushConstants constants = {};
  float tanHalfFov = camera.GetTanHalfFovY();
  constants.origin = glm::vec4(camera.GetPosition(), std::exp2(settings.exposure));
  constants.right = glm::vec4(camera.GetRight()*tanHalfFov*camera.GetAspect(), 0.0f);
  constants.up = glm::vec4(camera.GetUp()*tanHalfFov, 0.0f);
  constants.forward = glm::vec4(camera.GetForward(), 0.0f);
  constants.size[0] = width;
  constants.size[1] = height;
  constants.primitiveCount = static_cast<uint32_t>(primitives.size());
  constants.flags = (settings.lit ? kViewLit : 0) | (settings.grid ? kViewGrid : 0);
  constants.pixelAngle = 2.0f*tanHalfFov/static_cast<float>(height);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
  vkCmdDispatch(commandBuffer, (width + kWorkGroupSize - 1)/kWorkGroupSize, (height + kWorkGroupSize - 1)/kWorkGroupSize, 1);

  // The UI samples it later in this frame
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0, 0, nullptr, 0, nullptr, 1, &barrier);
}
