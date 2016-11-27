#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

vk::CommandPool CreateCommandPool(vk::Device device, uint32_t familyIndex) {
  vk::CommandPoolCreateInfo cmd_pool_create_info(
    vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // allow resetting command buffer
    familyIndex );

  vk::CommandPool commandPool;
  if (device.createCommandPool(&cmd_pool_create_info, nullptr, &commandPool) != vk::Result::eSuccess) {
    throw std::string("Could not create a command pool!");
  }

  return commandPool;
}

vk::CommandBuffer CreateCommandBuffer(vk::Device const& device, vk::CommandPool const& commandPool) {
  vk::CommandBufferAllocateInfo cmd_buffer_allocate_info(
    commandPool,                      // VkCommandPool                commandPool
    vk::CommandBufferLevel::ePrimary, // VkCommandBufferLevel         level
    1 );                              // uint32_t                     bufferCount

  vk::CommandBuffer commandBuffer;
  if( device.allocateCommandBuffers( &cmd_buffer_allocate_info, &commandBuffer ) != vk::Result::eSuccess ) {
    throw std::string("Could not allocate command buffer!");
  }

  return commandBuffer;
}