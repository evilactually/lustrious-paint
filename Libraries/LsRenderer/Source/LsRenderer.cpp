#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <assert.h>
#include <array>
#include <cstdio>
#include <vulkan_dynamic.hpp>

#include <destructor.h>
#include <LsVulkanLoader.h>
#include <LsError.h>
#include <LsVulkanInstance.h>
#include <LsVulkanDevice.h>
#include <LsVulkanPresentation.h>
#include <LsPushConstantTypes.h>
#include <LsVulkanRendering.h>
#include <LsVulkanCommandBuffers.h>
#include <LsRenderer.h>
#include <LsUiTheme.h>
#include <SDL3/SDL_vulkan.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#ifdef GIF_RECORDING
#include <FreeImage.h>
#endif

using namespace lslib;

// TODO: GIF_RECORDING feature is leaking resources, needs to get fixed

//-------------------------------------------------------------------------------
// @ requiredInstanceExtensions
//-------------------------------------------------------------------------------
// List of instance-level extension that will be enabled when creating an instance
//-------------------------------------------------------------------------------
static std::vector<const char*> GetRequiredInstanceExtensions() {
  // SDL knows which surface extensions the current video driver needs
  Uint32 sdlExtensionCount = 0;
  const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
  if ( !sdlExtensions ) {
    throw std::string("Could not query Vulkan instance extensions: ") + SDL_GetError();
  }

  std::vector<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);
#ifdef VULKAN_VALIDATION
  extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
  return extensions;
}

//-------------------------------------------------------------------------------
// @ requiredDeviceExtensions
//-------------------------------------------------------------------------------
// List of device-level extension that will be enabled when creating a logical 
// device
//-------------------------------------------------------------------------------
std::vector<const char*> requiredDeviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//-------------------------------------------------------------------------------
// @ requiredInstanceValidationLayers
//-------------------------------------------------------------------------------
// List of instance-level validation levels that will be enabled when creating an
// instance
//-------------------------------------------------------------------------------
std::vector<const char*> requiredInstanceValidationLayers = {
#ifdef VULKAN_VALIDATION
  "VK_LAYER_KHRONOS_validation"
#endif
};

// Needs device, swapchain, graphics queue

//LsRenderer::LsRenderer() {
//
//}

// LsRenderer LsRenderer::renderer;

//LsRenderer* LsRenderer::Get() {
//  return &renderer;
//}

#ifdef GIF_RECORDING
void LsRenderer::CreateCapturedFrameImage() {
  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.flags = 0;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageCreateInfo.extent = { swapChainInfo.extent.width, swapChainInfo.extent.height, 1 };
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
  imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.queueFamilyIndexCount = 0;
  imageCreateInfo.pQueueFamilyIndices = nullptr;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkMemoryRequirements memoryRequirements;
  if (vkCreateImage(device, &imageCreateInfo, nullptr, &captureInfo.capturedFrameImage.image) != VK_SUCCESS) {
    throw std::string("Could not create image!");
  }
  
  vkGetImageMemoryRequirements(device, captureInfo.capturedFrameImage.image, &memoryRequirements);
  // memory requirements give required flags only, not exact type index, we have to find that

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  int32_t memoryType = FindMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VkMemoryAllocateInfo memoryAllocationInfo = {};
  memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocationInfo.allocationSize = memoryRequirements.size;
  memoryAllocationInfo.memoryTypeIndex = memoryType;
  
  vkAllocateMemory(device, &memoryAllocationInfo, nullptr, &captureInfo.capturedFrameImage.memory);
  vkBindImageMemory(device, captureInfo.capturedFrameImage.image, captureInfo.capturedFrameImage.memory, 0);
  // frame memory size used later for writing to file, and later it doubles as frame pitch for reading
  captureInfo.capturedFrameImage.size = memoryRequirements.size;
}

void LsRenderer::RecordFrameCaptureCmds() {
  VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
  cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  cmd_buffer_begin_info.pInheritanceInfo = nullptr;
  
  vkBeginCommandBuffer( captureInfo.captureFrameCmds, &cmd_buffer_begin_info );
  
  VkImageSubresourceRange imageSubresourceRange;
  imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageSubresourceRange.baseMipLevel = 0;
  imageSubresourceRange.levelCount = 1;
  imageSubresourceRange.baseArrayLayer = 0;
  imageSubresourceRange.layerCount = 1;

  // transition swap chain image into transfer source layout
  CmdImageBarrier(captureInfo.captureFrameCmds, // cmdBuffer
    VK_ACCESS_MEMORY_READ_BIT,                  // srcAccessMask
    VK_ACCESS_TRANSFER_READ_BIT,                // dstAccessMask
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // oldLayout
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,       // newLayout
    VK_QUEUE_FAMILY_IGNORED,                    // srcQueueFamilyIndex
    VK_QUEUE_FAMILY_IGNORED,                    // dstQueueFamilyIndex                             
    swapChainInfo.images[swapChainInfo.acquiredImageIndex], // image
    imageSubresourceRange,                      // subresourceRange
    0,                                          // srcStageMask, wait for nothing
    VK_PIPELINE_STAGE_TRANSFER_BIT,             // dstStageMask, block transfer
    0);                                         // dependencyFlags

  // transition captureInfo.capturedFrameImage.image to transfer destination layout
  CmdImageBarrier(captureInfo.captureFrameCmds,  // cmdBuffer
                  0,                             // srcAccessMask
                  VK_ACCESS_TRANSFER_WRITE_BIT,  // dstAccessMask
                  VK_IMAGE_LAYOUT_UNDEFINED,     // oldLayout
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newLayout
                  VK_QUEUE_FAMILY_IGNORED,
                  VK_QUEUE_FAMILY_IGNORED,
                  captureInfo.capturedFrameImage.image,
                  imageSubresourceRange,
                  0,                              // wait for nothing
                  VK_PIPELINE_STAGE_TRANSFER_BIT, // block transfer
                  0);

  VkImageSubresourceLayers subresourceLayers = {
    VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags                     aspectMask
    0,                         // uint32_t                               mipLevel
    0,                         // uint32_t                               baseArrayLayer
    1                          // uint32_t                               layerCount
  };

  VkImageCopy region = {
    subresourceLayers,
    { 0,0,0 },
    subresourceLayers,
    { 0,0,0 },
    { swapChainInfo.extent.width, swapChainInfo.extent.height, 1 }
  };

  vkCmdCopyImage(captureInfo.captureFrameCmds,
                 swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 captureInfo.capturedFrameImage.image,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1,
                 &region);

  // transition captureInfo.capturedFrameImage.image to host read layout
  CmdImageBarrier(captureInfo.captureFrameCmds,
                  VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_ACCESS_HOST_READ_BIT,        // image memory will be mapped for reading later
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_GENERAL,        // layout that supports host access
                  VK_QUEUE_FAMILY_IGNORED,        // srcQueueFamilyIndex
                  VK_QUEUE_FAMILY_IGNORED,        // dstQueueFamilyIndex
                  captureInfo.capturedFrameImage.image,
                  imageSubresourceRange,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, // wait for transfer
                  VK_PIPELINE_STAGE_HOST_BIT,     // guarantee visibility of writes to host
                  0);

  // transition swap chain image back to presentation layout
  CmdImageBarrier(captureInfo.captureFrameCmds,
                  VK_ACCESS_TRANSFER_READ_BIT,
                  VK_ACCESS_MEMORY_READ_BIT,
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                  VK_QUEUE_FAMILY_IGNORED,
                  VK_QUEUE_FAMILY_IGNORED,
                  swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                  imageSubresourceRange,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, // wait for transfer
                  0,                              // block nothing, because nothing comes next
                  0);
  vkEndCommandBuffer(captureInfo.captureFrameCmds);
}

