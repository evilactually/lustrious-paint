#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <assert.h>
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

LsRenderer LsRenderer::renderer;

LsRenderer* LsRenderer::Get() {
  return &renderer;
}

void LsRenderer::Initialize(HINSTANCE hInstance, HWND window) {
  LsRenderer* renderer = LsRenderer::Get();
  
  renderer->window = window;

  LsLoadVulkanLibrary();

  LsLoadExportedEntryPoints();
  LsLoadGlobalLevelEntryPoints();

  CreateInstance(requiredInstanceExtensions, requiredInstanceValidationLayers, &renderer->instance);
 
  LsLoadInstanceLevelEntryPoints(renderer->instance, requiredInstanceExtensions);

#ifdef VULKAN_VALIDATION
  CreateDebugReportCallback(renderer->instance, &renderer->debugReportCallback);
#endif
  
  CreatePresentationSurface(renderer->instance, hInstance, window, &renderer->swapChainInfo.presentationSurface);

  CreateDevice(renderer->instance, 
               renderer->swapChainInfo.presentationSurface,
               requiredDeviceExtensions,
               &renderer->physicalDevice,
               &renderer->device,
               &renderer->graphicsQueue.familyIndex,
               &renderer->presentQueue.familyIndex);

  LsLoadDeviceLevelEntryPoints(LsRenderer::Get()->device, requiredDeviceExtensions);

  renderer->commandPool = CreateCommandPool(renderer->device, renderer->graphicsQueue.familyIndex);
  renderer->commandBuffer = CreateCommandBuffer(renderer->device, renderer->commandPool);
 
  CreateSemaphore(renderer->device, &renderer->semaphores.imageAvailable);
  CreateSemaphore(renderer->device, &renderer->semaphores.renderingFinished);

  renderer->shaderModules.lineVertexShader = CreateShaderModule(renderer->device, "Shaders/line.vert.spv");
  renderer->shaderModules.lineFragmentShader = CreateShaderModule(renderer->device, "Shaders/line.frag.spv");
  renderer->shaderModules.pointVertexShader = CreateShaderModule(renderer->device, "Shaders/point.vert.spv");
  renderer->shaderModules.pointFragmentShader = CreateShaderModule(renderer->device, "Shaders/point.frag.spv");

  CreateFence(renderer->device, &renderer->submitCompleteFence, true);

  renderer->device.getQueue( renderer->graphicsQueue.familyIndex, 0, &renderer->graphicsQueue.handle );
  renderer->device.getQueue( renderer->presentQueue.familyIndex, 0, &renderer->presentQueue.handle );

  if ( CreateSwapChain(renderer->physicalDevice,
                       renderer->device,
                       renderer->swapChainInfo.presentationSurface,
                       window,
                       &renderer->swapChainInfo.swapChain,
                       &renderer->swapChainInfo.format,
                       &renderer->swapChainInfo.extent) ) {

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

    CreatePrimitivePipelines(renderer->device,
                             renderer->shaderModules.lineVertexShader,
                             renderer->shaderModules.lineFragmentShader,
                             renderer->shaderModules.pointVertexShader,
                             renderer->shaderModules.pointFragmentShader,
                             renderer->linePipelineLayout,
                             renderer->pointPipelineLayout,
                             renderer->renderPass,
                             renderer->swapChainInfo.extent,
                             &renderer->linePipeline,
                             &renderer->pointPipeline);
    
    renderer->canRender = true;
  }
}

