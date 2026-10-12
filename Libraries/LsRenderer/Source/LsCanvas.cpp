#include <LsCanvas.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <glm/gtc/constants.hpp>
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
  uint32_t dabMode;
  uint32_t dabMaterial[2]; // packed, see PackBrdf
  float dabOpacity;
  float dabFalloff;
};
// The minimum limit for push constants, there is no room left
static_assert(sizeof(LsCanvasPushConstants) == 128, "LsCanvasPushConstants must match canvas.slang");

// Mirrors RenderParameters in Shaders/canvas.slang
struct LsCanvasRenderParameters
{
  float exposure; // multiplier
  uint32_t samplesPerFrame;
  uint32_t maxSamples;
  uint32_t localLightSlots; // bits of local light slots in use
  uint32_t occluderSlots;
  uint32_t pad[3];
};
static_assert(sizeof(LsCanvasRenderParameters) == 32, "LsCanvasRenderParameters must match canvas.slang");

// Mirrors LocalLight in Shaders/canvas.slang
struct LsCanvasLocalLight
{
  glm::vec4 direction; // xyz = world space towards the light, w = cosine of its angular radius
  glm::vec4 radiance;  // rgb = radiance of its disk, w = its solid angle
  uint32_t shadowed;
  uint32_t pad[3];
};
static_assert(sizeof(LsCanvasLocalLight) == 48, "LsCanvasLocalLight must match canvas.slang");

// Mirrors Occluder in Shaders/canvas.slang
struct LsCanvasOccluder
{
  glm::vec4 direction; // xyz = world space towards its centre, w = cosine of its angular radius
  glm::vec4 shape;     // x = cosine of the radius it blocks fully within, y = strength
};
static_assert(sizeof(LsCanvasOccluder) == 32, "LsCanvasOccluder must match canvas.slang");

// Mirrors Rig in Shaders/canvas.slang
struct LsCanvasRig
{
  uint32_t lights;
  uint32_t occluders;
  uint32_t color; // sRGB rgba8
  uint32_t pad;
};
static_assert(sizeof(LsCanvasRig) == 16, "LsCanvasRig must match canvas.slang");

// Mirror MIN_IOR and MAX_IOR in Shaders/canvas.slang
static const float kMinIor = 1.0f;
static const float kMaxIor = 3.0f;

// Packs the parameters the way Material::pack in Shaders/canvas.slang does: the
// sRGB encoded base colour and metallic as 8 bit unorms, roughness and the IOR
// as 16 bit unorms
// Octahedral encoding of a unit normal as encodeNormal in Shaders/canvas.slang does
static uint32_t EncodeNormal(glm::vec3 n) {
  glm::vec2 p = glm::vec2(n.x, n.y)/(std::abs(n.x) + std::abs(n.y) + std::abs(n.z));
  if (n.z < 0.0f) {
    glm::vec2 sign(p.x >= 0.0f ? 1.0f : -1.0f, p.y >= 0.0f ? 1.0f : -1.0f);
    p = (1.0f - glm::abs(glm::vec2(p.y, p.x)))*sign;
  }
  glm::ivec2 q = glm::ivec2(glm::round(glm::clamp(p, -1.0f, 1.0f)*32767.0f));
  return (static_cast<uint32_t>(q.x) & 0xFFFFu) | (static_cast<uint32_t>(q.y) << 16);
}

static void PackBrdf(const LsBrdfParameters& brdf, uint32_t packed[2]) {
  auto unorm = [](float value, float scale) {
    return static_cast<uint32_t>(std::round(glm::clamp(value, 0.0f, 1.0f)*scale));
  };
  auto srgb = [](float c) {
    c = glm::clamp(c, 0.0f, 1.0f);
    return c <= 0.0031308f ? c*12.92f : 1.055f*std::pow(c, 1.0f/2.4f) - 0.055f;
  };
  packed[0] = unorm(srgb(brdf.baseColor.r), 255.0f) | (unorm(srgb(brdf.baseColor.g), 255.0f) << 8) |
              (unorm(srgb(brdf.baseColor.b), 255.0f) << 16) | (unorm(brdf.metallic, 255.0f) << 24);
  packed[1] = unorm(brdf.roughness, 65535.0f) | (unorm((brdf.ior - kMinIor)/(kMaxIor - kMinIor), 65535.0f) << 16);
}

static const uint32_t kWorkGroupSize = 8;
// Mirrors DAB_BLEND in Shaders/canvas.slang, set in dabMode with the mode
static const uint32_t kDabBlend = 1u << 8;
// Mirrors DAB_FIXED_NORMAL in Shaders/canvas.slang
static const uint32_t kDabFixedNormal = 1u << 9;
// Mirrors DAB_CLEAR_RIG in Shaders/canvas.slang
static const uint32_t kDabClearRig = 1u << 10;
// Mirrors DAB_PUSH in Shaders/canvas.slang
static const uint32_t kDabPush = 1u << 11;
// Mirrors DAB_ALONG_NORMAL in Shaders/canvas.slang
static const uint32_t kDabAlongNormal = 1u << 12;
// Mirrors NO_NORMAL in Shaders/canvas.slang, (-32768, -32768) as two snorms, which encoding never produces
static const uint32_t kNoNormal = 0x80008000u;