void LsRenderer::StartGIFRecording(std::string filename) {
  std::cout << "Recording started" << std::endl;
  OpenBufferFile();
  CreateCapturedFrameImage();
  RecordFrameCaptureCmds();
  // frame row pitch is used for decoding frame memory
  VkImageSubresource imageSubresource = {
    VK_IMAGE_ASPECT_COLOR_BIT,
    0,
    0
  };

  VkSubresourceLayout subresourceLayout;
  vkGetImageSubresourceLayout(device, 
                              captureInfo.capturedFrameImage.image,
                              &imageSubresource,
                              &subresourceLayout);

  captureInfo.capturedFrameImage.rowPitch = subresourceLayout.rowPitch;
  // capture frame extent
  captureInfo.capturedFrameImage.extent = swapChainInfo.extent;
  //RecordFrameCaptureCmds();
  captureInfo.capturing = true;
  captureInfo.captureFrameCount = 0;
  captureInfo.startTime = std::chrono::high_resolution_clock::now();
  captureInfo.filename = filename;
}

void LsRenderer::StopGIFRecording() {
  std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
  BYTE* captureData = (BYTE*)malloc(captureInfo.capturedFrameImage.size);
  DWORD bytesRead;
  std::cout << "Recording stopped" << std::endl;
  SetFilePointer(captureInfo.hCaptureBufferFile,// hFile
    0,                                          // lDistanceToMove
    0,                                          // lpDistanceToMoveHigh
    FILE_BEGIN);                                // dwMoveMethod
  char fileName[1024];
  //BOOL dialogResult = ShowSaveAsDialog(fileName, sizeof(fileName), "*.gif", "gif");
  //if (!dialogResult) {
  //  free(captureData);
  //  CloseHandle(captureInfo.hCaptureBufferFile);
  //  captureInfo.capturing = false;
  //  return;
  //}
  std::cout << "Saving to " << captureInfo.filename << std::endl;
  FIMULTIBITMAP* multibitmap = FreeImage_OpenMultiBitmap(
    FIF_GIF,
    captureInfo.filename.c_str(),
    TRUE,
    FALSE);
  if (!multibitmap)
  {
    throw std::string("FreeImage_OpenMultiBitmap failed to create gif file!");
  }
  double captureDuration = std::chrono::duration<double, std::milli>(endTime - captureInfo.startTime).count() / 1000.0;
  double fps = 30.0f;//((double)captureInfo.captureFrameCount)/captureDuration;
  DWORD frameTimeMs = (DWORD)((1000.0 / fps) + 0.5);
  //DWORD frameTimeMs = (DWORD)( ((double)captureInfo.captureFrameCount)/captureDuration + 0.5);
  std::cout << ((double)captureInfo.captureFrameCount)/captureDuration << std::endl;

  FITAG* tag = FreeImage_CreateTag();
  FreeImage_SetTagKey(tag, "FrameTime");
  FreeImage_SetTagType(tag, FIDT_LONG);
  FreeImage_SetTagCount(tag, 1);
  FreeImage_SetTagLength(tag, 4);
  FreeImage_SetTagValue(tag, &frameTimeMs);
  while (captureInfo.captureFrameCount) {
    // read next captured frame
    BOOL readResult = ReadFile(captureInfo.hCaptureBufferFile,
      captureData,
      captureInfo.capturedFrameImage.size,
      &bytesRead,
      nullptr);
    if (!readResult || (bytesRead != captureInfo.capturedFrameImage.size)) {
      std::cout << "ReadFile failed:" << GetLastError() << std::endl;
      throw std::string("ReadFile failed");
    }
    FIBITMAP* bitmap = FreeImage_Allocate(captureInfo.capturedFrameImage.extent.width,
      captureInfo.capturedFrameImage.extent.height,
      24);
    RGBQUAD color;
    for (int y = 0; y < captureInfo.capturedFrameImage.extent.height; ++y)
    {
      for (int x = 0; x < captureInfo.capturedFrameImage.extent.width; ++x)
      {
        BYTE* pixel = captureData + captureInfo.capturedFrameImage.rowPitch*y + 4 * x;
        color.rgbRed = *(pixel + 2);
        color.rgbGreen = *(pixel + 1);
        color.rgbBlue = *pixel;
        FreeImage_SetPixelColor(bitmap, x, captureInfo.capturedFrameImage.extent.height - y, &color);
      }
    }
    FIBITMAP* quantized = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);
    // Reset animation metadata
    FreeImage_SetMetadata(FIMD_ANIMATION, quantized, NULL, NULL);
    FreeImage_SetMetadata(FIMD_ANIMATION, quantized, FreeImage_GetTagKey(tag), tag);
    //FreeImage_Save(FIF_PNG, bitmap, (std::to_string(captureInfo.captureFrameCount)+std::string(".png")).c_str(), 0);
    FreeImage_AppendPage(multibitmap, quantized);
    if (quantized) {
      FreeImage_Unload(quantized);
    }
    FreeImage_Unload(bitmap);
    captureInfo.captureFrameCount--;
    //captureData += captureInfo.capturedFrameImage.size;
  }
  FreeImage_CloseMultiBitmap(multibitmap);
  free(captureData);
  //UnmapViewOfFile(captureData);
  //CloseHandle(hMapping);
  CloseHandle(captureInfo.hCaptureBufferFile);
  captureInfo.capturing = false;
}

void LsRenderer::OpenBufferFile() {
  TCHAR tmpPath[MAX_PATH];
  TCHAR tmpFilePath[MAX_PATH];
  DWORD result = GetTempPath(MAX_PATH, tmpPath);
  if ((result > MAX_PATH) || (result == 0)) {
    throw std::string("GetTempPath failed!");
  }
  std::cout << "Using temporary path:" << tmpPath << std::endl;
  result = GetTempFileName(tmpPath, TEXT("ls"), 0, tmpFilePath);
  std::cout << "Buffer temporary file" << tmpFilePath << std::endl;
  if (!result) {
    throw std::string("GetTempFileName failed!");
  }

  captureInfo.hCaptureBufferFile = CreateFile(tmpFilePath,
    GENERIC_WRITE | GENERIC_READ,
    0,
    NULL,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
    NULL);
  if (captureInfo.hCaptureBufferFile == INVALID_HANDLE_VALUE) {
    throw std::string("Failed to create a file!");
  }
}

