#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

VkCommandPool CreateCommandPool(VkDevice device, uint32_t familyIndex) {
  VkCommandPoolCreateInfo cmd_pool_create_info = {};
  cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow resetting command buffer
  cmd_pool_create_info.queueFamilyIndex = familyIndex;

  VkCommandPool commandPool;
  if ( vkCreateCommandPool( device, &cmd_pool_create_info, nullptr, &commandPool ) != VK_SUCCESS ) {
    throw std::string("Could not create a command pool!");
  }

  return commandPool;
}

VkCommandBuffer CreateCommandBuffer(VkDevice const& device, VkCommandPool const& commandPool) {
  VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {};
  cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_buffer_allocate_info.commandPool = commandPool;
  cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd_buffer_allocate_info.commandBufferCount = 1;
	
  VkCommandBuffer commandBuffer;
  if( vkAllocateCommandBuffers( device, &cmd_buffer_allocate_info, &commandBuffer ) != VK_SUCCESS ) {
    throw std::string("Could not allocate command buffer!");
  }

  return commandBuffer;
}