LsCanvas::LsCanvas(std::shared_ptr<LsRenderer> renderer) :
  renderer(renderer), device(renderer->device),
  width(renderer->canvasState.width), height(renderer->canvasState.height) {
  paintShader = CreateShaderModule(device, "Shaders/canvas.paint.comp.spv");
  blendShader = CreateShaderModule(device, "Shaders/canvas.blend.comp.spv");
  pickShader = CreateShaderModule(device, "Shaders/canvas.pick.comp.spv");
  rigCountShader = CreateShaderModule(device, "Shaders/canvas.rigcount.comp.spv");
  rangeShader = CreateShaderModule(device, "Shaders/canvas.range.comp.spv");
  renderShader = CreateShaderModule(device, "Shaders/canvas.render.comp.spv");
  resolveShader = CreateShaderModule(device, "Shaders/canvas.resolve.comp.spv");

  const uint32_t bindingCount = 20;
  VkDescriptorSetLayoutBinding bindings[bindingCount] = {};
  const VkDescriptorType types[bindingCount] = {
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // distance
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // the image the renderer draws as the canvas
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // scene primitives
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // distance range
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // normal
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // rendered radiance
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // render parameters
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // material colour
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // material surface
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // stroke start distance
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // stroke start normal
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // stroke start material colour
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // stroke start material surface
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // stroke coverage
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // light mask
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // local lights
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // picked normal
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // occluders
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // light rigs
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // rig pixel counts
  };
  for (uint32_t i = 0; i < bindingCount; i++) {
    bindings[i].binding = i;
    bindings[i].descriptorType = types[i];
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = bindingCount;
  layoutInfo.pBindings = bindings;
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::string("Could not create canvas descriptor set layout!");
  }

  VkDescriptorPoolSize poolSizes[3] = {
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 12 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
  };
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 3;
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
  blendPipeline = CreatePipeline(blendShader);
  pickPipeline = CreatePipeline(pickShader);
  rigCountPipeline = CreatePipeline(rigCountShader);
  rangePipeline = CreatePipeline(rangeShader);
  renderPipeline = CreatePipeline(renderShader);
  resolvePipeline = CreatePipeline(resolveShader);

  CreateRangeBuffer();
  CreateParameterBuffer();
  // fp32 and linear, shadow rays will start from these positions
  distance = CreateAttributeImage(VK_FORMAT_R32_SFLOAT, "distance");
  // Packed by hand, storage images of two channel formats are an optional feature
  normal = CreateAttributeImage(VK_FORMAT_R32_UINT, "normal");
  // Sums of many samples of unbounded radiance
  radiance = CreateAttributeImage(VK_FORMAT_R32G32B32A32_SFLOAT, "radiance");
  materialColor = CreateAttributeImage(VK_FORMAT_R32_UINT, "material colour");
  materialSurface = CreateAttributeImage(VK_FORMAT_R32_UINT, "material surface");
  strokeDistance = CreateAttributeImage(VK_FORMAT_R32_SFLOAT, "stroke distance");
  strokeNormal = CreateAttributeImage(VK_FORMAT_R32_UINT, "stroke normal");
  strokeMaterialColor = CreateAttributeImage(VK_FORMAT_R32_UINT, "stroke material colour");
  strokeMaterialSurface = CreateAttributeImage(VK_FORMAT_R32_UINT, "stroke material surface");
  strokeCoverage = CreateAttributeImage(VK_FORMAT_R32_SFLOAT, "stroke coverage");
  rigImage = CreateAttributeImage(VK_FORMAT_R32_UINT, "rig");
  rigUsed[0] = true;
  rigs[0].name = "No rig";
  CreateHostBuffer(kLsMaxRigs*sizeof(LsCanvasRig), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "rig",
                   rigBuffer.buffer, rigBuffer.memory, rigBuffer.mapped);
  // Cleared with vkCmdFillBuffer before each count
  CreateHostBuffer(kLsMaxRigs*sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   "rig count", rigCountBuffer.buffer, rigCountBuffer.memory, rigCountBuffer.mapped);
  CreateHostBuffer(kLsMaxOccluders*sizeof(LsCanvasOccluder), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "occluder",
                   occluderBuffer.buffer, occluderBuffer.memory, occluderBuffer.mapped);
  CreateHostBuffer(kLsMaxLocalLights*sizeof(LsCanvasLocalLight), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "light",
                   lightBuffer.buffer, lightBuffer.memory, lightBuffer.mapped);
  // Read by the host a frame after a pick
  // The summed normal, then the rig under the centre
  CreateHostBuffer(2*sizeof(glm::vec4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "pick",
                   pickBuffer.buffer, pickBuffer.memory, pickBuffer.mapped);
  // Cleared by the first Render, which has a command buffer to do it with
  attributesCleared = false;
  UpdateDescriptors();
  CreatePrimitiveBuffer(64);
}

LsCanvas::~LsCanvas() {
  vkDeviceWaitIdle(device);
  DestroyPrimitiveBuffer();
  DestroyAttributeImage(distance);
  DestroyAttributeImage(normal);
  DestroyAttributeImage(radiance);
  for (AttributeImage* attribute : { &materialColor, &materialSurface, &strokeDistance, &strokeNormal,
                                     &strokeMaterialColor, &strokeMaterialSurface, &strokeCoverage, &rigImage }) {
    DestroyAttributeImage(*attribute);
  }
  vkDestroyBuffer(device, rangeBuffer.buffer, nullptr);
  vkFreeMemory(device, rangeBuffer.memory, nullptr);
  vkUnmapMemory(device, parameterBuffer.memory);
  vkDestroyBuffer(device, parameterBuffer.buffer, nullptr);
  vkFreeMemory(device, parameterBuffer.memory, nullptr);
  vkUnmapMemory(device, lightBuffer.memory);
  vkDestroyBuffer(device, lightBuffer.buffer, nullptr);
  vkFreeMemory(device, lightBuffer.memory, nullptr);
  vkUnmapMemory(device, pickBuffer.memory);
  vkDestroyBuffer(device, pickBuffer.buffer, nullptr);
  vkFreeMemory(device, pickBuffer.memory, nullptr);
  DestroyReadbackBuffer();
  DestroyUploadBuffer();
  auto destroyHostBuffer = [&](VkBuffer buffer, VkDeviceMemory memory) {
    vkUnmapMemory(device, memory);
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
  };
  destroyHostBuffer(occluderBuffer.buffer, occluderBuffer.memory);
  destroyHostBuffer(rigBuffer.buffer, rigBuffer.memory);
  destroyHostBuffer(rigCountBuffer.buffer, rigCountBuffer.memory);
  vkDestroyPipeline(device, paintPipeline, nullptr);
  vkDestroyPipeline(device, blendPipeline, nullptr);
  vkDestroyPipeline(device, pickPipeline, nullptr);
  vkDestroyPipeline(device, rigCountPipeline, nullptr);
  vkDestroyPipeline(device, rangePipeline, nullptr);
  vkDestroyPipeline(device, renderPipeline, nullptr);
  vkDestroyPipeline(device, resolvePipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  vkDestroyShaderModule(device, paintShader, nullptr);
  vkDestroyShaderModule(device, blendShader, nullptr);
  vkDestroyShaderModule(device, pickShader, nullptr);
  vkDestroyShaderModule(device, rigCountShader, nullptr);
  vkDestroyShaderModule(device, rangeShader, nullptr);
  vkDestroyShaderModule(device, renderShader, nullptr);
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

LsCanvas::AttributeImage LsCanvas::CreateAttributeImage(VkFormat format, const char* name) {
  AttributeImage attribute;
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = format;
  imageInfo.extent = { width, height, 1 };
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  // Copied from at the start of a stroke and cleared by transfers
  imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(device, &imageInfo, nullptr, &attribute.image) != VK_SUCCESS) {
    throw std::string("Could not create canvas ") + name + " image!";
  }

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(device, attribute.image, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &attribute.memory) != VK_SUCCESS) {
    throw std::string("Could not allocate canvas ") + name + " image memory!";
  }
  vkBindImageMemory(device, attribute.image, attribute.memory, 0);

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = attribute.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = imageInfo.format;
  viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  if (vkCreateImageView(device, &viewInfo, nullptr, &attribute.view) != VK_SUCCESS) {
    throw std::string("Could not create canvas ") + name + " image view!";
  }
  return attribute;
}

void LsCanvas::DestroyAttributeImage(AttributeImage& attribute) {
  vkDestroyImageView(device, attribute.view, nullptr);
  vkDestroyImage(device, attribute.image, nullptr);
  vkFreeMemory(device, attribute.memory, nullptr);
  attribute = {};
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

void LsCanvas::CreateHostBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const char* name,
                                VkBuffer& buffer, VkDeviceMemory& memory, void*& mapped) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::string("Could not create canvas ") + name + " buffer!";
  }

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, buffer, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
    throw std::string("Could not allocate canvas ") + name + " buffer memory!";
  }
  vkBindBufferMemory(device, buffer, memory, 0);
  vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped);
}