bool LsRenderer::CheckFrameSavingFinished() {
  if (captureInfo.hSavingThread) {
    DWORD result = WaitForSingleObject(captureInfo.hSavingThread, 0); // no wait
    return !result; // it's 0 when finished
  }
  else {
    return true; // no thread was started
  }
}

DWORD WINAPI WriteFrameThread(LPVOID lpParam) {
  LsRenderer* renderer = (LsRenderer*)lpParam;
  void* data;
  DWORD written;
  vkMapMemory(renderer->device, renderer->captureInfo.capturedFrameImage.memory, 0, renderer->captureInfo.capturedFrameImage.size, 0, &data);
  DWORD result = WriteFile(renderer->captureInfo.hCaptureBufferFile, data, renderer->captureInfo.capturedFrameImage.size, &written, nullptr);
  vkUnmapMemory(renderer->device, renderer->captureInfo.capturedFrameImage.memory);
  //TODO: What if write fails?
  return 0;
}

void LsRenderer::BeginSavingCapturedFrame() {
  captureInfo.captureFrameCount++;
  captureInfo.hSavingThread = CreateThread(NULL,
    0,
    WriteFrameThread,
    this, // pass instance of LsRenderer
    0,
    NULL);
}
#endif

LsRenderer::LsRenderer(SDL_Window* window):window(window) {
  //LsRenderer* renderer = LsRenderer::Get();
  
#ifdef GIF_RECORDING
  FreeImage_Initialise();
#endif

  LsLoadVulkanLibrary();

  LsLoadExportedEntryPoints();
  LsLoadGlobalLevelEntryPoints();

  std::vector<const char*> requiredInstanceExtensions = GetRequiredInstanceExtensions();
  CreateInstance(requiredInstanceExtensions, requiredInstanceValidationLayers, &instance);
 
  LsLoadInstanceLevelEntryPoints(instance, requiredInstanceExtensions);

#ifdef VULKAN_VALIDATION
  CreateDebugReportCallback(instance, &debugReportCallback);
#endif
  
  CreatePresentationSurface(instance, window, &swapChainInfo.presentationSurface);

  CreateDevice(instance, 
               swapChainInfo.presentationSurface,
               requiredDeviceExtensions,
               &physicalDevice,
               &device,
               &graphicsQueue.familyIndex,
               &presentQueue.familyIndex,
               &canvasState.queueFamilyIndex);

  LsLoadDeviceLevelEntryPoints(device, requiredDeviceExtensions);

  commandPool = CreateCommandPool(device, graphicsQueue.familyIndex);
  commandBuffer = CreateCommandBuffer(device, commandPool);
#ifdef GIF_RECORDING
  captureInfo.captureFrameCmds = CreateCommandBuffer(device, commandPool);
#endif
 
  CreateSemaphore(device, &semaphores.imageAvailable);
  CreateSemaphore(device, &semaphores.renderingFinished);
#ifdef GIF_RECORDING
  CreateSemaphore(device, &captureInfo.captureFinishedSemaphore);
#endif

  shaderModules.lineVertexShader = CreateShaderModule(device, "Shaders/line.vert.spv");
  shaderModules.lineFragmentShader = CreateShaderModule(device, "Shaders/line.frag.spv");
  shaderModules.pointVertexShader = CreateShaderModule(device, "Shaders/point.vert.spv");
  shaderModules.pointFragmentShader = CreateShaderModule(device, "Shaders/point.frag.spv");
  shaderModules.imageVertexShader = CreateShaderModule(device, "Shaders/image.vert.spv");
  shaderModules.imageFragmentShader = CreateShaderModule(device, "Shaders/image.frag.spv");
  
  CreateFence(device, &submitCompleteFence, true);
#ifdef GIF_RECORDING
  CreateFence(device, &captureInfo.captureCompleteFence, false);
#endif

  vkGetDeviceQueue( device, graphicsQueue.familyIndex, 0, &graphicsQueue.handle );
  vkGetDeviceQueue( device, presentQueue.familyIndex, 0, &presentQueue.handle );

  if ( CreateSwapChain(physicalDevice,
                       device,
                       swapChainInfo.presentationSurface,
                       window,
                       &swapChainInfo.swapChain,
                       &swapChainInfo.format,
                       &swapChainInfo.extent) ) {

    swapChainInfo.images = GetSwapChainImages(device, swapChainInfo.swapChain);

    swapChainInfo.imageViews = CreateSwapChainImageViews(device,
                                                                   swapChainInfo.images,
                                                                   swapChainInfo.format);

    renderPass = CreateSimpleRenderPass(device, swapChainInfo.format);

    framebuffers = CreateFramebuffers(device, 
                                                swapChainInfo.imageViews,
                                                swapChainInfo.extent,
                                                renderPass);

    linePipelineLayout = CreatePipelineLayout(device, sizeof(LinePushConstants));
    pointPipelineLayout = CreatePipelineLayout(device, sizeof(PointPushConstants));
    //imagePipelineLayout = CreatePipelineLayout(device, sizeof(ImagePushConstants));

    // WARNING MESSS !! WARNING MESSS !! WARNING MESSS !! WARNING MESSS !! 
    InitializeCanvas(640, 640); // needs to be divisible by 16(work group size)
    
    // Create vkDescriptorSetLayout, needed by Allocate and CreatePipeline
    VkDescriptorSetLayoutBinding setLayoutBinding {};
    setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    setLayoutBinding.binding = 0;
    setLayoutBinding.descriptorCount = 1;

    VkDescriptorSetLayoutCreateInfo descriptorLayout {};
    descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pBindings = &setLayoutBinding;
    descriptorLayout.bindingCount = 1;

    VkDescriptorSetLayout imageDescriptorSetLayout; // make a member?

    vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &imageDescriptorSetLayout);

    // Descriptor pool
    //VkDescriptorPool descriptorPool;
    //VkDescriptorPoolSize descriptorPoolSize {};
    //descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // need to change image layout
    //descriptorPoolSize.descriptorCount = 1;

    //VkDescriptorPoolCreateInfo descriptorPoolInfo {};
    //descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    //descriptorPoolInfo.poolSizeCount = 1;
    //descriptorPoolInfo.pPoolSizes = &descriptorPoolSize;
    //descriptorPoolInfo.maxSets = 1;
    //
    //vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);

    //VkDescriptorSetLayout imageDescriptorSetLayout;

    // Allocate descriptor sets
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.pSetLayouts = &imageDescriptorSetLayout; // make a member?
    descriptorSetAllocateInfo.descriptorSetCount = 1;

    //VkDescriptorSet imageDescriptorSet; // make member?

    vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &imageDescriptorSet);

    VkWriteDescriptorSet writeDescriptorSet {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = imageDescriptorSet;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.pImageInfo = &canvasState.descriptor; // keep same, that has sample, image and imageview
    writeDescriptorSet.descriptorCount = 1;
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);

    // Create Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &imageDescriptorSetLayout;


    VkPushConstantRange pushConstantRange = {
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(ImagePushConstants)
    };

    pipelineLayoutCreateInfo.flags = 0;
    //pipelineLayoutCreateInfo.setLayoutCount = 0;
    //pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &imagePipelineLayout);

   // WARNING MESSS !! WARNING MESSS !! WARNING MESSS !! WARNING MESSS !! 

    CreatePrimitivePipelines(device,
                             shaderModules.lineVertexShader,
                             shaderModules.lineFragmentShader,
                             shaderModules.pointVertexShader,
                             shaderModules.pointFragmentShader,
                             shaderModules.imageVertexShader,
                             shaderModules.imageFragmentShader,
                             linePipelineLayout,
                             pointPipelineLayout,
                             imagePipelineLayout,
                             renderPass,
                             swapChainInfo.extent,
                             &linePipeline,
                             &pointPipeline,
                             &imagePipeline);
    
    canRender = true;

    InitializeUi();
  }

