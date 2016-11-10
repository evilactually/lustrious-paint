#include <vulkan_dynamic.hpp>
#include <iostream>

// Needs device, swapchain, graphics queue

class LsRenderer {
public:
  LsRenderer();
  void BeginDrawing();
  void EndDrawing();
  void BeginFrame();
  void EndFrame();
  void Clear(float r, float g, float b);
  void DrawLine(float x1, float y1, float x2, float y2);
  void DrawPoint(float x, float y);
  void SetColor(float r, float g, float b);
  void SetLineWidth(float width);
  void SetPointSize(float size);
private:
  vk::Device device;
  vk::Fence submitCompleteFence;
};

void LsRenderer::BeginFrame() {
  if( device.waitForFences( 1, &submitCompleteFence, VK_FALSE, 1000000000 ) != vk::Result::eSuccess ) {
    std::cout << "Waiting for fence takes too long!" << std::endl;
    //LsError();
  }

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
  //   break;
  //   case vk::Result::eErrorOutOfDateKHR:
  //   std::cout << "Swap chain out of date" << std::endl;
  //   RefreshSwapChain();
  //   UpdatePixelDimensions();
  //   return;
  //   default:
  //   std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
  //   LsError();
  // }

  // vk::CommandBufferBeginInfo cmd_buffer_begin_info(
  //         vk::CommandBufferUsageFlagBits::eSimultaneousUse, // VkCommandBufferUsageFlags              flags
  //         nullptr                                           // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
  //         );

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