void LsCanvas::CreateParameterBuffer() {
  // Written by the host every frame, after the previous frame has finished with it
  CreateHostBuffer(sizeof(LsCanvasRenderParameters), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, "parameter",
                   parameterBuffer.buffer, parameterBuffer.memory, parameterBuffer.mapped);
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
  VkDescriptorImageInfo normalInfo = { VK_NULL_HANDLE, normal.view, VK_IMAGE_LAYOUT_GENERAL };
  VkDescriptorImageInfo radianceInfo = { VK_NULL_HANDLE, radiance.view, VK_IMAGE_LAYOUT_GENERAL };
  VkDescriptorBufferInfo parameterInfo = { parameterBuffer.buffer, 0, VK_WHOLE_SIZE };
  // Bound from 7 on, in the order of the bindings in canvas.slang
  const AttributeImage* laterImages[8] = { &materialColor, &materialSurface, &strokeDistance, &strokeNormal,
                                           &strokeMaterialColor, &strokeMaterialSurface, &strokeCoverage, &rigImage };
  VkDescriptorImageInfo laterInfos[8];
  for (int i = 0; i < 8; i++) {
    laterInfos[i] = { VK_NULL_HANDLE, laterImages[i]->view, VK_IMAGE_LAYOUT_GENERAL };
  }
  VkDescriptorBufferInfo bufferInfo = { primitiveBuffer.buffer, 0, VK_WHOLE_SIZE };
  VkDescriptorBufferInfo rangeInfo = { rangeBuffer.buffer, 0, VK_WHOLE_SIZE };

  VkWriteDescriptorSet writes[20] = {};
  VkDescriptorBufferInfo occluderInfo = { occluderBuffer.buffer, 0, VK_WHOLE_SIZE };
  VkDescriptorBufferInfo rigInfo = { rigBuffer.buffer, 0, VK_WHOLE_SIZE };
  VkDescriptorBufferInfo rigCountInfo = { rigCountBuffer.buffer, 0, VK_WHOLE_SIZE };
  VkDescriptorBufferInfo pickInfo = { pickBuffer.buffer, 0, VK_WHOLE_SIZE };
  VkDescriptorBufferInfo lightInfo = { lightBuffer.buffer, 0, VK_WHOLE_SIZE };
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
  if (normal.view) {
    write(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE).pImageInfo = &normalInfo;
  }
  if (radiance.view) {
    write(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE).pImageInfo = &radianceInfo;
  }
  if (parameterBuffer.buffer) {
    write(6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER).pBufferInfo = &parameterInfo;
  }
  if (lightBuffer.buffer) {
    write(15, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).pBufferInfo = &lightInfo;
  }
  if (pickBuffer.buffer) {
    write(16, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).pBufferInfo = &pickInfo;
  }
  if (occluderBuffer.buffer) {
    write(17, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).pBufferInfo = &occluderInfo;
  }
  if (rigBuffer.buffer) {
    write(18, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).pBufferInfo = &rigInfo;
  }
  if (rigCountBuffer.buffer) {
    write(19, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).pBufferInfo = &rigCountInfo;
  }
  for (uint32_t i = 0; i < 8; i++) {
    if (laterImages[i]->view) {
      write(7 + i, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE).pImageInfo = &laterInfos[i];
    }
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
  // Emitters and shadow casters light every pixel
  radianceStale = true;
}

void LsCanvas::SetTool(const LsScenePrimitive* tool) {
  bool had = hasTool;
  hasTool = tool != nullptr;
  if (tool) {
    if (had && std::memcmp(tool, &this->tool, sizeof(*tool)) == 0) {
      return;
    }
    this->tool = *tool;
  }
  else if (!had) {
    return;
  }
  primitivesDirty = true;
  // Its intersection with the painted positions is drawn over the canvas
  resolveNeeded = true;
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

void LsCanvas::SetRenderSettings(const LsCanvasRenderSettings& settings) {
  renderSettings = settings;
  renderSettings.samplesPerFrame = std::max(renderSettings.samplesPerFrame, 1u);
  renderSettings.maxSamples = std::max(renderSettings.maxSamples, 1u);
  // The exposure only changes how the samples are shown
  resolveNeeded = true;
}

bool LsCanvas::IsRendering() const {
  return display == LsCanvasDisplay::Rendering && hasCamera && renderedSamples < renderSettings.maxSamples;
}

void LsCanvas::Paint(const std::vector<glm::vec2>& centers, const LsCanvasBrush& brush, bool startsStroke) {
  // Erasing leaves the default material
  LsBrdfParameters brdf = brush.mode == LsCanvasDabMode::Material ? brushBrdf : LsBrdfParameters();
  if (brush.mode == LsCanvasDabMode::Material && !centers.empty() &&
      std::find(paintedBrdfs.begin(), paintedBrdfs.end(), brdf) == paintedBrdfs.end()) {
    paintedBrdfs.push_back(brdf);
  }
  for (const glm::vec2& center : centers) {
    dabs.push_back({ center, brush, brdf, startsStroke });
    startsStroke = false;
  }
}

void LsCanvas::RequestNormalPick(glm::vec2 center, float radius) {
  pickRequested = true;
  pickDistanceOnly = false;
  pickCenter = center;
  pickRadius = std::max(radius, 0.5f);
}

void LsCanvas::CollectPick() {
  // The frame the pick was recorded into must have finished
  if (!pickRecorded || renderer->frameNumber <= pickFrame) {
    return;
  }
  pickRecorded = false;
  const glm::vec4* results = static_cast<const glm::vec4*>(pickBuffer.mapped);
  if (pickRecordedDistanceOnly) {
    pickedDistance = results[1].y;
    pickedDistanceReady = true;
    return;
  }
  glm::vec4 sum = results[0];
  pickedNormalReady = sum.w > 0.0f && glm::dot(glm::vec3(sum), glm::vec3(sum)) >= 1.0e-12f;
  if (pickedNormalReady) {
    glm::vec3 world = glm::normalize(glm::vec3(sum));
    pickedNormal = glm::normalize(glm::vec3(glm::dot(world, camera.GetRight()), glm::dot(world, camera.GetUp()),
                                            -glm::dot(world, camera.GetForward())));
  }
  std::memcpy(&pickedRig, &results[1].x, sizeof(pickedRig));
  pickedRigReady = true;
}

bool LsCanvas::TakePickedNormal(glm::vec3& normal) {
  CollectPick();
  if (!pickedNormalReady) {
    return false;
  }
  pickedNormalReady = false;
  normal = pickedNormal;
  return true;
}

bool LsCanvas::TakePickedRig(uint32_t& rig) {
  CollectPick();
  if (!pickedRigReady) {
    return false;
  }
  pickedRigReady = false;
  rig = GetRig(pickedRig) ? pickedRig : 0u;
  return true;
}

void LsCanvas::RequestDistancePick(glm::vec2 center) {
  pickRequested = true;
  pickDistanceOnly = true;
  pickedDistanceReady = false;
  pickCenter = center;
  pickRadius = 0.5f;
}

bool LsCanvas::TakePickedDistance(float& distance) {
  CollectPick();
  if (!pickedDistanceReady) {
    return false;
  }
  pickedDistanceReady = false;
  distance = pickedDistance;
  return true;
}

int LsCanvas::AddRig(const LsLightRig& rig) {
  for (uint32_t slot = 1; slot < kLsMaxRigs; slot++) {
    if (!rigUsed[slot]) {
      // Removing a rig took it off every pixel, so the slot starts unpainted
      rigs[slot] = rig;
      rigUsed[slot] = true;
      return static_cast<int>(slot);
    }
  }
  return -1;
}

void LsCanvas::RemoveRig(uint32_t slot) {
  if (!GetRig(slot)) {
    return;
  }
  rigUsed[slot] = false;
  radianceStale = true;
  // A light stroke over the whole canvas takes it off every pixel
  LsCanvasBrush brush;
  brush.radius = static_cast<float>(width + height);
  brush.opacity = 1.0f;
  brush.falloff = 0.0f;
  brush.mode = LsCanvasDabMode::Light;
  brush.rig = slot;
  brush.clearRig = true;
  Paint({ glm::vec2(width, height)*0.5f }, brush, true);
}

const LsLightRig* LsCanvas::GetRig(uint32_t slot) const {
  if (slot == 0 || slot >= kLsMaxRigs || !rigUsed[slot]) {
    return nullptr;
  }
  return &rigs[slot];
}

void LsCanvas::SetRig(uint32_t slot, const LsLightRig& rig) {
  if (GetRig(slot)) {
    // Only the colour and name change nothing rendered
    if (rig.lights != rigs[slot].lights || rig.occluders != rigs[slot].occluders) {
      radianceStale = true;
    }
    else {
      resolveNeeded = true;
    }
    rigs[slot] = rig;
  }
}

void LsCanvas::RequestAttributes() {
  readbackRequested = true;
}

// The images in the order they are copied into the staging buffer, each a
// 32 bit value per pixel
static const int kReadbackImages = 5;

void LsCanvas::RecordReadback(VkCommandBuffer commandBuffer) {
  VkDeviceSize imageSize = VkDeviceSize(width)*height*sizeof(uint32_t);
  if (!readbackBuffer.buffer) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize*kReadbackImages;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &readbackBuffer.buffer) != VK_SUCCESS) {
      throw std::string("Could not create canvas readback buffer!");
    }
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, readbackBuffer.buffer, &requirements);
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &allocateInfo, nullptr, &readbackBuffer.memory) != VK_SUCCESS) {
      throw std::string("Could not allocate canvas readback buffer memory!");
    }
    vkBindBufferMemory(device, readbackBuffer.buffer, readbackBuffer.memory, 0);
  }

  // After the dabs wrote the attributes
  VkMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
  const VkImage images[kReadbackImages] = { distance.image, normal.image, materialColor.image, materialSurface.image,
                                            rigImage.image };
  for (int i = 0; i < kReadbackImages; i++) {
    VkBufferImageCopy region = {};
    region.bufferOffset = imageSize*i;
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.imageExtent = { width, height, 1 };
    vkCmdCopyImageToBuffer(commandBuffer, images[i], VK_IMAGE_LAYOUT_GENERAL, readbackBuffer.buffer, 1, &region);
  }
  // Read on the host once the frame's fence has been waited on, and later
  // shader writes wait for the copies
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);
  readbackRequested = false;
  readbackRecorded = true;
  readbackFrame = renderer->frameNumber;
}