//  InitializeCanvas(640, 800); // needs to be divisible by 16(work group size)
}

void LsRenderer::RefreshSwapChain() {
  canRender = false;

  if ( device ) {
    vkDeviceWaitIdle(device);
  
    if ( linePipeline )
    {
      vkDestroyPipeline( device, linePipeline, nullptr);
    linePipeline = VK_NULL_HANDLE;
    }

    if ( pointPipeline )
    {
      vkDestroyPipeline(device, pointPipeline, nullptr);
    pointPipeline = VK_NULL_HANDLE;
    }

    if ( linePipelineLayout ) {
      vkDestroyPipelineLayout(device, linePipelineLayout, nullptr);
    linePipelineLayout = VK_NULL_HANDLE;
    }

    if ( pointPipelineLayout ) {
      vkDestroyPipelineLayout(device, pointPipelineLayout, nullptr);
    pointPipelineLayout = VK_NULL_HANDLE;
    }

    for ( const VkFramebuffer& framebuffer:framebuffers ) {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    framebuffers.clear();

    if ( renderPass ) {
      vkDestroyRenderPass(device, renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
    }

    for( auto &imageView: swapChainInfo.imageViews) {
      vkDestroyImageView(device, imageView, nullptr);
    }
    swapChainInfo.imageViews.clear();

    // We don't have to destroy the swap chain here, it will be 
    // destroyed by a call to CreateSwapChain
  }

  if ( CreateSwapChain(physicalDevice,
                       device,
                       swapChainInfo.presentationSurface,
                       window,
                       &swapChainInfo.swapChain,
                       &swapChainInfo.format,
                       &swapChainInfo.extent) ) {
    
    swapChainInfo.images = GetSwapChainImages(device, swapChainInfo.swapChain);

    swapChainInfo.imageViews = CreateSwapChainImageViews(device,
                                                         swapChainInfo.images,
                                                         swapChainInfo.format);

    renderPass = CreateSimpleRenderPass(device, swapChainInfo.format);

    framebuffers = CreateFramebuffers(device, 
                                      swapChainInfo.imageViews,
                                      swapChainInfo.extent,
                                      renderPass);

    linePipelineLayout = CreatePipelineLayout(device, sizeof(LinePushConstants));
    pointPipelineLayout = CreatePipelineLayout(device, sizeof(PointPushConstants));

    CreatePrimitivePipelines(device,
                             shaderModules.lineVertexShader,
                             shaderModules.lineFragmentShader,
                             shaderModules.pointVertexShader,
                             shaderModules.pointFragmentShader,
                             shaderModules.imageVertexShader,
                             shaderModules.imageFragmentShader,
                             linePipelineLayout,
                             pointPipelineLayout,
                             imagePipelineLayout,
                             renderPass,
                             swapChainInfo.extent,
                             &linePipeline,
                             &pointPipeline,
                             &imagePipeline);

  windowToVulkanTransformation = WindowToVulkanTransformation(swapChainInfo.extent);

    canRender = true;
  } else {
    std::cout << "Swap chain not ready, cannot render." << std::endl;
  }
}

int LsRenderer::GetSurfaceWidth()
{
  return swapChainInfo.extent.width;
}

int LsRenderer::GetSurfaceHeight()
{
  return swapChainInfo.extent.height;
}

LsRenderer::~LsRenderer() {
  canRender = false;

  if ( device ) {
    vkDeviceWaitIdle(device);

    ShutdownUi();
  
    if ( linePipeline )
    {
      vkDestroyPipeline( device, linePipeline, nullptr );
    }

    if ( pointPipeline )
    {
      vkDestroyPipeline( device, pointPipeline, nullptr );
    }

    if ( linePipelineLayout ) {
      vkDestroyPipelineLayout( device, linePipelineLayout, nullptr );
    }

    if ( pointPipelineLayout ) {
    vkDestroyPipelineLayout( device, pointPipelineLayout, nullptr );
    }

    for ( const VkFramebuffer& framebuffer:framebuffers ) {
      vkDestroyFramebuffer( device, framebuffer, nullptr );
    }

    if ( renderPass )
      vkDestroyRenderPass( device, renderPass, nullptr );

    for( auto &imageView: swapChainInfo.imageViews) {
      vkDestroyImageView( device, imageView, nullptr );
    }

    if ( swapChainInfo.swapChain )
       vkDestroySwapchainKHR( device, swapChainInfo.swapChain, nullptr );

    if ( submitCompleteFence )
      vkDestroyFence( device, submitCompleteFence, nullptr );

    if ( shaderModules.lineVertexShader )
      vkDestroyShaderModule( device, shaderModules.lineVertexShader, nullptr );
    if ( shaderModules.lineFragmentShader )
      vkDestroyShaderModule( device, shaderModules.lineFragmentShader, nullptr );
    if ( shaderModules.pointVertexShader )
      vkDestroyShaderModule( device, shaderModules.pointVertexShader, nullptr );
    if ( shaderModules.pointFragmentShader )
      vkDestroyShaderModule( device, shaderModules.pointFragmentShader, nullptr );

    if ( semaphores.renderingFinished )
      vkDestroySemaphore( device, semaphores.renderingFinished, nullptr );

    if ( semaphores.imageAvailable )
      vkDestroySemaphore( device, semaphores.imageAvailable, nullptr );

    if( commandPool ) {
      vkDestroyCommandPool( device, commandPool, nullptr );
    commandPool = VK_NULL_HANDLE;
    }

  vkDestroyDevice(device, nullptr);
  }

  if ( swapChainInfo.presentationSurface )
    vkDestroySurfaceKHR( instance, swapChainInfo.presentationSurface, nullptr );

#ifdef VULKAN_VALIDATION
  if ( debugReportCallback )
    vkDestroyDebugReportCallbackEXT( instance, debugReportCallback, nullptr );
#endif // VULKAN_VALIDATION

  if (instance)
    vkDestroyInstance( instance, nullptr );

  LsUnloadVulkanLibrary();
}

void LsRenderer::BeginFrame() {
  BeginUiFrame();

  if( vkWaitForFences( device, 1, &submitCompleteFence, VK_FALSE, 1000000000 ) != VK_SUCCESS ) {
    throw std::string("Waiting for fence takes too long!");
  }

  vkResetFences( device, 1, &submitCompleteFence );
  frameNumber++;

  VkResult result = vkAcquireNextImageKHR( device,
                                           swapChainInfo.swapChain, 
                                           UINT64_MAX,
                                           semaphores.imageAvailable,
                                           VK_NULL_HANDLE,
                                           &swapChainInfo.acquiredImageIndex );

  switch( result ) {
    case VK_SUCCESS:
    break;
    case VK_SUBOPTIMAL_KHR:
    // It's still OK to use.
    break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    RefreshSwapChain();
    return;
    default:
    throw std::string( "Problem occurred during swap chain image acquisition!" );
  }

  VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
  cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  vkBeginCommandBuffer( commandBuffer, &cmd_buffer_begin_info );

  VkImageSubresourceRange image_subresource_range = {
  VK_IMAGE_ASPECT_COLOR_BIT,                      // VkImageAspectFlags                     aspectMask
  0,                                              // uint32_t                               baseMipLevel
  1,                                              // uint32_t                               levelCount
  0,                                              // uint32_t                               baseArrayLayer
  1                                               // uint32_t                               layerCount
  };

  // Transition to presentation layout and tell vulkan that we are discarding previous contents of the image
  // block reads from present(atachment output), blocks draws and clears
  VkImageMemoryBarrier barrier_from_present_to_draw = {}; 
  barrier_from_present_to_draw.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_from_present_to_draw.srcAccessMask = 0;
  barrier_from_present_to_draw.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier_from_present_to_draw.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier_from_present_to_draw.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier_from_present_to_draw.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_present_to_draw.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_present_to_draw.image = swapChainInfo.images[swapChainInfo.acquiredImageIndex];
  barrier_from_present_to_draw.subresourceRange = image_subresource_range;

  vkCmdPipelineBarrier( commandBuffer, 
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,//VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_present_to_draw );
}

void LsRenderer::EndFrame() {
#ifdef GIF_RECORDING
  BOOL captureInitiated = false;
#endif
  RenderUi();

  if( drawingContext.drawing ) {
    EndDrawing();
  }

  if( !screenshot.path.empty() ) {
    RecordScreenshotCopy();
  }

  if( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS ) {
    throw std::string("Could not record command buffers!");
  }

  // stall these stages until image is available from swap chain
  VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                             VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &semaphores.imageAvailable;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &commandBuffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &semaphores.renderingFinished;
  submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
  
  if( vkQueueSubmit( graphicsQueue.handle, 1, &submit_info, submitCompleteFence ) != VK_SUCCESS ) {
    throw std::string("Submit to queue failed!");
  }

  VkSemaphore presentBlockingSemaphore = semaphores.renderingFinished;

#ifdef GIF_RECORDING
  if (captureInfo.capturing) {
    // capture next frame only if previous capture already finished
    if (CheckFrameSavingFinished()) {
      std::cout << captureInfo.captureFrameCount << std::endl;
      // stall transfer stage until rendering is finished 
      wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

      submit_info.waitSemaphoreCount = 1;
      submit_info.pWaitSemaphores = &semaphores.renderingFinished;
      submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &captureInfo.captureFrameCmds;
      submit_info.signalSemaphoreCount = 1;
      submit_info.pSignalSemaphores = &captureInfo.captureFinishedSemaphore;

      vkResetFences( device, 1, &captureInfo.captureCompleteFence );
      if (vkQueueSubmit( graphicsQueue.handle, 1, &submit_info, captureInfo.captureCompleteFence) != VK_SUCCESS ) {
        throw std::string("Submit to queue failed!");
      }
      // make present wait for capture instead of just render
      presentBlockingSemaphore = captureInfo.captureFinishedSemaphore;
      // resume presentation as soon as possible, using this flag to resume capture
      captureInitiated = true;
    }
    else {
      std::cout << "Frame skipped, still writing!" << std::endl;
    }
  }
#endif

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &presentBlockingSemaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapChainInfo.swapChain;
  present_info.pImageIndices = &swapChainInfo.acquiredImageIndex;
  present_info.pResults = nullptr;
  
  VkResult result = vkQueuePresentKHR(presentQueue.handle, &present_info );

  if( screenshot.recorded ) {
    WriteScreenshot();
  }

  switch( result ) {
    case VK_SUCCESS:
    break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
    RefreshSwapChain();
    return;
    default:
    throw std::string("Problem occurred during image presentation!");
  }

#ifdef GIF_RECORDING
  if (captureInitiated) {
    // wait for capture submission to finish
    if (vkWaitForFences(device, 1, &captureInfo.captureCompleteFence, VK_FALSE, 1000000000) != VK_SUCCESS) {
      throw std::string("Waiting for fence takes too long!");
    }
    // async copy frame to file
    BeginSavingCapturedFrame();
  }
#endif
}

void LsRenderer::Clear(float r, float g, float b) {
  if( drawingContext.drawing ) {
    EndDrawing();
  }

  VkImageSubresourceRange image_subresource_range = {
  VK_IMAGE_ASPECT_COLOR_BIT,                      // VkImageAspectFlags                     aspectMask
  0,                                              // uint32_t                               baseMipLevel
  1,                                              // uint32_t                               levelCount
  0,                                              // uint32_t                               baseArrayLayer
  1                                               // uint32_t                               layerCount
  };

  VkImageMemoryBarrier barrier_from_present_to_clear = {};
  barrier_from_present_to_clear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_from_present_to_clear.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier_from_present_to_clear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier_from_present_to_clear.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier_from_present_to_clear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier_from_present_to_clear.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_present_to_clear.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_present_to_clear.image = swapChainInfo.images[swapChainInfo.acquiredImageIndex];
  barrier_from_present_to_clear.subresourceRange = image_subresource_range;

  vkCmdPipelineBarrier( commandBuffer, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_present_to_clear );

  VkClearColorValue clear_color = {};
  clear_color.float32[0] = r;
  clear_color.float32[1] = g;
  clear_color.float32[2] = b;
  clear_color.float32[3] = 1.0f;

  vkCmdClearColorImage( commandBuffer,
                      swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      &clear_color,
                      1,
                      &image_subresource_range );

  VkImageMemoryBarrier barrier_from_clear_to_present = {};
  barrier_from_clear_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_from_clear_to_present.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // VK_ACCESS_MEMORY_READ_BIT fails validation
  barrier_from_clear_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier_from_clear_to_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier_from_clear_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier_from_clear_to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_clear_to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_clear_to_present.image = swapChainInfo.images[swapChainInfo.acquiredImageIndex];
  barrier_from_clear_to_present.subresourceRange = image_subresource_range;

  vkCmdPipelineBarrier( commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_clear_to_present );

}

LsImage LsRenderer::CreateImage(uint32_t width, uint32_t height) {
  return LsImage(width, height, *this);
}

void LsRenderer::DrawImage(glm::vec2 p1, glm::vec2 p2, LsImage image) {

}

void LsRenderer::DrawCanvas() {
  if( !drawingContext.drawing ) {
    BeginDrawing();
  }

  if(drawingContext.pipelineBinding != PipelineBinding::eImage) {
    // Bind image graphics pipeline
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, imagePipelineLayout, 0, 1, &imageDescriptorSet, 0, NULL);
    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, imagePipeline );
    drawingContext.pipelineBinding = PipelineBinding::eImage; 
  }

  // ********
  //Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
  // VkImageMemoryBarrier imageMemoryBarrier = {};
  // imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  // // We won't be changing the layout of the image
  // imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  // imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  // imageMemoryBarrier.image = canvasState.image;
  // imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  // imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  // imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  // vkCmdPipelineBarrier(
  //   commandBuffer,
  //   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
  //   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
  //   0, // VkDependencyFlagBits
  //   0, nullptr,
  //   0, nullptr,
  //   1, &imageMemoryBarrier);
  // ********

  // Define a quad covering the canvas as placed by the view
  glm::vec2 topLeft = canvasView.offset;
  glm::vec2 bottomRight = canvasView.offset + GetCanvasSize()*canvasView.zoom;

  glm::vec3 vulkanPoint1 = windowToVulkanTransformation*glm::vec3(topLeft.x, topLeft.y, 1.0f);
  glm::vec3 vulkanPoint2 = windowToVulkanTransformation*glm::vec3(bottomRight.x, topLeft.y, 1.0f);
  glm::vec3 vulkanPoint3 = windowToVulkanTransformation*glm::vec3(bottomRight.x, bottomRight.y, 1.0f);

  glm::vec3 vulkanPoint4 = windowToVulkanTransformation*glm::vec3(topLeft.x, topLeft.y, 1.0f);
  glm::vec3 vulkanPoint5 = windowToVulkanTransformation*glm::vec3(bottomRight.x, bottomRight.y, 1.0f);
  glm::vec3 vulkanPoint6 = windowToVulkanTransformation*glm::vec3(topLeft.x, bottomRight.y, 1.0f);

  ImagePushConstants pushConstants;
  pushConstants.positions[0] = vulkanPoint1[0];
  pushConstants.positions[1] = vulkanPoint1[1];

  pushConstants.positions[2] = vulkanPoint2[0];
  pushConstants.positions[3] = vulkanPoint2[1];

  pushConstants.positions[4] = vulkanPoint3[0];
  pushConstants.positions[5] = vulkanPoint3[1];

  pushConstants.positions[6] = vulkanPoint4[0];
  pushConstants.positions[7] = vulkanPoint4[1];

  pushConstants.positions[8] = vulkanPoint5[0];
  pushConstants.positions[9] = vulkanPoint5[1];

  pushConstants.positions[10] = vulkanPoint6[0];
  pushConstants.positions[11] = vulkanPoint6[1];

  vkCmdPushConstants( commandBuffer, 
                      imagePipelineLayout,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, // offset
                      sizeof(ImagePushConstants),
                      &pushConstants );
  vkCmdDraw( commandBuffer, 12, 1, 0, 0 );
}

void LsRenderer::SetCanvasView(glm::vec2 offset, float zoom) {
  canvasView.offset = offset;
  canvasView.zoom = zoom;
}

ImTextureID LsRenderer::GetCanvasTexture() {
  if ( !canvasUiTexture && uiInitialized ) {
    // Sampled where the compute shader paints it, in the general layout
    canvasUiTexture = ImGui_ImplVulkan_AddTexture(canvasState.sampler, canvasState.view, VK_IMAGE_LAYOUT_GENERAL);
  }
  return reinterpret_cast<ImTextureID>(canvasUiTexture);
}

glm::vec2 LsRenderer::GetCanvasSize() {
  return glm::vec2(static_cast<float>(canvasState.width), static_cast<float>(canvasState.height));
}

void LsRenderer::DrawLine(float x1, float y1, float x2, float y2) {
  if( !drawingContext.drawing ) {
    BeginDrawing();
  }

  if(drawingContext.pipelineBinding != PipelineBinding::eLine) {
    // Bind line graphics pipeline
  vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline );
    drawingContext.pipelineBinding = PipelineBinding::eLine; 
  }

  glm::vec3 vulkanPoint1 = windowToVulkanTransformation*glm::vec3(x1, y1, 1.0f);
  glm::vec3 vulkanPoint2 = windowToVulkanTransformation*glm::vec3(x2, y2, 1.0f);

  // Transition image layout from generic read/present
  LinePushConstants pushConstants;
  pushConstants.positions[0] = vulkanPoint1[0];
  pushConstants.positions[1] = vulkanPoint1[1];
  pushConstants.positions[2] = vulkanPoint2[0];
  pushConstants.positions[3] = vulkanPoint2[1];
  std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));

  vkCmdPushConstants( commandBuffer, 
                    linePipelineLayout,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, // offset
                      sizeof(LinePushConstants),
                      &pushConstants );
  vkCmdSetLineWidth( commandBuffer, drawingContext.lineWidth );
  vkCmdDraw( commandBuffer, 2, 1, 0, 0 );
}

