#include <LsCanvas.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <LsRenderer.h>
#include <LsVulkanCommon.h>

// Mirrors CanvasPushConstants in Shaders/canvas.slang
struct LsCanvasPushConstants
{
  glm::vec4 origin;  // xyz = canvas camera position
  glm::vec4 right;   // scaled by tan(fov/2)*aspect
  glm::vec4 up;      // scaled by tan(fov/2)
  glm::vec4 forward; // unit length view direction plus the lens shift
  uint32_t size[2];
  int32_t offset[2];
  glm::vec2 dabCenter;
  float dabRadius;
  uint32_t primitiveCount;
  float pixelAngle;
  uint32_t display;
  float isolineSpacing; // 0 for none
  float pad;
};
static_assert(sizeof(LsCanvasPushConstants) == 112, "LsCanvasPushConstants must match canvas.slang");

static const uint32_t kWorkGroupSize = 8;

LsCanvas::LsCanvas(std::shared_ptr<LsRenderer> renderer) :
  renderer(renderer), device(renderer->device),
  width(renderer->canvasState.width), height(renderer->canvasState.height) {
  paintShader = CreateShaderModule(device, "Shaders/canvas.paint.comp.spv");
  rangeShader = CreateShaderModule(device, "Shaders/canvas.range.comp.spv");
  resolveShader = CreateShaderModule(device, "Shaders/canvas.resolve.comp.spv");

  VkDescriptorSetLayoutBinding bindings[4] = {};
  const VkDescriptorType types[4] = {
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // distance
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // the image the renderer draws as the canvas
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // scene primitives
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // distance range
  };
  for (uint32_t i = 0; i < 4; i++) {
    bindings[i].binding = i;
    bindings[i].descriptorType = types[i];
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 4;
  layoutInfo.pBindings = bindings;
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::string("Could not create canvas descriptor set layout!");
  }

  VkDescriptorPoolSize poolSizes[2] = {
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 },
  };
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 2;
  poolInfo.pPoolSizes = poolSizes;
  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::string("Could not create canvas descriptor pool!");
  }

  VkDescriptorSetAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &descriptorSetLayout;
  if (vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet) != VK_SUCCESS) {
    throw std::string("Could not allocate canvas descriptor set!");
  }

  VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(LsCanvasPushConstants) };
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::string("Could not create canvas pipeline layout!");
  }

  paintPipeline = CreatePipeline(paintShader);
  rangePipeline = CreatePipeline(rangeShader);
  resolvePipeline = CreatePipeline(resolveShader);

  CreateRangeBuffer();
  CreateDistanceImage();
  CreatePrimitiveBuffer(64);
}

LsCanvas::~LsCanvas() {
  vkDeviceWaitIdle(device);
  DestroyPrimitiveBuffer();
  vkDestroyImageView(device, distance.view, nullptr);
  vkDestroyImage(device, distance.image, nullptr);
  vkFreeMemory(device, distance.memory, nullptr);
  vkDestroyBuffer(device, rangeBuffer.buffer, nullptr);
  vkFreeMemory(device, rangeBuffer.memory, nullptr);
  vkDestroyPipeline(device, paintPipeline, nullptr);
  vkDestroyPipeline(device, rangePipeline, nullptr);
  vkDestroyPipeline(device, resolvePipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  vkDestroyShaderModule(device, paintShader, nullptr);
  vkDestroyShaderModule(device, rangeShader, nullptr);
  vkDestroyShaderModule(device, resolveShader, nullptr);
}

uint32_t LsCanvas::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(renderer->physicalDevice, &memoryProperties);
  int32_t type = ::FindMemoryType(memoryProperties, typeBits, properties);
  if (type < 0) {
    throw std::string("No suitable memory type for the canvas!");
  }
  return static_cast<uint32_t>(type);
}

VkPipeline LsCanvas::CreatePipeline(VkShaderModule shader) {
  VkComputePipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipelineInfo.stage.module = shader;
  pipelineInfo.stage.pName = "main";
  VkPipeline pipeline = VK_NULL_HANDLE;
  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
    throw std::string("Could not create canvas pipeline!");
  }
  return pipeline;
}

void LsCanvas::CreateDistanceImage() {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  // fp32 and linear, shadow rays will start from these positions
  imageInfo.format = VK_FORMAT_R32_SFLOAT;
  imageInfo.extent = { width, height, 1 };
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(device, &imageInfo, nullptr, &distance.image) != VK_SUCCESS) {
    throw std::string("Could not create canvas distance image!");
  }

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(device, distance.image, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &distance.memory) != VK_SUCCESS) {
    throw std::string("Could not allocate canvas distance image memory!");
  }
  vkBindImageMemory(device, distance.image, distance.memory, 0);

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = distance.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = imageInfo.format;
  viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  if (vkCreateImageView(device, &viewInfo, nullptr, &distance.view) != VK_SUCCESS) {
    throw std::string("Could not create canvas distance image view!");
  }
  // Cleared by the first Render, which has a command buffer to do it with
  distance.cleared = false;
  UpdateDescriptors();
}