void LsRenderer::RefreshSwapChain() {
  canRender = false;

  if ( device ) {
    device.waitIdle();
  
    if ( linePipeline )
    {
      device.destroyPipeline(linePipeline, nullptr);
      linePipeline = vk::Pipeline();
    }

    if ( pointPipeline )
    {
      device.destroyPipeline(pointPipeline, nullptr);
      pointPipeline = vk::Pipeline();
    }

    if ( linePipelineLayout ) {
      device.destroyPipelineLayout(linePipelineLayout, nullptr);
      linePipelineLayout = vk::PipelineLayout();
    }

    if ( pointPipelineLayout ) {
      device.destroyPipelineLayout(pointPipelineLayout, nullptr);
      pointPipelineLayout = vk::PipelineLayout();
    }

    for ( const vk::Framebuffer& framebuffer:framebuffers ) {
      device.destroyFramebuffer(framebuffer, nullptr);
    }
    framebuffers.clear();

    if ( renderPass ) {
      device.destroyRenderPass(renderPass, nullptr);
      renderPass = vk::RenderPass();
    }

    for( auto &imageView: swapChainInfo.imageViews) {
      device.destroyImageView(imageView, nullptr);
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

    canRender = true;
  } else {
    std::cout << "Swap chain not ready, cannot render." << std::endl;
  }
}

LsRenderer::~LsRenderer() {
  canRender = false;

  if ( device ) {
    device.waitIdle();
  
    if ( linePipeline )
    {
      device.destroyPipeline(linePipeline, nullptr);
    }

    if ( pointPipeline )
    {
      device.destroyPipeline(pointPipeline, nullptr);
    }

    if ( linePipelineLayout ) {
      device.destroyPipelineLayout(linePipelineLayout, nullptr);
    }

    if ( pointPipelineLayout ) {
      device.destroyPipelineLayout(pointPipelineLayout, nullptr);
    }

    for ( const vk::Framebuffer& framebuffer:framebuffers ) {
      device.destroyFramebuffer(framebuffer, nullptr);
    }

    if ( renderPass )
      device.destroyRenderPass(renderPass, nullptr);

    for( auto &imageView: swapChainInfo.imageViews) {
      device.destroyImageView(imageView, nullptr);
    }

    if ( swapChainInfo.swapChain )
      device.destroySwapchainKHR(swapChainInfo.swapChain, nullptr);

    if ( submitCompleteFence )
      device.destroyFence(submitCompleteFence, nullptr);

    if ( shaderModules.lineVertexShader )
      device.destroyShaderModule(shaderModules.lineVertexShader, nullptr);
    if ( shaderModules.lineFragmentShader )
      device.destroyShaderModule(shaderModules.lineFragmentShader, nullptr);
    if ( shaderModules.pointVertexShader )
      device.destroyShaderModule(shaderModules.pointVertexShader, nullptr);
    if ( shaderModules.pointFragmentShader )
      device.destroyShaderModule(shaderModules.pointFragmentShader, nullptr);

    if ( semaphores.renderingFinished )
      device.destroySemaphore(semaphores.renderingFinished, nullptr);

    if ( semaphores.imageAvailable )
      device.destroySemaphore(semaphores.imageAvailable, nullptr);

    if( commandPool ) {
      device.destroyCommandPool( commandPool, nullptr );
      commandPool = vk::CommandPool();
    }

    device.destroy(nullptr);
  }

  if ( swapChainInfo.presentationSurface )
    instance.destroySurfaceKHR(swapChainInfo.presentationSurface, nullptr);
  
  if ( debugReportCallback )
    instance.destroyDebugReportCallbackEXT(debugReportCallback, nullptr);
  
  if ( instance )  
    instance.destroy(nullptr);

  LsUnloadVulkanLibrary();
}

void LsRenderer::BeginFrame() {
  if( device.waitForFences( 1, &submitCompleteFence, VK_FALSE, 1000000000 ) != vk::Result::eSuccess ) {
    throw std::string("Waiting for fence takes too long!");
  }

  device.resetFences( 1, &submitCompleteFence );

  vk::Result result = device.acquireNextImageKHR(
    swapChainInfo.swapChain, 
    UINT64_MAX,
    semaphores.imageAvailable,
    vk::Fence(),
    &swapChainInfo.acquiredImageIndex );

  switch( result ) {
    case vk::Result::eSuccess:
    break;
    case vk::Result::eSuboptimalKHR:
    // It's still OK to use.
    break;
    case vk::Result::eErrorOutOfDateKHR:
    RefreshSwapChain();
    return;
    default:
    throw std::string("Problem occurred during swap chain image acquisition!");
  }

  vk::CommandBufferBeginInfo cmd_buffer_begin_info(
    vk::CommandBufferUsageFlagBits::eSimultaneousUse, // VkCommandBufferUsageFlags              flags
    nullptr                                           // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
    );

  commandBuffer.begin( &cmd_buffer_begin_info );

  vk::ImageSubresourceRange image_subresource_range(
    vk::ImageAspectFlagBits::eColor,                // VkImageAspectFlags                     aspectMask
    0,                                              // uint32_t                               baseMipLevel
    1,                                              // uint32_t                               levelCount
    0,                                              // uint32_t                               baseArrayLayer
    1                                               // uint32_t                               layerCount
    );

  // Transition to presentation layout and tell vulkan that we are discarding previous contents of the image
  // block reads from present(atachment output), blocks draws and clears
  vk::ImageMemoryBarrier barrier_from_present_to_draw(
    vk::AccessFlagBits(),                                   // VkAccessFlags            srcAccessMask
    vk::AccessFlagBits::eMemoryRead,                        // VkAccessFlags            dstAccessMask
    vk::ImageLayout::eUndefined,                            // VkImageLayout            oldLayout // TODO: DO AN INITIAL FILL
    vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            newLayout
    VK_QUEUE_FAMILY_IGNORED,                                // uint32_t                 srcQueueFamilyIndex
    VK_QUEUE_FAMILY_IGNORED,                                // uint32_t                 dstQueueFamilyIndex
    swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
    image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
    );

  commandBuffer.pipelineBarrier( 
    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
    vk::DependencyFlagBits(),
    0,
    nullptr,
    0,
    nullptr,
    1,
    &barrier_from_present_to_draw);
}

void LsRenderer::EndFrame() {
  if( drawingContext.drawing ) {
    EndDrawing();
  }

  if( commandBuffer.end() != vk::Result::eSuccess ) {
    throw std::string("Could not record command buffers!");
  }

  // stall these stages until image is available from swap chain
  vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
  vk::PipelineStageFlagBits::eTransfer;
  vk::SubmitInfo submit_info(
    1,                             // uint32_t                     waitSemaphoreCount
    &semaphores.imageAvailable,    // const VkSemaphore           *pWaitSemaphores
    &wait_dst_stage_mask,          // const VkPipelineStageFlags  *pWaitDstStageMask;
    1,                             // uint32_t                     commandBufferCount
    &commandBuffer,                // const VkCommandBuffer       *pCommandBuffers
    1,                             // uint32_t                     signalSemaphoreCount
    &semaphores.renderingFinished  // const VkSemaphore           *pSignalSemaphores
    );

  if( graphicsQueue.handle.submit( 1, &submit_info, submitCompleteFence ) != vk::Result::eSuccess ) {
    throw std::string("Submit to queue failed!");
  }

  vk::PresentInfoKHR present_info(
    1,                                    // uint32_t                     waitSemaphoreCount
    &semaphores.renderingFinished,        // const VkSemaphore           *pWaitSemaphores
    1,                                    // uint32_t                     swapchainCount
    &swapChainInfo.swapChain,             // const VkSwapchainKHR        *pSwapchains
    &swapChainInfo.acquiredImageIndex,    // const uint32_t              *pImageIndices
    nullptr                               // VkResult                    *pResults
    );

  vk::Result result = presentQueue.handle.presentKHR( &present_info );

  switch( result ) {
    case vk::Result::eSuccess:
    break;
    case vk::Result::eErrorOutOfDateKHR:
    case vk::Result::eSuboptimalKHR:
    RefreshSwapChain();
    return;
    default:
    throw std::string("Problem occurred during image presentation!");
  }
}

void LsRenderer::Clear(float r, float g, float b) {
  if( drawingContext.drawing ) {
    EndDrawing();
  }

  vk::ImageSubresourceRange image_subresource_range(
    vk::ImageAspectFlagBits::eColor,                // VkImageAspectFlags                     aspectMask
    0,                                              // uint32_t                               baseMipLevel
    1,                                              // uint32_t                               levelCount
    0,                                              // uint32_t                               baseArrayLayer
    1                                               // uint32_t                               layerCount
  );

  vk::ImageMemoryBarrier barrier_from_present_to_clear(
    vk::AccessFlagBits::eMemoryRead,            // VkAccessFlags                          srcAccessMask,
    vk::AccessFlagBits::eTransferWrite,         // VkAccessFlags                          dstAccessMask
    vk::ImageLayout::ePresentSrcKHR,            // VkImageLayout                          oldLayout
    vk::ImageLayout::eTransferDstOptimal,       // VkImageLayout                          newLayout
    VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
    VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
    swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                                image
    image_subresource_range                     // VkImageSubresourceRange                subresourceRange
  );

  commandBuffer.pipelineBarrier( vk::PipelineStageFlagBits::eTransfer, 
                                 vk::PipelineStageFlagBits::eTransfer,
                                 vk::DependencyFlagBits(),
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier_from_present_to_clear );

  const std::array<float, 4> color = { r, g, b, 1.0f };
  vk::ClearColorValue clear_color(
    color
  );

  commandBuffer.clearColorImage( swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                                 vk::ImageLayout::eTransferDstOptimal,
                                 &clear_color,
                                 1,
                                 &image_subresource_range );

  vk::ImageMemoryBarrier barrier_from_clear_to_present(
    vk::AccessFlagBits::eTransferWrite ,        // VkAccessFlags                          srcAccessMask, eMemoryRead fails validation
    vk::AccessFlagBits::eMemoryRead,            // VkAccessFlags                          dstAccessMask
    vk::ImageLayout::eTransferDstOptimal,       // VkImageLayout                          oldLayout
    vk::ImageLayout::ePresentSrcKHR,            // VkImageLayout                          newLayout
    VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
    VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
    swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                                image
    image_subresource_range                     // VkImageSubresourceRange                subresourceRange
  );

  commandBuffer.pipelineBarrier( vk::PipelineStageFlagBits::eTransfer, 
                                 vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
                                 vk::DependencyFlagBits(),
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
    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, linePipeline );
    drawingContext.pipelineBinding = PipelineBinding::eLine; 
  }

  // Transition image layout from generic read/present
  LinePushConstants pushConstants;
  pushConstants.positions[0] = x1;
  pushConstants.positions[1] = y1;
  pushConstants.positions[2] = x2;
  pushConstants.positions[3] = y2;
  std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));

  commandBuffer.pushConstants( linePipelineLayout,
                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                               0, // offset
                               sizeof(LinePushConstants),
                               &pushConstants );
  commandBuffer.setLineWidth( drawingContext.lineWidth );
  commandBuffer.draw( 2, 1, 0, 0 );
}