void LsRenderer::DrawLine(glm::vec2 p1, glm::vec2 p2)
{
  DrawLine(p1[0], p1[1], p2[0], p2[1]);
}

void LsRenderer::DrawPoint(float x, float y) {
  if( !drawingContext.drawing ) {
    BeginDrawing();
  }

  if(drawingContext.pipelineBinding != PipelineBinding::ePoint) {
    // Bind line graphics pipeline
  vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipeline );
    drawingContext.pipelineBinding = PipelineBinding::ePoint;
  }

  glm::vec3 vulkanPoint = windowToVulkanTransformation*glm::vec3(x, y, 1.0f);

  // Transition image layout from generic read/present
  // TODO: Do I need to push it every time?
  PointPushConstants pushConstants;
  pushConstants.positions[0] = vulkanPoint[0];
  pushConstants.positions[1] = vulkanPoint[1];
  pushConstants.size = drawingContext.pointSize;
  std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));

  vkCmdPushConstants( commandBuffer, 
                    pointPipelineLayout,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, // offset
                      sizeof(PointPushConstants),
                      &pushConstants );
  vkCmdDraw( commandBuffer, 1, 1, 0, 0 );
}

void DrawImage(glm::vec2 p1, glm::vec2 p2, LsImage image) {
  
}