bool LsCanvas::TakeAttributes(LsCanvasAttributes& attributes) {
  // The frame the copies were recorded into must have finished
  if (!readbackRecorded || renderer->frameNumber <= readbackFrame) {
    return false;
  }
  readbackRecorded = false;
  size_t pixels = size_t(width)*height;
  attributes.width = width;
  attributes.height = height;
  void* data = nullptr;
  vkMapMemory(device, readbackBuffer.memory, 0, VK_WHOLE_SIZE, 0, &data);
  const uint8_t* mapped = static_cast<const uint8_t*>(data);
  auto copy = [&](auto& target, int image) {
    target.resize(pixels);
    std::memcpy(target.data(), mapped + pixels*sizeof(uint32_t)*image, pixels*sizeof(uint32_t));
  };
  copy(attributes.distance, 0);
  copy(attributes.normal, 1);
  copy(attributes.materialColor, 2);
  copy(attributes.materialSurface, 3);
  copy(attributes.rig, 4);
  vkUnmapMemory(device, readbackBuffer.memory);
  // Large at large canvas sizes and rarely needed, so not kept around
  DestroyReadbackBuffer();
  return true;
}

void LsCanvas::SetAttributes(const LsCanvasAttributes& attributes) {
  if (attributes.width != width || attributes.height != height) {
    return;
  }
  // A frame still holding the previous upload has finished, Render waited on it
  vkDeviceWaitIdle(device);
  DestroyUploadBuffer();
  VkDeviceSize imageSize = VkDeviceSize(width)*height*sizeof(uint32_t);
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = imageSize*kReadbackImages;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(device, &bufferInfo, nullptr, &uploadBuffer.buffer) != VK_SUCCESS) {
    throw std::string("Could not create canvas upload buffer!");
  }
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, uploadBuffer.buffer, &requirements);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (vkAllocateMemory(device, &allocateInfo, nullptr, &uploadBuffer.memory) != VK_SUCCESS) {
    throw std::string("Could not allocate canvas upload buffer memory!");
  }
  vkBindBufferMemory(device, uploadBuffer.buffer, uploadBuffer.memory, 0);
  void* data = nullptr;
  vkMapMemory(device, uploadBuffer.memory, 0, VK_WHOLE_SIZE, 0, &data);
  uint8_t* mapped = static_cast<uint8_t*>(data);
  // In the order RecordReadback copies them
  const void* images[kReadbackImages] = { attributes.distance.data(), attributes.normal.data(),
                                          attributes.materialColor.data(), attributes.materialSurface.data(),
                                          attributes.rig.data() };
  for (int i = 0; i < kReadbackImages; i++) {
    std::memcpy(mapped + imageSize*i, images[i], static_cast<size_t>(imageSize));
  }
  vkUnmapMemory(device, uploadBuffer.memory);
  uploadPending = true;
  uploadRecorded = false;
}

