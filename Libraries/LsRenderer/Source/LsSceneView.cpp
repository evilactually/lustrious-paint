#include <LsSceneView.h>

#include <algorithm>
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
  uint32_t pickCount;
  float debugDepthRange;
  uint32_t pad;
  uint32_t pickPixels[4]; // xy and zw
};
static_assert(sizeof(LsViewportPushConstants) == 112, "LsViewportPushConstants must match viewport.slang");

// Mirrors PickResult in Shaders/viewport.slang
struct LsPickResult
{
  glm::vec4 position; // w = distance along the ray
  uint32_t nodeId;
  uint32_t hit;
  uint32_t pad[2];
};
static_assert(sizeof(LsPickResult) == 32, "LsPickResult must match viewport.slang");

// Pixels whose hits one Render reads back, others wait for the next one
static const uint32_t kMaxPickPixels = 2;

// Mirrors SceneLinePushConstants in Shaders/sceneline.slang
struct LsSceneLinePushConstants
{
  glm::vec4 positions[2]; // clip space
  glm::vec4 color;
  float fadeDistance;
  float pad[3];
};
static_assert(sizeof(LsSceneLinePushConstants) == 64, "LsSceneLinePushConstants must match sceneline.slang");

static const uint32_t kViewLit = 1u << 0;
static const uint32_t kViewOutlines = 1u << 1;
static const uint32_t kViewOutlinesOnly = 1u << 2;
static const uint32_t kViewDebugPick = 1u << 3;
static const uint32_t kViewDebugDepth = 1u << 4;
static const uint32_t kWorkGroupSize = 8;
// Lines nearer than this are cut off, in world units
static const float kLineNearDepth = 0.01f;

LsSceneView::LsSceneView(std::shared_ptr<LsRenderer> renderer) : renderer(renderer), device(renderer->device) {
  shader = CreateShaderModule(device, "Shaders/viewport.comp.spv");

  // Colour image, primitives, depth image, pick results
  VkDescriptorType types[4] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
  VkDescriptorSetLayoutBinding bindings[4] = {};
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
    throw std::string("Could not create scene view descriptor set layout!");
  }

  // The tracer's set writes the colour and depth images, the line set reads the depth
  VkDescriptorPoolSize poolSizes[3] = {
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
  };
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets = 2;
  poolInfo.poolSizeCount = 3;
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

  CreateLinePipeline();
  CreatePickBuffer();
  CreatePrimitiveBuffer(64);
}

LsSceneView::~LsSceneView() {
  vkDeviceWaitIdle(device);
  DestroyTarget();
  DestroyPrimitiveBuffer();
  DestroyPickBuffer();
  vkDestroyPipeline(device, linePipeline, nullptr);
  vkDestroyPipelineLayout(device, linePipelineLayout, nullptr);
  vkDestroyRenderPass(device, lineRenderPass, nullptr);
  vkDestroyDescriptorSetLayout(device, lineDescriptorSetLayout, nullptr);
  vkDestroyShaderModule(device, lineVertexShader, nullptr);
  vkDestroyShaderModule(device, lineFragmentShader, nullptr);
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

void LsSceneView::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                              VkImage& image, VkDeviceMemory& memory, VkImageView& view) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = format;
  imageInfo.extent = { width, height, 1 };
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::string("Could not create scene view image!");
  }

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(device, image, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
    throw std::string("Could not allocate scene view image memory!");
  }
  vkBindImageMemory(device, image, memory, 0);

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
    throw std::string("Could not create scene view image view!");
  }
}

void LsSceneView::CreateTarget(uint32_t width, uint32_t height) {
  CreateImage(width, height, VK_FORMAT_R8G8B8A8_UNORM,
              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              target.image, target.memory, target.view);
  CreateImage(width, height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              target.depthImage, target.depthMemory, target.depthView);

  VkFramebufferCreateInfo framebufferInfo = {};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = lineRenderPass;
  framebufferInfo.attachmentCount = 1;
  framebufferInfo.pAttachments = &target.view;
  framebufferInfo.width = width;
  framebufferInfo.height = height;
  framebufferInfo.layers = 1;
  if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &target.framebuffer) != VK_SUCCESS) {
    throw std::string("Could not create scene view framebuffer!");
  }

  // Written by compute and the lines, and sampled by the UI, all in the general layout
  target.texture = ImGui_ImplVulkan_AddTexture(sampler, target.view, VK_IMAGE_LAYOUT_GENERAL);
  target.width = width;
  target.height = height;
  UpdateDescriptors();
}

