#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <vulkan_dynamic.hpp>

#include <destructor.h>
#include <LsVulkanLoader.h>
#include <LsError.h>
#include <LsVulkanInstance.h>
#include <LsVulkanDevice.h>
#include <LsVulkanPresentation.h>
#include <LsPushConstantTypes.h>
#include <LsVulkanRendering.h>
#include <LsRenderer.h>

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

LsRenderer::LsRenderer() {

}

LsRenderer::~LsRenderer() {

}

LsRenderer LsRenderer::renderer;

LsRenderer* LsRenderer::Get() {
  return &renderer;
}

void LsRenderer::Initialize(HINSTANCE hInstance, HWND window) {
  LsRenderer* renderer = LsRenderer::Get();
  
  LsLoadVulkanLibrary();
  renderer->vulkanDestructor = std::make_unique<destructor>(destructor([](){
    LsUnloadVulkanLibrary();
  }));

  LsLoadExportedEntryPoints();
  LsLoadGlobalLevelEntryPoints();

  CreateInstance(requiredInstanceExtensions, requiredInstanceValidationLayers, &renderer->instance);
  destructor& instanceDestructor = destructor([]() {
    LsRenderer::Get()->instance.destroy(nullptr);
  }).attach_to(*renderer->vulkanDestructor);

  LsLoadInstanceLevelEntryPoints(renderer->instance, requiredInstanceExtensions);

#ifdef VULKAN_VALIDATION
  CreateDebugReportCallback(renderer->instance, &renderer->debugReportCallback);
  destructor& debugReportCallbackDestructor = destructor([]() {
    LsRenderer::Get()->instance.destroyDebugReportCallbackEXT(LsRenderer::Get()->debugReportCallback, nullptr);
  }).attach_to(instanceDestructor);
#endif
  
  CreatePresentationSurface(renderer->instance, hInstance, window, &renderer->swapChainInfo.presentationSurface);
  destructor& presentationDestructor = destructor([]() {
    LsRenderer* renderer = LsRenderer::Get();
    renderer->instance.destroySurfaceKHR(renderer->swapChainInfo.presentationSurface, nullptr);
  }).attach_to(instanceDestructor);

  CreateDevice(renderer->instance, 
               renderer->swapChainInfo.presentationSurface,
               requiredDeviceExtensions,
               &renderer->physicalDevice,
               &renderer->device,
               &renderer->graphicsQueue.familyIndex,
               &renderer->presentQueue.familyIndex);
 
  LsLoadDeviceLevelEntryPoints(LsRenderer::Get()->device, requiredDeviceExtensions);
 
  destructor& deviceDestructor = destructor([]() {
    LsRenderer::Get()->device.waitIdle();
    LsRenderer::Get()->device.destroy(nullptr);
  }).attach_to(presentationDestructor);

  CreateSemaphore(renderer->device, &renderer->semaphores.imageAvailable);
  CreateSemaphore(renderer->device, &renderer->semaphores.renderingFinished);
  destructor& semaphoresDestructor = destructor([]() {
    LsRenderer* renderer = LsRenderer::Get();
    if ( renderer->semaphores.renderingFinished ) {
      renderer->device.destroySemaphore(renderer->semaphores.renderingFinished, nullptr);
    }

    if ( renderer->semaphores.imageAvailable ) {
      renderer->device.destroySemaphore(renderer->semaphores.imageAvailable, nullptr);
    }
  }).attach_to(deviceDestructor);

  renderer->shaderModules.lineVertexShader = CreateShaderModule(renderer->device, "Shaders/line.vert.spv");
  renderer->shaderModules.lineFragmentShader = CreateShaderModule(renderer->device, "Shaders/line.frag.spv");
  renderer->shaderModules.pointVertexShader = CreateShaderModule(renderer->device, "Shaders/point.vert.spv");
  renderer->shaderModules.pointFragmentShader = CreateShaderModule(renderer->device, "Shaders/point.frag.spv");
  destructor& shaderDestructor = destructor([]() {
    auto& shaderModules = LsRenderer::Get()->shaderModules;
    vk::Device& device = LsRenderer::Get()->device;
    device.destroyShaderModule(shaderModules.lineVertexShader, nullptr);
    device.destroyShaderModule(shaderModules.lineFragmentShader, nullptr);
    device.destroyShaderModule(shaderModules.pointVertexShader, nullptr);
    device.destroyShaderModule(shaderModules.pointFragmentShader, nullptr);
  }).attach_to(deviceDestructor);

  CreateFence(renderer->device, &renderer->submitCompleteFence, true);
  destructor& fenceDestructor = destructor([]() {
    LsRenderer* renderer = LsRenderer::Get();
    renderer->device.destroyFence(renderer->submitCompleteFence, nullptr);
  }).attach_to(deviceDestructor);

  renderer->device.getQueue( renderer->graphicsQueue.familyIndex, 0, &renderer->graphicsQueue.handle );
  renderer->device.getQueue( renderer->presentQueue.familyIndex, 0, &renderer->presentQueue.handle );

  CreateSwapChain(renderer->physicalDevice,
                  renderer->device,
                  renderer->swapChainInfo.presentationSurface,
                  window,
                  &renderer->swapChainInfo.swapChain,
                  &renderer->swapChainInfo.format,
                  &renderer->swapChainInfo.extent);

  renderer->swapChainInfo.images = GetSwapChainImages(renderer->device, renderer->swapChainInfo.swapChain);

  renderer->swapChainInfo.imageViews = CreateSwapChainImageViews(renderer->device,
                                                                 renderer->swapChainInfo.images,
                                                                 renderer->swapChainInfo.format);

  renderer->renderPass = CreateSimpleRenderPass(renderer->device, renderer->swapChainInfo.format);

  renderer->framebuffers = CreateFramebuffers(renderer->device, 
                                              renderer->swapChainInfo.imageViews,
                                              renderer->swapChainInfo.extent,
                                              renderer->renderPass);

  renderer->linePipelineLayout = CreatePipelineLayout(renderer->device, sizeof(LinePushConstants));
  renderer->pointPipelineLayout = CreatePipelineLayout(renderer->device, sizeof(PointPushConstants));

  // CreatePrimitivePipelines(renderer->device,
  //                          renderer->shaderModules.lineVertexShader,
  //                          renderer->shaderModules.lineFragmentShader,
  //                          renderer->shaderModules.pointVertexShader,
  //                          renderer->shaderModules.pointFragmentShader,
  //                          renderer->linePipelineLayout,
  //                          renderer->pointPipelineLayout,
  //                          renderer->renderPass,
  //                          renderer->swapChainInfo.extent,
  //                          &renderer->linePipeline,
  //                          &renderer->pointPipeline);
  
}