void LsCanvas::RecordUpload(VkCommandBuffer commandBuffer) {
  VkDeviceSize imageSize = VkDeviceSize(width)*height*sizeof(uint32_t);
  // After whatever last read or wrote the attributes
  VkMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
  const VkImage images[kReadbackImages] = { distance.image, normal.image, materialColor.image, materialSurface.image,
                                            rigImage.image };
  for (int i = 0; i < kReadbackImages; i++) {
    VkBufferImageCopy region = {};
    region.bufferOffset = imageSize*i;
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.imageExtent = { width, height, 1 };
    vkCmdCopyBufferToImage(commandBuffer, uploadBuffer.buffer, images[i], VK_IMAGE_LAYOUT_GENERAL, 1, &region);
  }
  // Before the dabs, the rendering and the resolve read them
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);
  uploadPending = false;
  uploadRecorded = true;
  uploadFrame = renderer->frameNumber;
  // Everything shown and rendered starts over from the new attributes
  radianceStale = true;
  resolveNeeded = true;
  rigCountRequested = true;
}

void LsCanvas::DestroyUploadBuffer() {
  if (uploadBuffer.buffer) {
    vkDestroyBuffer(device, uploadBuffer.buffer, nullptr);
  }
  if (uploadBuffer.memory) {
    vkFreeMemory(device, uploadBuffer.memory, nullptr);
  }
  uploadBuffer = {};
}

void LsCanvas::ClearLighting() {
  localLightSlots = 0;
  occluderSlots = 0;
  rigUsed.fill(false);
  rigUsed[0] = true;
  radianceStale = true;
}

void LsCanvas::SetLocalLightAt(uint32_t slot, const LsLocalLight& light) {
  if (slot < kLsMaxLocalLights) {
    localLights[slot] = light;
    localLightSlots |= 1u << slot;
    radianceStale = true;
  }
}

void LsCanvas::SetOccluderAt(uint32_t slot, const LsOccluder& occluder) {
  if (slot < kLsMaxOccluders) {
    occluders[slot] = occluder;
    occluderSlots |= 1u << slot;
    radianceStale = true;
  }
}

void LsCanvas::SetRigAt(uint32_t slot, const LsLightRig& rig) {
  if (slot > 0 && slot < kLsMaxRigs) {
    rigs[slot] = rig;
    rigUsed[slot] = true;
    radianceStale = true;
  }
}

void LsCanvas::DestroyReadbackBuffer() {
  if (readbackBuffer.buffer) {
    vkDestroyBuffer(device, readbackBuffer.buffer, nullptr);
  }
  if (readbackBuffer.memory) {
    vkFreeMemory(device, readbackBuffer.memory, nullptr);
  }
  readbackBuffer = {};
}

void LsCanvas::CollectRigCounts() {
  // The frame the count was recorded into must have finished
  if (!rigCountRecorded || renderer->frameNumber <= rigCountFrame) {
    return;
  }
  rigCountRecorded = false;
  std::memcpy(rigPixelCounts.data(), rigCountBuffer.mapped, sizeof(uint32_t)*kLsMaxRigs);
  paintedPixelCount = 0;
  for (uint32_t count : rigPixelCounts) {
    paintedPixelCount += count;
  }
}

int LsCanvas::AddLocalLight(const LsLocalLight& light) {
  for (uint32_t slot = 0; slot < kLsMaxLocalLights; slot++) {
    if ((localLightSlots & (1u << slot)) == 0) {
      // Removing a light took it out of every rig, so the slot starts in none
      localLights[slot] = light;
      localLightSlots |= 1u << slot;
      radianceStale = true;
      return static_cast<int>(slot);
    }
  }
  return -1;
}