void LsRenderer::SetColor(float r, float g, float b) {
  drawingContext.color[0] = r;
  drawingContext.color[1] = g;
  drawingContext.color[2] = b;
}

void LsRenderer::SetLineWidth(float width) {
  drawingContext.lineWidth = width;
}

void LsRenderer::SetPointSize(float size) {
  drawingContext.pointSize = size;
}

// Find and create a compute capable device queue
void LsRenderer::getComputeQueue()
{
  //uint32_t queueFamilyCount;
  //vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
  //assert(queueFamilyCount >= 1);

  //std::vector<VkQueueFamilyProperties> queueFamilyProperties;
  //queueFamilyProperties.resize(queueFamilyCount);
  //vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

  //  // Some devices have dedicated compute queues, so we first try to find a queue that supports compute and not graphics
  //bool computeQueueFound = false;
  //for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
  //{
  //  if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
  //  {
  //    canvasState.queueFamilyIndex = i;
  //    computeQueueFound = true;
  //    break;
  //  }
  //}

  //// If there is no dedicated compute queue, just find the first queue family that supports compute
  //if (!computeQueueFound)
  //{
  //  for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
  //  {
  //    if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
  //    {
  //      canvasState.queueFamilyIndex = i;
  //      computeQueueFound = true;
  //      break;
  //    }
  //  }
  //} 

  //  // Compute is mandatory in Vulkan, so there must be at least one queue family that supports compute
  //assert(computeQueueFound);
  //  // Get a compute queue from the device
  //
  //(device, canvasState.queueFamilyIndex, 0, &canvasState.queue);
}

