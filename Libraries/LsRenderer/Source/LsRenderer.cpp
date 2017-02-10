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

LsRenderer::LsRenderer(HINSTANCE hInstance, HWND window):window(window) {
  //LsRenderer* renderer = LsRenderer::Get();
  
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
 
  CreateSemaphore(device, &semaphores.imageAvailable);
  CreateSemaphore(device, &semaphores.renderingFinished);

  shaderModules.lineVertexShader = CreateShaderModule(device, "Shaders/line.vert.spv");
  shaderModules.lineFragmentShader = CreateShaderModule(device, "Shaders/line.frag.spv");
  shaderModules.pointVertexShader = CreateShaderModule(device, "Shaders/point.vert.spv");
  shaderModules.pointFragmentShader = CreateShaderModule(device, "Shaders/point.frag.spv");
  
  CreateFence(device, &submitCompleteFence, true);

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
  
  if ( debugReportCallback )
    vkDestroyDebugReportCallbackEXT( instance, debugReportCallback, nullptr );
  
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

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &semaphores.renderingFinished;
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