void LsRenderer::BeginFrame() {
  // if( device.waitForFences( 1, &submitCompleteFence, VK_FALSE, 1000000000 ) != vk::Result::eSuccess ) {
  //   throw std::string("Waiting for fence takes too long!");
  // }

  // device.resetFences( 1, &submitCompleteFence );

  // vk::Result result = device.acquireNextImageKHR( swapChainInfo.swapChain, 
  //   UINT64_MAX,
  //   semaphores.imageAvailable,
  //   vk::Fence(),
  //   &swapChainInfo.acquiredImageIndex );

  // switch( result ) {
  //   case vk::Result::eSuccess:
  //   break;
  //   case vk::Result::eSuboptimalKHR:
  //   // It's still OK to use.
  //   break;
  //   case vk::Result::eErrorOutOfDateKHR:
  //   //RefreshSwapChain();
  //   //UpdatePixelDimensions();
  //   return;
  //   default:
  //   throw std::string("Problem occurred during swap chain image acquisition!");
  // }

  // vk::CommandBufferBeginInfo cmd_buffer_begin_info(
  //   vk::CommandBufferUsageFlagBits::eSimultaneousUse, // VkCommandBufferUsageFlags              flags
  //   nullptr                                           // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
  //   );

  // commandBuffer.begin( &cmd_buffer_begin_info );

  // vk::ImageSubresourceRange image_subresource_range(
  //   vk::ImageAspectFlagBits::eColor,                // VkImageAspectFlags                     aspectMask
  //   0,                                              // uint32_t                               baseMipLevel
  //   1,                                              // uint32_t                               levelCount
  //   0,                                              // uint32_t                               baseArrayLayer
  //   1                                               // uint32_t                               layerCount
  //   );

  // // Transition to presentation layout and tell vulkan that we are discarding previous contents of the image
  // // block reads from present(atachment output), blocks draws and clears
  // vk::ImageMemoryBarrier barrier_from_present_to_draw(
  //   vk::AccessFlagBits(),                                   // VkAccessFlags            srcAccessMask
  //   vk::AccessFlagBits::eMemoryRead,                        // VkAccessFlags            dstAccessMask
  //   vk::ImageLayout::eUndefined,                            // VkImageLayout            oldLayout // TODO: DO AN INITIAL FILL
  //   vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            newLayout
  //   VK_QUEUE_FAMILY_IGNORED,                                // uint32_t                 srcQueueFamilyIndex
  //   VK_QUEUE_FAMILY_IGNORED,                                // uint32_t                 dstQueueFamilyIndex
  //   swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
  //   image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
  //   );

  // commandBuffer.pipelineBarrier( 
  //   vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
  //   vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
  //   vk::DependencyFlagBits(),
  //   0,
  //   nullptr,
  //   0,
  //   nullptr,
  //   1,
  //   &barrier_from_present_to_draw);
}

void LsRenderer::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_SIZE:
      break;
  }
}