void LsRenderer::DrawPoint(float x, float y) {
  if( !drawingContext.drawing ) {
    BeginDrawing();
  }

  if(drawingContext.pipelineBinding != PipelineBinding::ePoint) {
    // Bind line graphics pipeline
    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, pointPipeline );
    drawingContext.pipelineBinding = PipelineBinding::ePoint;
  }

  // Transition image layout from generic read/present
  // TODO: Do I need to push it every time?
  PointPushConstants pushConstants;
  pushConstants.positions[0] = x;
  pushConstants.positions[1] = y;
  pushConstants.size = drawingContext.pointSize;
  std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));

  commandBuffer.pushConstants( pointPipelineLayout,
                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                               0, // offset
                               sizeof(PointPushConstants),
                               &pushConstants );
  commandBuffer.draw( 1, 1, 0, 0 );
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
  vk::ImageSubresourceRange image_subresource_range(
    vk::ImageAspectFlagBits::eColor, // VkImageAspectFlags                     aspectMask
    0,                               // uint32_t                               baseMipLevel
    1,                               // uint32_t                               levelCount
    0,                               // uint32_t                               baseArrayLayer
    1                                // uint32_t                               layerCount
    );

  // wait for writes from clears and draws, block draws
  vk::ImageMemoryBarrier barrier_from_present_to_draw(
    vk::AccessFlagBits(),                                   // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
    vk::AccessFlagBits::eColorAttachmentWrite,              // VkAccessFlags            dstAccessMask
    vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            oldLayout
    vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            newLayout
    presentQueue.familyIndex,                               // uint32_t                 srcQueueFamilyIndex
    graphicsQueue.familyIndex,                              // uint32_t                 dstQueueFamilyIndex
    swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
    image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
    );

  commandBuffer.pipelineBarrier( 
    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::DependencyFlagBits(),
    0,
    nullptr,
    0,
    nullptr,
    1,
    &barrier_from_present_to_draw);

  // Begin rendering pass
  vk::RenderPassBeginInfo render_pass_begin_info = {
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
  commandBuffer.beginRenderPass( &render_pass_begin_info, vk::SubpassContents::eInline );

  // Tell drawing functions that command buffer is already prepared for drawing
  drawingContext.drawing = true;

  drawingContext.pipelineBinding = PipelineBinding::eNone;
}