void LsCanvas::RemoveLocalLight(uint32_t slot) {
  if (slot >= kLsMaxLocalLights || (localLightSlots & (1u << slot)) == 0) {
    return;
  }
  localLightSlots &= ~(1u << slot);
  radianceStale = true;
  // Out of every rig, so a light later added to the slot isn't in them
  for (LsLightRig& rig : rigs) {
    rig.lights &= ~(1u << slot);
  }
}

const LsLocalLight* LsCanvas::GetLocalLight(uint32_t slot) const {
  if (slot >= kLsMaxLocalLights || (localLightSlots & (1u << slot)) == 0) {
    return nullptr;
  }
  return &localLights[slot];
}

void LsCanvas::SetLocalLight(uint32_t slot, const LsLocalLight& light) {
  if (GetLocalLight(slot)) {
    localLights[slot] = light;
    radianceStale = true;
  }
}

int LsCanvas::AddOccluder(const LsOccluder& occluder) {
  for (uint32_t slot = 0; slot < kLsMaxOccluders; slot++) {
    if ((occluderSlots & (1u << slot)) == 0) {
      // Removing an occluder took it out of every rig, so the slot starts in none
      occluders[slot] = occluder;
      occluderSlots |= 1u << slot;
      radianceStale = true;
      return static_cast<int>(slot);
    }
  }
  return -1;
}

void LsCanvas::RemoveOccluder(uint32_t slot) {
  if (!GetOccluder(slot)) {
    return;
  }
  occluderSlots &= ~(1u << slot);
  radianceStale = true;
  // Out of every rig, so an occluder later added to the slot isn't in them
  for (LsLightRig& rig : rigs) {
    rig.occluders &= ~(1u << slot);
  }
}

const LsOccluder* LsCanvas::GetOccluder(uint32_t slot) const {
  if (slot >= kLsMaxOccluders || (occluderSlots & (1u << slot)) == 0) {
    return nullptr;
  }
  return &occluders[slot];
}

void LsCanvas::SetOccluder(uint32_t slot, const LsOccluder& occluder) {
  if (GetOccluder(slot)) {
    occluders[slot] = occluder;
    radianceStale = true;
  }
}

glm::vec3 LsCanvas::ViewToWorld(glm::vec3 direction) const {
  return glm::normalize(camera.GetRight()*direction.x + camera.GetUp()*direction.y - camera.GetForward()*direction.z);
}

// Directions follow the camera, so this is rewritten every Render
void LsCanvas::UpdateLightBuffer() {
  LsCanvasLocalLight* mapped = static_cast<LsCanvasLocalLight*>(lightBuffer.mapped);
  for (uint32_t slot = 0; slot < kLsMaxLocalLights; slot++) {
    LsCanvasLocalLight gpu = {};
    if (localLightSlots & (1u << slot)) {
      const LsLocalLight& light = localLights[slot];
      float radius = glm::radians(glm::clamp(light.size, 0.1f, 45.0f));
      float sinRadius = std::sin(radius);
      float solidAngle = 2.0f*glm::pi<float>()*(1.0f - std::cos(radius));
      // A disk of radiance L and angular radius r gives irradiance L pi sin^2 r on a surface facing it
      glm::vec3 radiance = light.color*light.intensity/(glm::pi<float>()*sinRadius*sinRadius);
      gpu.direction = glm::vec4(hasCamera ? ViewToWorld(light.direction) : light.direction, std::cos(radius));
      gpu.radiance = glm::vec4(radiance, solidAngle);
      gpu.shadowed = light.shadows ? 1u : 0u;
    }
    mapped[slot] = gpu;
  }
  LsCanvasOccluder* mappedOccluders = static_cast<LsCanvasOccluder*>(occluderBuffer.mapped);
  for (uint32_t slot = 0; slot < kLsMaxOccluders; slot++) {
    LsCanvasOccluder gpu = {};
    if (occluderSlots & (1u << slot)) {
      const LsOccluder& occluder = occluders[slot];
      float radius = glm::radians(glm::clamp(occluder.size, 0.5f, 90.0f));
      float inner = radius*(1.0f - glm::clamp(occluder.softness, 0.0f, 1.0f));
      gpu.direction = glm::vec4(hasCamera ? ViewToWorld(occluder.direction) : occluder.direction, std::cos(radius));
      gpu.shape = glm::vec4(std::cos(inner), glm::clamp(occluder.strength, 0.0f, 1.0f), 0.0f, 0.0f);
    }
    mappedOccluders[slot] = gpu;
  }
  LsCanvasRig* mappedRigs = static_cast<LsCanvasRig*>(rigBuffer.mapped);
  for (uint32_t slot = 0; slot < kLsMaxRigs; slot++) {
    LsCanvasRig gpu = {};
    if (slot != 0 && rigUsed[slot]) {
      const LsLightRig& rig = rigs[slot];
      glm::uvec3 color = glm::uvec3(glm::round(glm::clamp(rig.color, 0.0f, 1.0f)*255.0f));
      gpu.lights = rig.lights & localLightSlots;
      gpu.occluders = rig.occluders & occluderSlots;
      gpu.color = color.r | (color.g << 8) | (color.b << 16) | (255u << 24);
    }
    mappedRigs[slot] = gpu;
  }
}

