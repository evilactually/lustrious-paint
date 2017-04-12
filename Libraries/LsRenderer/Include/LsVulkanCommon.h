#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan_dynamic.hpp>
#include <LsFile.h>

VKAPI_ATTR VkBool32 VKAPI_CALL LsDebugReportCallback( VkDebugReportFlagsEXT flags, 
    VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, 
    int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData ) {
    std::cout << pLayerPrefix << ": " << pMessage << std::endl;
    return VK_FALSE;
}

void CreateDebugReportCallback(VkInstance instance, VkDebugReportCallbackEXT* debugReportCallback) {
	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	callbackCreateInfo.pfnCallback = LsDebugReportCallback;
	vkCreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, debugReportCallback);
}

bool CheckExtensionAvailability( const char *extensionName, 
                                 std::vector<VkExtensionProperties> const& availableExtensions ) {
  for( size_t i = 0; i < availableExtensions.size(); ++i ) {
    if( !strcmp(availableExtensions[i].extensionName, extensionName) ) {
      return true;
    }
  }
  return false;
}

bool CheckLayerAvailability( const char *layerName, 
                             std::vector<VkLayerProperties> const& availableLayers ) {
  for( size_t i = 0; i < availableLayers.size(); ++i ) {
    if( !strcmp(availableLayers[i].layerName, layerName) ) {
      return true;
    }
  }
  return false;
}

void CreateSemaphore(VkDevice const& device, VkSemaphore* semaphore) {
  VkSemaphoreCreateInfo semaphoreCreateInfo = {};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if( vkCreateSemaphore( device, &semaphoreCreateInfo, nullptr, semaphore ) != VK_SUCCESS ) {
    throw std::string("Could not create semaphores!");
  }
}

void CreateFence(VkDevice const& device, VkFence* fence, bool signaled) {
  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  if ( signaled )
  {
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  }

  if( vkCreateFence( device, &fenceCreateInfo, nullptr, fence ) != VK_SUCCESS ) {
    throw std::string("Could not create a fence!");   
  }
}

VkShaderModule CreateShaderModule(VkDevice const& device, const char* filename ) {
  const std::vector<char> code = GetBinaryFileContents( filename );
  if( code.size() == 0 ) {
    LsError();
  }

  VkShaderModuleCreateInfo shader_module_create_info = {};
  shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_module_create_info.flags = 0;
  shader_module_create_info.codeSize = code.size();
  shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(&code[0]);

  VkShaderModule shader_module;
  if( vkCreateShaderModule( device, &shader_module_create_info, nullptr, &shader_module ) != VK_SUCCESS ) {
    throw std::string("Could not create shader module from a \"") + std::string(filename) + std::string("\" file!");
  }
  return shader_module;
}

static glm::tmat3x3<float> WindowToVulkanTransformation(HWND windowHandle) {
  RECT clientRect;
  ::GetClientRect(windowHandle, &clientRect);

  float width = (float)(clientRect.right - clientRect.left);
  float height = (float)(clientRect.bottom - clientRect.top);
  float pixel_width = 2.0f/width;
  float pixel_height = 2.0f/height;
 
  glm::tmat3x3<float> transformation(
	  pixel_width,             0.0f,                      0.0f,
	  0.0f,                    pixel_height,              0.0f,
	  -pixel_width*width/2.0f, -pixel_height*height/2.0f, 1.0f);

  return transformation;
}

// Find a memory type in "memoryTypeBits" that includes all of "properties"
int32_t FindMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
  for (int32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
  {
    if ((memoryTypeBits & (1 << i)) &&
      ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
      return i;
  }
  return -1;
}

void CmdImageBarrier(VkCommandBuffer cmdBuffer,
                     VkAccessFlags srcAccessMask,
                     VkAccessFlags  dstAccessMask,
                     VkImageLayout oldLayout,
                     VkImageLayout newLayout,
                     uint32_t srcQueueFamilyIndex,
                     uint32_t dstQueueFamilyIndex,
                     VkImage image,
                     VkImageSubresourceRange subresourceRange,
                     VkPipelineStageFlags srcStageMask,
                     VkPipelineStageFlags dstStageMask,
                     VkDependencyFlags dependencyFlags) {
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = srcAccessMask;
  barrier.dstAccessMask = dstAccessMask;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
  barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
  barrier.image = image;
  barrier.subresourceRange = subresourceRange;
  vkCmdPipelineBarrier(cmdBuffer, srcAccessMask, dstAccessMask, dependencyFlags, 0, nullptr, 0, nullptr, 1, &barrier);
}