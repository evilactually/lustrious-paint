#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <assert.h>
#include <array>
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

#ifdef GIF_RECORDING
#include <FreeImage.h>
#endif

using namespace lslib;

//-------------------------------------------------------------------------------
// @ requiredInstanceExtensions
//-------------------------------------------------------------------------------
// List of instance-level extension that will be enabled when creating an instance
//-------------------------------------------------------------------------------
std::vector<const char*> requiredInstanceExtensions = {
  VK_KHR_SURFACE_EXTENSION_NAME,
  VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef VULKAN_VALIDATION
  VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
};

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
  "VK_LAYER_LUNARG_standard_validation"
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
                  VK_QUEUE_FAMILY_IGNORED,
                  VK_QUEUE_FAMILY_IGNORED,
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

LsRenderer::LsRenderer(HINSTANCE hInstance, HWND window):window(window) {
  //LsRenderer* renderer = LsRenderer::Get();
  
#ifdef GIF_RECORDING
  FreeImage_Initialise();
#endif

  LsLoadVulkanLibrary();

  LsLoadExportedEntryPoints();
  LsLoadGlobalLevelEntryPoints();

  CreateInstance(requiredInstanceExtensions, requiredInstanceValidationLayers, &instance);
 
  LsLoadInstanceLevelEntryPoints(instance, requiredInstanceExtensions);

#ifdef VULKAN_VALIDATION
  CreateDebugReportCallback(instance, &debugReportCallback);
#endif
  
  CreatePresentationSurface(instance, hInstance, window, &swapChainInfo.presentationSurface);

  CreateDevice(instance, 
               swapChainInfo.presentationSurface,
               requiredDeviceExtensions,
               &physicalDevice,
               &device,
               &graphicsQueue.familyIndex,
               &presentQueue.familyIndex);

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

    CreatePrimitivePipelines(device,
                             shaderModules.lineVertexShader,
                             shaderModules.lineFragmentShader,
                             shaderModules.pointVertexShader,
                             shaderModules.pointFragmentShader,
                             linePipelineLayout,
                             pointPipelineLayout,
                             renderPass,
                             swapChainInfo.extent,
                             &linePipeline,
                             &pointPipeline);
    
    canRender = true;
  }
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
                             linePipelineLayout,
                             pointPipelineLayout,
                             renderPass,
                             swapChainInfo.extent,
                             &linePipeline,
                             &pointPipeline);

	windowToVulkanTransformation = WindowToVulkanTransformation(window);

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
  if( vkWaitForFences( device, 1, &submitCompleteFence, VK_FALSE, 1000000000 ) != VK_SUCCESS ) {
    throw std::string("Waiting for fence takes too long!");
  }

  vkResetFences( device, 1, &submitCompleteFence );

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
  if( drawingContext.drawing ) {
    EndDrawing();
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

//void LsRenderer::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
//  switch (uMsg) {
//    case WM_SIZE:
//    RefreshSwapChain();
//    windowToVulkanTransformation = WindowToVulkanTransformation(window);
//    break;
//  }
//}