void LsCanvas::RecordBlendCopy(VkCommandBuffer commandBuffer, glm::ivec2 first, glm::ivec2 last) {
  VkMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);

  const std::pair<const AttributeImage*, const AttributeImage*> copies[4] = {
    { &strokeDistance, &distance }, { &strokeNormal, &normal },
    { &strokeMaterialColor, &materialColor }, { &strokeMaterialSurface, &materialSurface },
  };
  VkImageCopy region = {};
  region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
  region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
  region.srcOffset = { first.x, first.y, 0 };
  region.dstOffset = { first.x, first.y, 0 };
  region.extent = { static_cast<uint32_t>(last.x - first.x + 1), static_cast<uint32_t>(last.y - first.y + 1), 1 };
  for (const auto& copy : copies) {
    vkCmdCopyImage(commandBuffer, copy.first->image, VK_IMAGE_LAYOUT_GENERAL, copy.second->image, VK_IMAGE_LAYOUT_GENERAL,
                   1, &region);
  }

  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void LsCanvas::RecordStrokeStart(VkCommandBuffer commandBuffer) {
  // After the previous dabs wrote the attributes, and the previous stroke read the copies
  VkMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);

  const std::pair<const AttributeImage*, const AttributeImage*> copies[4] = {
    { &distance, &strokeDistance }, { &normal, &strokeNormal },
    { &materialColor, &strokeMaterialColor }, { &materialSurface, &strokeMaterialSurface },
  };
  VkImageCopy region = {};
  region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
  region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
  region.extent = { width, height, 1 };
  for (const auto& copy : copies) {
    vkCmdCopyImage(commandBuffer, copy.first->image, VK_IMAGE_LAYOUT_GENERAL, copy.second->image, VK_IMAGE_LAYOUT_GENERAL,
                   1, &region);
  }
  VkClearColorValue zero = {};
  VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  vkCmdClearColorImage(commandBuffer, strokeCoverage.image, VK_IMAGE_LAYOUT_GENERAL, &zero, 1, &range);

  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void LsCanvas::Render() {
  // The tool goes after the scene's primitives
  size_t primitiveCount = primitives.size() + (hasTool ? 1 : 0);
  if (primitiveCount > primitiveBuffer.capacity) {
    vkDeviceWaitIdle(device);
    DestroyPrimitiveBuffer();
    CreatePrimitiveBuffer(primitiveCount*2);
  }
  if (primitivesDirty) {
    // Host writes before the submission are visible to it without a barrier.
    // The previous frame has finished reading the buffer, its fence was waited on.
    if (!primitives.empty()) {
      std::memcpy(primitiveBuffer.mapped, primitives.data(), primitives.size()*sizeof(LsScenePrimitive));
    }
    if (hasTool) {
      std::memcpy(static_cast<LsScenePrimitive*>(primitiveBuffer.mapped) + primitives.size(), &tool, sizeof(tool));
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
  constants.primitiveCount = static_cast<uint32_t>(primitiveCount);
  constants.display = static_cast<uint32_t>(display);
  constants.isolineSpacing = isolines ? isolineSpacing : 0.0f;

  // Positions follow the camera, so moving it changes what the canvas shows
  const glm::vec4 frame[4] = { constants.origin, constants.right, constants.up, constants.forward };
  if (hasCamera && std::memcmp(frame, resolvedFrame, sizeof(frame)) != 0) {
    resolveNeeded = true;
    radianceStale = true;
  }
  if (radianceStale) {
    renderedSamples = 0;
  }
  if (!dabs.empty()) {
    // The pixels the dabs change start over, others skip the samples they have
    renderedSamples = 0;
  }
  bool render = hasCamera && display == LsCanvasDisplay::Rendering && renderedSamples < renderSettings.maxSamples;
  bool resolve = hasCamera && (resolveNeeded || !dabs.empty() || render);
  if (!hasCamera) {
    // Nothing to reconstruct normals with
    pickRequested = false;
  }
  CollectRigCounts();
  if (!dabs.empty()) {
    rigCountRequested = true;
  }
  // The upload is freed once the frame that copied it has finished
  if (uploadRecorded && renderer->frameNumber > uploadFrame) {
    DestroyUploadBuffer();
    uploadRecorded = false;
  }
  if (attributesCleared && dabs.empty() && !resolve && !pickRequested && !rigCountRequested &&
      !(readbackRequested && !readbackRecorded) && !uploadPending) {
    return;
  }

  LsCanvasRenderParameters parameters = {};
  parameters.exposure = std::exp2(renderSettings.exposure);
  parameters.samplesPerFrame = renderSettings.samplesPerFrame;
  parameters.maxSamples = renderSettings.maxSamples;
  parameters.localLightSlots = localLightSlots;
  parameters.occluderSlots = occluderSlots;
  UpdateLightBuffer();
  // The previous frame has finished reading it, the frame fence was waited on
  std::memcpy(parameterBuffer.mapped, &parameters, sizeof(parameters));

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

  if (!attributesCleared) {
    // No pixel has a position or a normal yet and has the default material.
    // Pixels without a position show black, also without a canvas camera to
    // resolve the canvas image with.
    VkClearColorValue zero = {};
    VkClearColorValue noNormal = {};
    noNormal.uint32[0] = kNoNormal;
    VkClearColorValue black = {};
    black.float32[3] = 1.0f;
    uint32_t defaultMaterial[2];
    PackBrdf(LsBrdfParameters(), defaultMaterial);
    VkClearColorValue defaultColor = {};
    defaultColor.uint32[0] = defaultMaterial[0];
    VkClearColorValue defaultSurface = {};
    defaultSurface.uint32[0] = defaultMaterial[1];
    // The stroke images are written at the start of each stroke
    const std::pair<VkImage, VkClearColorValue> clears[] = {
      { distance.image, zero }, { renderer->canvasState.image, black }, { normal.image, noNormal },
      { radiance.image, zero }, { materialColor.image, defaultColor }, { materialSurface.image, defaultSurface },
      { strokeDistance.image, zero }, { strokeNormal.image, noNormal }, { strokeMaterialColor.image, defaultColor },
      { strokeMaterialSurface.image, defaultSurface }, { strokeCoverage.image, zero }, { rigImage.image, zero },
    };
    const uint32_t clearCount = sizeof(clears)/sizeof(clears[0]);
    VkImageMemoryBarrier clearBarriers[clearCount];
    for (uint32_t c = 0; c < clearCount; c++) {
      clearBarriers[c] = distanceBarrier;
      clearBarriers[c].image = clears[c].first;
      clearBarriers[c].srcAccessMask = 0;
      clearBarriers[c].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      clearBarriers[c].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      clearBarriers[c].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, clearCount, clearBarriers);
    for (const auto& clear : clears) {
      vkCmdClearColorImage(commandBuffer, clear.first, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear.second, 1,
                           &distanceBarrier.subresourceRange);
    }
    for (VkImageMemoryBarrier& barrier : clearBarriers) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    }
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, clearCount, clearBarriers);
    attributesCleared = true;
    radianceStale = false;
  }
  else if (radianceStale && render) {
    // Every pixel starts over. Cleared only when rendering, until then the
    // stale samples aren't looked at.
    VkImageMemoryBarrier radianceBarrier = distanceBarrier;
    radianceBarrier.image = radiance.image;
    radianceBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    radianceBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    radianceBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    radianceBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &radianceBarrier);
    VkClearColorValue zero = {};
    vkCmdClearColorImage(commandBuffer, radiance.image, VK_IMAGE_LAYOUT_GENERAL, &zero, 1, &radianceBarrier.subresourceRange);
    radianceBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    radianceBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &radianceBarrier);
    radianceStale = false;
  }

  // After the images were first cleared, which puts them in the general layout
  if (uploadPending) {
    RecordUpload(commandBuffer);
  }

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  // Each dab reads and writes the attributes after the previous one, so they composite in order
  VkMemoryBarrier computeBarrier = {};
  computeBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  computeBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  computeBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  VkPipeline boundPipeline = VK_NULL_HANDLE;
  bool dispatched = false;
  for (const Dab& dab : dabs) {
    // Blend strokes don't blend from the start of the stroke
    if (dab.startsStroke && !dab.brush.blend) {
      // Its barriers order it after the previous dabs and before the next one
      RecordStrokeStart(commandBuffer);
      dispatched = false;
    }
    // Only the pixels whose centres the dab can cover
    float radius = dab.brush.radius;
    glm::ivec2 first = glm::max(glm::ivec2(glm::floor(dab.center - radius - 0.5f)), glm::ivec2(0));
    glm::ivec2 last = glm::min(glm::ivec2(glm::ceil(dab.center + radius - 0.5f)),
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
    constants.dabRadius = radius;
    constants.dabMode = static_cast<uint32_t>(dab.brush.mode) | (dab.brush.blend ? kDabBlend : 0u) |
                        (dab.brush.mode == LsCanvasDabMode::Sculpt && dab.brush.push ? kDabPush : 0u) |
                        (dab.brush.mode == LsCanvasDabMode::Sculpt && dab.brush.alongNormal ? kDabAlongNormal : 0u);
    PackBrdf(dab.brdf, constants.dabMaterial);
    if (dab.brush.mode == LsCanvasDabMode::Light) {
      constants.dabMaterial[0] = dab.brush.rig;
      constants.dabMode |= dab.brush.clearRig ? kDabClearRig : 0u;
      if (dab.brush.fixedNormal && !dab.brush.clearRig) {
        // The rig's normal in world space for the camera as the dab is painted
        constants.dabMode |= kDabFixedNormal;
        constants.dabMaterial[1] = EncodeNormal(ViewToWorld(dab.brush.normal));
      }
    }
    else if (dab.brush.mode == LsCanvasDabMode::Normal && dab.brush.fixedNormal && !dab.brush.blend) {
      // The normal in world space for the camera as the dab is painted
      constants.dabMode |= kDabFixedNormal;
      constants.dabMaterial[0] = EncodeNormal(ViewToWorld(dab.brush.normal));
    }
    if (dab.brush.blend || dab.brush.mode == LsCanvasDabMode::Depth) {
      // Blend and depth dabs paint no material, the shader reads the threshold
      // from its place: depth dabs switch to positions on another surface
      // rather than blending to them
      float threshold = std::max(dab.brush.blendEdgeThreshold, 0.0f);
      std::memcpy(&constants.dabMaterial[0], &threshold, sizeof(threshold));
    }
    constants.dabOpacity = glm::clamp(dab.brush.opacity, 0.0f, 1.0f);
    constants.dabFalloff = glm::clamp(dab.brush.falloff, 0.0f, 1.0f);
    VkPipeline pipeline = dab.brush.blend ? blendPipeline : paintPipeline;
    if (pipeline != boundPipeline) {
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
      boundPipeline = pipeline;
    }
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    glm::uvec2 extent = glm::uvec2(last - first) + 1u;
    vkCmdDispatch(commandBuffer, (extent.x + kWorkGroupSize - 1)/kWorkGroupSize, (extent.y + kWorkGroupSize - 1)/kWorkGroupSize, 1);
    dispatched = true;
    if (dab.brush.blend) {
      // Its barriers order it after this dab and before the next one
      RecordBlendCopy(commandBuffer, first, last);
      dispatched = false;
    }
  }
  dabs.clear();

  if (pickRequested) {
    // After the dabs, so it sees this frame's strokes
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 1, &computeBarrier, 0, nullptr, 0, nullptr);
    constants.dabCenter = pickCenter;
    constants.dabRadius = pickRadius;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pickPipeline);
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, 1, 1, 1);
    // Read on the host once the frame's fence has been waited on
    VkMemoryBarrier hostBarrier = {};
    hostBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    hostBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    hostBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0, 1, &hostBarrier, 0, nullptr, 0, nullptr);
    pickRequested = false;
    pickRecorded = true;
    pickRecordedDistanceOnly = pickDistanceOnly;
    pickFrame = renderer->frameNumber;
  }

  if (readbackRequested && !readbackRecorded) {
    RecordReadback(commandBuffer);
  }

  // Counts what the strokes changed, once the last count has been read back
  if (rigCountRequested && !rigCountRecorded) {
    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);
    vkCmdFillBuffer(commandBuffer, rigCountBuffer.buffer, 0, VK_WHOLE_SIZE, 0u);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);
    // After the dabs, which wrote the rigs and positions it counts
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 1, &computeBarrier, 0, nullptr, 0, nullptr);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, rigCountPipeline);
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, (width + kWorkGroupSize - 1)/kWorkGroupSize, (height + kWorkGroupSize - 1)/kWorkGroupSize, 1);
    VkMemoryBarrier hostBarrier = {};
    hostBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    hostBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    hostBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0, 1, &hostBarrier, 0, nullptr, 0, nullptr);
    rigCountRequested = false;
    rigCountRecorded = true;
    rigCountFrame = renderer->frameNumber;
  }

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

    if (render) {
      // After the dabs, which the computeBarrier above waits for. Resolved below
      // after another one.
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, renderPipeline);
      vkCmdDispatch(commandBuffer, (width + kWorkGroupSize - 1)/kWorkGroupSize, (height + kWorkGroupSize - 1)/kWorkGroupSize, 1);
      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 1, &computeBarrier, 0, nullptr, 0, nullptr);
      renderedSamples = std::min(renderedSamples + renderSettings.samplesPerFrame, renderSettings.maxSamples);
    }

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