void LsRenderer::EndDrawing() {
  commandBuffer.endRenderPass();

  vk::ImageSubresourceRange image_subresource_range(
    vk::ImageAspectFlagBits::eColor, // VkImageAspectFlags                     aspectMask
    0,                               // uint32_t                               baseMipLevel
    1,                               // uint32_t                               levelCount
    0,                               // uint32_t                               baseArrayLayer
    1                                // uint32_t                               layerCount
  );

  vk::ImageMemoryBarrier barrier_from_present_to_draw(
    vk::AccessFlagBits::eColorAttachmentWrite,              // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
    vk::AccessFlagBits::eMemoryRead,                        // VkAccessFlags            dstAccessMask
    vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            oldLayout
    vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            newLayout
    graphicsQueue.familyIndex,                              // uint32_t                 srcQueueFamilyIndex
    presentQueue.familyIndex,                               // uint32_t                 dstQueueFamilyIndex
    swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
    image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
  );

  commandBuffer.pipelineBarrier( 
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::PipelineStageFlagBits::eBottomOfPipe, // need to block presentation, there's no actual stage for it,
    vk::DependencyFlagBits(),                 // but bottom of pipe is what we need
    0,
    nullptr,
    0,
    nullptr,
    1,
    &barrier_from_present_to_draw);

  drawingContext.drawing = false;
}

void LsRenderer::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_SIZE:
    RefreshSwapChain();
    break;
  }
}