void LsSceneView::DestroyTarget() {
  if (target.texture) {
    ImGui_ImplVulkan_RemoveTexture(target.texture);
  }
  if (target.framebuffer) {
    vkDestroyFramebuffer(device, target.framebuffer, nullptr);
  }
  VkImageView views[2] = { target.view, target.depthView };
  VkImage images[2] = { target.image, target.depthImage };
  VkDeviceMemory memories[2] = { target.memory, target.depthMemory };
  for (int i = 0; i < 2; i++) {
    if (views[i]) {
      vkDestroyImageView(device, views[i], nullptr);
    }
    if (images[i]) {
      vkDestroyImage(device, images[i], nullptr);
    }
    if (memories[i]) {
      vkFreeMemory(device, memories[i], nullptr);
    }
  }
  target = {};
}

void LsSceneView::CreateLinePipeline() {
  lineVertexShader = CreateShaderModule(device, "Shaders/sceneline.vert.spv");
  lineFragmentShader = CreateShaderModule(device, "Shaders/sceneline.frag.spv");

  VkDescriptorSetLayoutBinding binding = {};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &binding;
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &lineDescriptorSetLayout) != VK_SUCCESS) {
    throw std::string("Could not create scene line descriptor set layout!");
  }
  VkDescriptorSetAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &lineDescriptorSetLayout;
  if (vkAllocateDescriptorSets(device, &allocateInfo, &lineDescriptorSet) != VK_SUCCESS) {
    throw std::string("Could not allocate scene line descriptor set!");
  }

  VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LsSceneLinePushConstants) };
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &lineDescriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &linePipelineLayout) != VK_SUCCESS) {
    throw std::string("Could not create scene line pipeline layout!");
  }

  // Draws over what the tracer wrote, the image stays in the general layout throughout
  VkAttachmentDescription attachment = {};
  attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
  attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
  VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_GENERAL };
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorReference;
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &attachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &lineRenderPass) != VK_SUCCESS) {
    throw std::string("Could not create scene line render pass!");
  }

  VkPipelineShaderStageCreateInfo stages[2] = {};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = lineVertexShader;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = lineFragmentShader;
  stages[1].pName = "main";

  VkPipelineVertexInputStateCreateInfo vertexInput = {};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  // The size and the line width change, so they are set when drawing
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;
  VkPipelineRasterizationStateCreateInfo rasterization = {};
  rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization.cullMode = VK_CULL_MODE_NONE;
  rasterization.lineWidth = 1.0f;
  VkPipelineMultisampleStateCreateInfo multisample = {};
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  VkPipelineColorBlendAttachmentState blendAttachment = {};
  blendAttachment.blendEnable = VK_TRUE;
  blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  VkPipelineColorBlendStateCreateInfo colorBlend = {};
  colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlend.attachmentCount = 1;
  colorBlend.pAttachments = &blendAttachment;
  VkDynamicState dynamicStates[3] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };
  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 3;
  dynamicState.pDynamicStates = dynamicStates;

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = stages;
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterization;
  pipelineInfo.pMultisampleState = &multisample;
  pipelineInfo.pColorBlendState = &colorBlend;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = linePipelineLayout;
  pipelineInfo.renderPass = lineRenderPass;
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &linePipeline) != VK_SUCCESS) {
    throw std::string("Could not create scene line pipeline!");
  }
}

void LsSceneView::CreatePickBuffer() {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = kMaxPickPixels*sizeof(LsPickResult);
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(device, &bufferInfo, nullptr, &pickBuffer.buffer) != VK_SUCCESS) {
    throw std::string("Could not create scene view pick buffer!");
  }

  // Read by the host every frame something is picked
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, pickBuffer.buffer, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &pickBuffer.memory) != VK_SUCCESS) {
    throw std::string("Could not allocate scene view pick buffer memory!");
  }
  vkBindBufferMemory(device, pickBuffer.buffer, pickBuffer.memory, 0);
  vkMapMemory(device, pickBuffer.memory, 0, VK_WHOLE_SIZE, 0, &pickBuffer.mapped);
}