void LsCanvas::CreateRangeBuffer() {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = 2*sizeof(uint32_t);
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(device, &bufferInfo, nullptr, &rangeBuffer.buffer) != VK_SUCCESS) {
    throw std::string("Could not create canvas range buffer!");
  }

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, rangeBuffer.buffer, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &rangeBuffer.memory) != VK_SUCCESS) {
    throw std::string("Could not allocate canvas range buffer memory!");
  }
  vkBindBufferMemory(device, rangeBuffer.buffer, rangeBuffer.memory, 0);
}

void LsCanvas::CreatePrimitiveBuffer(size_t capacity) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = capacity*sizeof(LsScenePrimitive);
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(device, &bufferInfo, nullptr, &primitiveBuffer.buffer) != VK_SUCCESS) {
    throw std::string("Could not create canvas primitive buffer!");
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
    throw std::string("Could not allocate canvas primitive buffer memory!");
  }
  vkBindBufferMemory(device, primitiveBuffer.buffer, primitiveBuffer.memory, 0);
  vkMapMemory(device, primitiveBuffer.memory, 0, VK_WHOLE_SIZE, 0, &primitiveBuffer.mapped);
  primitiveBuffer.capacity = capacity;
  primitivesDirty = true;
  UpdateDescriptors();
}

void LsCanvas::DestroyPrimitiveBuffer() {
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

void LsCanvas::UpdateDescriptors() {
  VkDescriptorImageInfo distanceInfo = { VK_NULL_HANDLE, distance.view, VK_IMAGE_LAYOUT_GENERAL };
  VkDescriptorImageInfo displayInfo = { VK_NULL_HANDLE, renderer->canvasState.view, VK_IMAGE_LAYOUT_GENERAL };
  VkDescriptorBufferInfo bufferInfo = { primitiveBuffer.buffer, 0, VK_WHOLE_SIZE };
  VkDescriptorBufferInfo rangeInfo = { rangeBuffer.buffer, 0, VK_WHOLE_SIZE };

  VkWriteDescriptorSet writes[4] = {};
  uint32_t writeCount = 0;
  auto write = [&](uint32_t binding, VkDescriptorType type) -> VkWriteDescriptorSet& {
    VkWriteDescriptorSet& w = writes[writeCount++];
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet = descriptorSet;
    w.dstBinding = binding;
    w.descriptorCount = 1;
    w.descriptorType = type;
    return w;
  };
  if (distance.view) {
    write(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE).pImageInfo = &distanceInfo;
    write(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE).pImageInfo = &displayInfo;
  }
  if (primitiveBuffer.buffer) {
    write(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).pBufferInfo = &bufferInfo;
  }
  if (rangeBuffer.buffer) {
    write(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).pBufferInfo = &rangeInfo;
  }
  vkUpdateDescriptorSets(device, writeCount, writes, 0, nullptr);
}

void LsCanvas::SetPrimitives(const std::vector<LsScenePrimitive>& primitives) {
  this->primitives = primitives;
  primitivesDirty = true;
}

void LsCanvas::SetCamera(const LsCamera& camera) {
  this->camera = camera;
  hasCamera = true;
}

void LsCanvas::SetDisplay(LsCanvasDisplay display) {
  if (display != this->display) {
    this->display = display;
    resolveNeeded = true;
  }
}

void LsCanvas::SetIsolines(bool shown) {
  if (shown != isolines) {
    isolines = shown;
    resolveNeeded = true;
  }
}

void LsCanvas::SetIsolineSpacing(float spacing) {
  spacing = std::max(spacing, 1.0e-4f);
  if (spacing != isolineSpacing) {
    isolineSpacing = spacing;
    resolveNeeded = true;
  }
}

void LsCanvas::Paint(const std::vector<glm::vec2>& centers, float radius) {
  for (const glm::vec2& center : centers) {
    dabs.push_back({ center, radius });
  }
}

void LsCanvas::Render() {
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

  LsCanvasPushConstants constants = {};
  if (hasCamera) {
    float tanHalfFov = camera.GetTanHalfFovY();
    constants.origin = glm::vec4(camera.GetPosition(), 0.0f);
    constants.right = glm::vec4(camera.GetRight()*tanHalfFov*camera.GetAspect(), 0.0f);
    constants.up = glm::vec4(camera.GetUp()*tanHalfFov, 0.0f);
    // A lens shift only moves where the image centre looks, the shader normalises the rays
    glm::vec2 shift = camera.GetLensShift();
    constants.forward = glm::vec4(camera.GetForward() + camera.GetRight()*shift.x + camera.GetUp()*shift.y, 0.0f);
    constants.pixelAngle = 2.0f*tanHalfFov/static_cast<float>(height);
  }
  else {
    // Nothing to trace from
    dabs.clear();
  }
  constants.size[0] = width;
  constants.size[1] = height;
  constants.primitiveCount = static_cast<uint32_t>(primitives.size());
  constants.display = static_cast<uint32_t>(display);
  constants.isolineSpacing = isolines ? isolineSpacing : 0.0f;

  // Positions follow the camera, so moving it changes what the canvas shows
  const glm::vec4 frame[4] = { constants.origin, constants.right, constants.up, constants.forward };
  if (hasCamera && std::memcmp(frame, resolvedFrame, sizeof(frame)) != 0) {
    resolveNeeded = true;
  }
  bool resolve = hasCamera && (resolveNeeded || !dabs.empty());
  if (distance.cleared && dabs.empty() && !resolve) {
    return;
  }

  // Dispatches can't be recorded inside a render pass
  if (renderer->drawingContext.drawing) {
    renderer->EndDrawing();
  }
  VkCommandBuffer commandBuffer = renderer->commandBuffer;

  VkImageMemoryBarrier distanceBarrier = {};
  distanceBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  distanceBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  distanceBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  distanceBarrier.image = distance.image;
  distanceBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

  if (!distance.cleared) {
    // No pixel has a position yet, and pixels without one show black, also
    // without a canvas camera to resolve the canvas image with
    VkImageMemoryBarrier clearBarriers[2] = { distanceBarrier, distanceBarrier };
    clearBarriers[1].image = renderer->canvasState.image;
    for (VkImageMemoryBarrier& barrier : clearBarriers) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 2, clearBarriers);
    VkClearColorValue zero = {};
    VkClearColorValue black = {};
    black.float32[3] = 1.0f;
    vkCmdClearColorImage(commandBuffer, distance.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &zero, 1,
                         &distanceBarrier.subresourceRange);
    vkCmdClearColorImage(commandBuffer, renderer->canvasState.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black, 1,
                         &distanceBarrier.subresourceRange);
    for (VkImageMemoryBarrier& barrier : clearBarriers) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    }
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 2, clearBarriers);
    distance.cleared = true;
  }

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  // Each dab reads and writes the attributes after the previous one, so they composite in order
  VkMemoryBarrier computeBarrier = {};
  computeBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  computeBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  computeBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  if (!dabs.empty()) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, paintPipeline);
  }
  bool dispatched = false;
  for (const Dab& dab : dabs) {
    // Only the pixels whose centres the dab can cover
    glm::ivec2 first = glm::max(glm::ivec2(glm::floor(dab.center - dab.radius - 0.5f)), glm::ivec2(0));
    glm::ivec2 last = glm::min(glm::ivec2(glm::ceil(dab.center + dab.radius - 0.5f)),
                               glm::ivec2(static_cast<int>(width) - 1, static_cast<int>(height) - 1));
    if (last.x < first.x || last.y < first.y) {
      continue;
    }
    if (dispatched) {
      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 1, &computeBarrier, 0, nullptr, 0, nullptr);
    }
    constants.offset[0] = first.x;
    constants.offset[1] = first.y;
    constants.dabCenter = dab.center;
    constants.dabRadius = dab.radius;
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    glm::uvec2 extent = glm::uvec2(last - first) + 1u;
    vkCmdDispatch(commandBuffer, (extent.x + kWorkGroupSize - 1)/kWorkGroupSize, (extent.y + kWorkGroupSize - 1)/kWorkGroupSize, 1);
    dispatched = true;
  }
  dabs.clear();

  if (resolve) {
    // After the dabs, and after the canvas image was last drawn
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 1, &computeBarrier, 0, nullptr, 0, nullptr);
    VkImageMemoryBarrier displayBarrier = {};
    displayBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    displayBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    displayBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    displayBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    displayBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    displayBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    displayBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    displayBarrier.image = renderer->canvasState.image;
    displayBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &displayBarrier);

    constants.offset[0] = 0;
    constants.offset[1] = 0;
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);

    if (display == LsCanvasDisplay::Depth) {
      // Depth is shown relative to the nearest and furthest painted pixel. Read by the
      // previous resolve, reset, gathered and then read by this one.
      VkBufferMemoryBarrier rangeBarrier = {};
      rangeBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      rangeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      rangeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      rangeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      rangeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      rangeBarrier.buffer = rangeBuffer.buffer;
      rangeBarrier.size = VK_WHOLE_SIZE;
      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           0, 0, nullptr, 1, &rangeBarrier, 0, nullptr);
      vkCmdFillBuffer(commandBuffer, rangeBuffer.buffer, 0, sizeof(uint32_t), ~0u);
      vkCmdFillBuffer(commandBuffer, rangeBuffer.buffer, sizeof(uint32_t), sizeof(uint32_t), 0u);
      rangeBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      rangeBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 0, nullptr, 1, &rangeBarrier, 0, nullptr);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, rangePipeline);
      vkCmdDispatch(commandBuffer, (width + kWorkGroupSize - 1)/kWorkGroupSize, (height + kWorkGroupSize - 1)/kWorkGroupSize, 1);

      rangeBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      rangeBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 0, nullptr, 1, &rangeBarrier, 0, nullptr);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, resolvePipeline);
    vkCmdDispatch(commandBuffer, (width + kWorkGroupSize - 1)/kWorkGroupSize, (height + kWorkGroupSize - 1)/kWorkGroupSize, 1);

    // The canvas and the 3D view sample it later in this frame
    displayBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    displayBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &displayBarrier);

    std::memcpy(resolvedFrame, frame, sizeof(frame));
    resolveNeeded = false;
  }
}