void LsRenderer::InitializeCanvas(uint32_t width, uint32_t height) {
  this->canvasState.width = width;
  this->canvasState.height = height;
  this->canvasState.format = VK_FORMAT_R8G8B8A8_UNORM;

  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.flags = 0;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = this->canvasState.format;
  imageCreateInfo.extent = { width, height, 1 };
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.queueFamilyIndexCount = 0;
  imageCreateInfo.pQueueFamilyIndices = nullptr;
  // LsCanvas clears it and moves it to the general layout on its first frame
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkMemoryRequirements memoryRequirements;
  if (vkCreateImage(device, &imageCreateInfo, nullptr, &canvasState.image) != VK_SUCCESS) {
      throw std::string("Could not create image!");
  }

  vkGetImageMemoryRequirements(device, canvasState.image, &memoryRequirements);
  // memory requirements give required flags only, not exact type index, we have to find that

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  int32_t memoryType = FindMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  
  VkMemoryAllocateInfo memoryAllocationInfo = {};
  memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocationInfo.allocationSize = memoryRequirements.size;
  memoryAllocationInfo.memoryTypeIndex = memoryType;

  vkAllocateMemory(device, &memoryAllocationInfo, nullptr, &canvasState.memory);
  vkBindImageMemory(device, canvasState.image, canvasState.memory, 0);
  // frame memory size used later for writing to file, and later it doubles as frame pitch for reading
  canvasState.size = memoryRequirements.size;

  // Create sampler
  VkSamplerCreateInfo samplerCreateInfo {};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.maxAnisotropy = 1.0f;
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  // Clamp to the edge texels, a border colour would bleed into the canvas edge through linear filtering
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
  samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
  samplerCreateInfo.mipLodBias = 0.0f;
  samplerCreateInfo.maxAnisotropy = 1.0f;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
  samplerCreateInfo.minLod = 0.0f;
  samplerCreateInfo.maxLod = 1;
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  vkCreateSampler(device, &samplerCreateInfo, nullptr, &canvasState.sampler);

  // Create image view
  VkImageViewCreateInfo imageViewCreateInfo {};
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCreateInfo.format = canvasState.format;
  imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
  imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  imageViewCreateInfo.image = canvasState.image;
  vkCreateImageView(device, &imageViewCreateInfo, nullptr, &canvasState.view);

  // Set image layout 
  canvasState.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  canvasState.descriptor.sampler = canvasState.sampler;
  canvasState.descriptor.imageView = canvasState.view;
  canvasState.descriptor.imageLayout = canvasState.imageLayout;

  // Descriptor pool for drawing the canvas, LsCanvas paints it with its own descriptors
  VkDescriptorPoolSize descriptorPoolSize {};
  descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorPoolSize.descriptorCount = 1;

  VkDescriptorPoolCreateInfo descriptorPoolInfo {};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.poolSizeCount = 1;
  descriptorPoolInfo.pPoolSizes = &descriptorPoolSize;
  descriptorPoolInfo.maxSets = 1;

  vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
}

void LsRenderer::BeginDrawing() {
  assert(!drawingContext.drawing);

  // If queue present and graphics queue families are not the same
  // transfer ownership to graphics queue
  VkImageSubresourceRange image_subresource_range = {
  VK_IMAGE_ASPECT_COLOR_BIT,       // VkImageAspectFlags                     aspectMask
  0,                               // uint32_t                               baseMipLevel
  1,                               // uint32_t                               levelCount
  0,                               // uint32_t                               baseArrayLayer
  1                                // uint32_t                               layerCount
  };

  // wait for writes from clears and draws, block draws
  VkImageMemoryBarrier barrier_from_present_to_draw = {
  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
  nullptr,
  0,                                                      // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                   // VkAccessFlags            dstAccessMask
  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                        // VkImageLayout            oldLayout
  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                        // VkImageLayout            newLayout
  presentQueue.familyIndex,                               // uint32_t                 srcQueueFamilyIndex
  graphicsQueue.familyIndex,                              // uint32_t                 dstQueueFamilyIndex
  swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
  image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
  };

  vkCmdPipelineBarrier( commandBuffer,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_present_to_draw );

  // Begin rendering pass
  VkRenderPassBeginInfo render_pass_begin_info = {
    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
  nullptr,
    renderPass,                                     // VkRenderPass                   renderPass
    framebuffers[swapChainInfo.acquiredImageIndex], // VkFramebuffer                  framebuffer
    {                                               // VkRect2D                       renderArea
      {                                             // VkOffset2D                     offset
        0,                                          // int32_t                        x
        0                                           // int32_t                        y
      },
      {                                             // VkExtent2D                     extent
        swapChainInfo.extent.width,                 // int32_t                        width
        swapChainInfo.extent.height,                // int32_t                        height
      }
    },
    0,                                              // uint32_t                       clearValueCount
    nullptr                                         // const VkClearValue            *pClearValues
  };

  vkCmdBeginRenderPass( commandBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );

  // Tell drawing functions that command buffer is already prepared for drawing
  drawingContext.drawing = true;

  drawingContext.pipelineBinding = PipelineBinding::eNone;
}

void LsRenderer::EndDrawing() {
  vkCmdEndRenderPass(commandBuffer);
  
  VkImageSubresourceRange image_subresource_range = {
  VK_IMAGE_ASPECT_COLOR_BIT,       // VkImageAspectFlags                     aspectMask
  0,                               // uint32_t                               baseMipLevel
  1,                               // uint32_t                               levelCount
  0,                               // uint32_t                               baseArrayLayer
  1                                // uint32_t                               layerCount
  };

  VkImageMemoryBarrier barrier_from_present_to_draw = {
  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
  nullptr,
  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                   // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
  VK_ACCESS_MEMORY_READ_BIT,                              // VkAccessFlags            dstAccessMask
  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                        // VkImageLayout            oldLayout
  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                        // VkImageLayout            newLayout
  graphicsQueue.familyIndex,                              // uint32_t                 srcQueueFamilyIndex
  presentQueue.familyIndex,                               // uint32_t                 dstQueueFamilyIndex
  swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
  image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
  };

  vkCmdPipelineBarrier( commandBuffer, 
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // need to block presentation, there's no actual stage for it,
                        0,                                    // but bottom of pipe is what we need
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_present_to_draw );

  drawingContext.drawing = false;
}