void LsSceneView::DestroyPickBuffer() {
  if (pickBuffer.mapped) {
    vkUnmapMemory(device, pickBuffer.memory);
  }
  if (pickBuffer.buffer) {
    vkDestroyBuffer(device, pickBuffer.buffer, nullptr);
  }
  if (pickBuffer.memory) {
    vkFreeMemory(device, pickBuffer.memory, nullptr);
  }
  pickBuffer = {};
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
  VkDescriptorImageInfo depthInfo = { VK_NULL_HANDLE, target.depthView, VK_IMAGE_LAYOUT_GENERAL };
  VkDescriptorBufferInfo bufferInfo = { primitiveBuffer.buffer, 0, VK_WHOLE_SIZE };
  VkDescriptorBufferInfo pickInfo = { pickBuffer.buffer, 0, VK_WHOLE_SIZE };

  VkWriteDescriptorSet writes[5] = {};
  uint32_t writeCount = 0;
  auto writeImage = [&](VkDescriptorSet set, uint32_t binding, const VkDescriptorImageInfo* info,
                        VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
    writes[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[writeCount].dstSet = set;
    writes[writeCount].dstBinding = binding;
    writes[writeCount].descriptorCount = 1;
    writes[writeCount].descriptorType = type;
    writes[writeCount].pImageInfo = info;
    writeCount++;
  };
  if (target.view) {
    writeImage(descriptorSet, 0, &imageInfo);
    writeImage(descriptorSet, 2, &depthInfo);
    writeImage(lineDescriptorSet, 0, &depthInfo, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
  }
  auto writeBuffer = [&](uint32_t binding, const VkDescriptorBufferInfo* info) {
    writes[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[writeCount].dstSet = descriptorSet;
    writes[writeCount].dstBinding = binding;
    writes[writeCount].descriptorCount = 1;
    writes[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[writeCount].pBufferInfo = info;
    writeCount++;
  };
  if (primitiveBuffer.buffer) {
    writeBuffer(1, &bufferInfo);
  }
  if (pickBuffer.buffer) {
    writeBuffer(3, &pickInfo);
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

LsPickTicket LsSceneView::RequestPick(glm::uvec2 pixel) {
  LsPickTicket ticket = nextTicket++;
  requestedPicks.push_back({ ticket, pixel });
  return ticket;
}

LsPickStatus LsSceneView::GetPick(LsPickTicket ticket, LsSceneHit& hit) {
  CollectPicks();
  auto found = completed.find(ticket);
  if (found != completed.end()) {
    if (!found->second.hit) {
      return LsPickStatus::Miss;
    }
    hit = found->second.sceneHit;
    return LsPickStatus::Hit;
  }
  bool requested = std::any_of(requestedPicks.begin(), requestedPicks.end(), [&](const PickRequest& r) { return r.ticket == ticket; });
  bool rendered = std::any_of(renderedPicks.begin(), renderedPicks.end(), [&](const auto& r) { return r.first == ticket; });
  return requested || rendered ? LsPickStatus::Pending : LsPickStatus::Unknown;
}

void LsSceneView::CollectPicks() {
  if (renderedPicks.empty() || renderer->frameNumber <= renderedFrame) {
    return;
  }
  completed.clear();
  const LsPickResult* results = static_cast<const LsPickResult*>(pickBuffer.mapped);
  for (const auto& rendered : renderedPicks) {
    uint32_t slot = rendered.second;
    Completed result = {};
    result.hit = results[slot].hit != 0;
    if (result.hit) {
      result.sceneHit.node = results[slot].nodeId;
      result.sceneHit.position = glm::vec3(results[slot].position);
      result.sceneHit.distance = results[slot].position.w;
    }
    completed[rendered.first] = result;
  }
  renderedPicks.clear();
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

  VkImageMemoryBarrier barriers[2] = {};
  for (int i = 0; i < 2; i++) {
    barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[i].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[i].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    // The old contents are overwritten completely
    barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[i].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  }
  barriers[0].image = target.image;
  barriers[1].image = target.depthImage;
  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 0, nullptr, 0, nullptr, 2, barriers);

  LsViewportPushConstants constants = {};
  float tanHalfFov = camera.GetTanHalfFovY();
  constants.origin = glm::vec4(camera.GetPosition(), std::exp2(settings.exposure));
  constants.right = glm::vec4(camera.GetRight()*tanHalfFov*camera.GetAspect(), 0.0f);
  constants.up = glm::vec4(camera.GetUp()*tanHalfFov, 0.0f);
  // A lens shift only moves where the image centre looks, the shader normalises the rays
  glm::vec2 shift = camera.GetLensShift();
  constants.forward = glm::vec4(camera.GetForward() + camera.GetRight()*shift.x + camera.GetUp()*shift.y, 0.0f);
  constants.size[0] = width;
  constants.size[1] = height;
  constants.primitiveCount = static_cast<uint32_t>(primitives.size());
  constants.flags = (settings.lit ? kViewLit : 0) |
                    (settings.outlines || settings.outlinesOnly ? kViewOutlines : 0) |
                    (settings.outlinesOnly ? kViewOutlinesOnly : 0) |
                    (settings.debug == LsSceneViewDebug::Pick ? kViewDebugPick : 0) |
                    (settings.debug == LsSceneViewDebug::Depth ? kViewDebugDepth : 0);
  constants.pixelAngle = 2.0f*tanHalfFov/static_cast<float>(height);
  constants.debugDepthRange = settings.debugDepthRange;

  // The last picks are read before this frame overwrites them, and picks
  // rendered earlier in this frame still own the buffer
  CollectPicks();
  if (renderedPicks.empty()) {
    glm::uvec2 slots[kMaxPickPixels];
    for (auto request = requestedPicks.begin(); request != requestedPicks.end();) {
      // Requests for the same pixel share its slot
      glm::uvec2 pixel = glm::min(request->pixel, glm::uvec2(width - 1, height - 1));
      uint32_t slot = 0;
      while (slot < constants.pickCount && slots[slot] != pixel) {
        slot++;
      }
      if (slot == kMaxPickPixels) {
        ++request;
        continue;
      }
      if (slot == constants.pickCount) {
        slots[constants.pickCount++] = pixel;
        constants.pickPixels[slot*2] = pixel.x;
        constants.pickPixels[slot*2 + 1] = pixel.y;
      }
      renderedPicks.push_back({ request->ticket, slot });
      request = requestedPicks.erase(request);
    }
    renderedFrame = renderer->frameNumber;
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
  vkCmdDispatch(commandBuffer, (width + kWorkGroupSize - 1)/kWorkGroupSize, (height + kWorkGroupSize - 1)/kWorkGroupSize, 1);

  if (constants.pickCount > 0) {
    // Read on the host once the frame's fence has been waited on
    VkMemoryBarrier pickBarrier = {};
    pickBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    pickBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    pickBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0, 1, &pickBarrier, 0, nullptr, 0, nullptr);
  }

  // Lines draw over the colour and read the depth
  barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  barriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barriers[1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0, 0, nullptr, 0, nullptr, 2, barriers);

  if (settings.grid && !settings.outlinesOnly && settings.debug == LsSceneViewDebug::None) {
    AppendGrid(camera);
  }
  DrawLines(commandBuffer, camera);
  lines.clear();

  // The UI samples it later in this frame
  barriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0, 0, nullptr, 0, nullptr, 1, barriers);
}

void LsSceneView::AppendGrid(const LsCamera& camera) {
  struct Level { float spacing; float halfExtent; glm::vec4 color; };
  // Minor lines every unit near the camera, major ones every ten units further out.
  // Light, so they show on mid grey ground as well as on the dark background.
  const Level levels[2] = {
    { 1.0f, 50.0f, glm::vec4(0.9f, 0.9f, 0.9f, 0.22f) },
    { 10.0f, 250.0f, glm::vec4(0.95f, 0.95f, 0.95f, 0.4f) },
  };
  glm::vec3 eye = camera.GetPosition();
  for (const Level& level : levels) {
    // Centred under the camera on whole cells, so the lines don't move with it
    float centerX = std::round(eye.x/level.spacing)*level.spacing;
    float centerZ = std::round(eye.z/level.spacing)*level.spacing;
    int count = static_cast<int>(level.halfExtent/level.spacing);
    for (int i = -count; i <= count; i++) {
      float x = centerX + i*level.spacing;
      float z = centerZ + i*level.spacing;
      // Axes and the coarser level have their own lines
      bool skipX = std::abs(x) < 1.0e-3f || (level.spacing < 10.0f && std::abs(std::fmod(x, 10.0f)) < 1.0e-3f);
      bool skipZ = std::abs(z) < 1.0e-3f || (level.spacing < 10.0f && std::abs(std::fmod(z, 10.0f)) < 1.0e-3f);
      LsSceneLine line;
      line.color = level.color;
      line.fadeDistance = level.halfExtent;
      if (!skipX) {
        line.a = glm::vec3(x, 0.0f, centerZ - level.halfExtent);
        line.b = glm::vec3(x, 0.0f, centerZ + level.halfExtent);
        lines.push_back(line);
      }
      if (!skipZ) {
        line.a = glm::vec3(centerX - level.halfExtent, 0.0f, z);
        line.b = glm::vec3(centerX + level.halfExtent, 0.0f, z);
        lines.push_back(line);
      }
    }
  }

  // World axes along the grid: X red, Z blue
  const float axisLength = 1000.0f;
  LsSceneLine axis;
  axis.width = 2.0f;
  axis.fadeDistance = 500.0f;
  axis.color = glm::vec4(0.85f, 0.3f, 0.3f, 0.9f);
  axis.a = glm::vec3(eye.x - axisLength, 0.0f, 0.0f);
  axis.b = glm::vec3(eye.x + axisLength, 0.0f, 0.0f);
  lines.push_back(axis);
  axis.color = glm::vec4(0.3f, 0.5f, 0.95f, 0.9f);
  axis.a = glm::vec3(0.0f, 0.0f, eye.z - axisLength);
  axis.b = glm::vec3(0.0f, 0.0f, eye.z + axisLength);
  lines.push_back(axis);
}

void LsSceneView::DrawLines(VkCommandBuffer commandBuffer, const LsCamera& camera) {
  VkRenderPassBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  beginInfo.renderPass = lineRenderPass;
  beginInfo.framebuffer = target.framebuffer;
  beginInfo.renderArea = { { 0, 0 }, { target.width, target.height } };
  vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = { 0.0f, 0.0f, float(target.width), float(target.height), 0.0f, 1.0f };
  VkRect2D scissor = { { 0, 0 }, { target.width, target.height } };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipelineLayout, 0, 1, &lineDescriptorSet, 0, nullptr);

  // Clip space with w as the view depth and z from 0 at the near plane, so the
  // rasteriser cuts lines there. Vulkan's y points down.
  glm::vec3 position = camera.GetPosition();
  glm::vec3 right = camera.GetRight();
  glm::vec3 up = camera.GetUp();
  glm::vec3 forward = camera.GetForward();
  glm::vec2 shift = camera.GetLensShift();
  float tanY = camera.GetTanHalfFovY();
  float tanX = tanY*camera.GetAspect();
  auto toClip = [&](glm::vec3 p) {
    glm::vec3 d = p - position;
    float depth = glm::dot(d, forward);
    float x = glm::dot(d, right) - shift.x*depth;
    float y = glm::dot(d, up) - shift.y*depth;
    return glm::vec4(x/tanX, -y/tanY, depth - kLineNearDepth, depth);
  };

  float width = -1.0f;
  for (const LsSceneLine& line : lines) {
    LsSceneLinePushConstants constants = {};
    constants.positions[0] = toClip(line.a);
    constants.positions[1] = toClip(line.b);
    constants.color = line.color;
    constants.fadeDistance = line.fadeDistance;
    if (line.width != width) {
      width = line.width;
      vkCmdSetLineWidth(commandBuffer, width);
    }
    vkCmdPushConstants(commandBuffer, linePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(constants), &constants);
    vkCmdDraw(commandBuffer, 2, 1, 0, 0);
  }
  vkCmdEndRenderPass(commandBuffer);
}