void LsRenderer::InitializeUi() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  LsApplyBlenderTheme();
  LsLoadUiFont("Fonts/DejaVuSans.ttf", 15.0f);

  // Scale the UI for high DPI displays
  float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  if ( mainScale > 0.0f ) {
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(mainScale);
    style.FontScaleDpi = mainScale;
  }

  ImGui_ImplSDL3_InitForVulkan(window);

  // The backend is built without Vulkan prototypes, so hand it the same loader the renderer uses.
  // The instance does not request an API version, which makes it Vulkan 1.0.
  bool functionsLoaded = ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_0,
    [](const char* functionName, void* userData) {
      return vkGetInstanceProcAddr(*static_cast<VkInstance*>(userData), functionName);
    },
    &instance);
  if ( !functionsLoaded ) {
    throw std::string("Could not load Vulkan functions for Dear ImGui!");
  }

  uint32_t imageCount = static_cast<uint32_t>(swapChainInfo.images.size());

  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.ApiVersion = VK_API_VERSION_1_0;
  initInfo.Instance = instance;
  initInfo.PhysicalDevice = physicalDevice;
  initInfo.Device = device;
  initInfo.QueueFamily = graphicsQueue.familyIndex;
  initInfo.Queue = graphicsQueue.handle;
  initInfo.DescriptorPoolSize = 64; // backend owns its pool, sized with room for textures shown in the UI
  initInfo.MinImageCount = 2;
  initInfo.ImageCount = imageCount < 2 ? 2 : imageCount;
  // UI is drawn inside the same render pass as lines and points; recreated render passes
  // in RefreshSwapChain keep the same format, so they stay compatible with this pipeline
  initInfo.PipelineInfoMain.RenderPass = renderPass;
  initInfo.PipelineInfoMain.Subpass = 0;
  initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  if ( !ImGui_ImplVulkan_Init(&initInfo) ) {
    throw std::string("Could not initialize Dear ImGui Vulkan backend!");
  }

  uiInitialized = true;
}

void LsRenderer::ShutdownUi() {
  if ( !uiInitialized ) {
    return;
  }

  if ( canvasUiTexture ) {
    ImGui_ImplVulkan_RemoveTexture(canvasUiTexture);
    canvasUiTexture = VK_NULL_HANDLE;
  }
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  uiInitialized = false;
}

void LsRenderer::BeginUiFrame() {
  if ( !uiInitialized ) {
    return;
  }

  // Let ImGui set the system cursor only while it is over the UI,
  // elsewhere the application decides whether the cursor is visible
  ImGuiIO& io = ImGui::GetIO();
  if ( io.WantCaptureMouse ) {
    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
  } else {
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
  }

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void LsRenderer::RenderUi() {
  if ( !uiInitialized ) {
    return;
  }

  ImGui::Render();
  ImDrawData* drawData = ImGui::GetDrawData();
  if ( drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f ) {
    return;
  }

  if ( !drawingContext.drawing ) {
    BeginDrawing();
  }

  ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

  // ImGui bound its own pipeline
  drawingContext.pipelineBinding = PipelineBinding::eNone;
}

bool LsRenderer::ProcessUiEvent(const SDL_Event& event) {
  if ( !uiInitialized ) {
    return false;
  }

  ImGui_ImplSDL3_ProcessEvent(&event);

  ImGuiIO& io = ImGui::GetIO();
  switch ( event.type ) {
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_PEN_MOTION:
    case SDL_EVENT_PEN_DOWN:
    case SDL_EVENT_PEN_BUTTON_DOWN:
    case SDL_EVENT_PEN_AXIS:
    return io.WantCaptureMouse;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_TEXT_INPUT:
    return io.WantCaptureKeyboard;
    default:
    // Releases always pass through, so a stroke or key held outside the UI can't get stuck
    return false;
  }
}

void LsRenderer::SaveScreenshot(const std::string& path) {
  screenshot.path = path;
}

void LsRenderer::RecordScreenshotCopy() {
  VkExtent2D extent = swapChainInfo.extent;
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = VkDeviceSize(extent.width)*extent.height*4;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if( vkCreateBuffer(device, &bufferInfo, nullptr, &screenshot.buffer) != VK_SUCCESS ) {
    screenshot.path.clear();
    return;
  }
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, screenshot.buffer, &requirements);
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(memoryProperties, requirements.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vkAllocateMemory(device, &allocateInfo, nullptr, &screenshot.memory);
  vkBindBufferMemory(device, screenshot.buffer, screenshot.memory, 0);

  VkImage image = swapChainInfo.images[swapChainInfo.acquiredImageIndex];
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0, 0, nullptr, 0, nullptr, 1, &barrier);

  VkBufferImageCopy region = {};
  region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
  region.imageExtent = { extent.width, extent.height, 1 };
  vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, screenshot.buffer, 1, &region);

  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = 0;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                       0, 0, nullptr, 0, nullptr, 1, &barrier);

  screenshot.extent = extent;
  screenshot.recorded = true;
}

void LsRenderer::WriteScreenshot() {
  vkWaitForFences(device, 1, &submitCompleteFence, VK_TRUE, UINT64_MAX);

  uint32_t width = screenshot.extent.width;
  uint32_t height = screenshot.extent.height;
  uint8_t* pixels = nullptr;
  vkMapMemory(device, screenshot.memory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&pixels));

  // BMP stores BGRA, a negative height makes the rows run top down like the image
  bool rgba = swapChainInfo.format == VK_FORMAT_R8G8B8A8_UNORM || swapChainInfo.format == VK_FORMAT_R8G8B8A8_SRGB;
  FILE* file = fopen(screenshot.path.c_str(), "wb");
  if( file ) {
    uint32_t imageSize = width*height*4;
    uint8_t header[54] = { 'B', 'M' };
    auto put32 = [&](int offset, uint32_t value) {
      for( int i = 0; i < 4; i++ ) {
        header[offset + i] = uint8_t(value >> (8*i));
      }
    };
    put32(2, 54 + imageSize);
    put32(10, 54);
    put32(14, 40);
    put32(18, width);
    put32(22, uint32_t(-int32_t(height)));
    header[26] = 1;
    header[28] = 32;
    put32(34, imageSize);
    fwrite(header, 1, sizeof(header), file);
    std::vector<uint8_t> row(width*4);
    for( uint32_t y = 0; y < height; y++ ) {
      const uint8_t* source = pixels + size_t(y)*width*4;
      for( uint32_t x = 0; x < width; x++ ) {
        row[x*4 + 0] = source[x*4 + (rgba ? 2 : 0)];
        row[x*4 + 1] = source[x*4 + 1];
        row[x*4 + 2] = source[x*4 + (rgba ? 0 : 2)];
        row[x*4 + 3] = 255;
      }
      fwrite(row.data(), 1, row.size(), file);
    }
    fclose(file);
    std::cout << "Saved screenshot " << screenshot.path << std::endl;
  }

  vkUnmapMemory(device, screenshot.memory);
  vkDestroyBuffer(device, screenshot.buffer, nullptr);
  vkFreeMemory(device, screenshot.memory, nullptr);
  screenshot.buffer = VK_NULL_HANDLE;
  screenshot.memory = VK_NULL_HANDLE;
  screenshot.path.clear();
  screenshot.recorded = false;
}

bool LsRenderer::IsUiCapturingMouse() {
  return uiInitialized && ImGui::GetIO().WantCaptureMouse;
}
