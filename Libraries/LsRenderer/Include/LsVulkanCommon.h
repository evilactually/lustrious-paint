#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>
#include <LsFile.h>

VKAPI_ATTR VkBool32 VKAPI_CALL LsDebugReportCallback( VkDebugReportFlagsEXT flags, 
    VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, 
    int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData ) {
    std::cout << pLayerPrefix << ": " << pMessage << std::endl;
    return VK_FALSE;
}

void CreateDebugReportCallback(vk::Instance instance, vk::DebugReportCallbackEXT* debugReportCallback) {
  vk::DebugReportCallbackCreateInfoEXT callbackCreateInfo(
    vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning,
    LsDebugReportCallback,
    nullptr);
  instance.createDebugReportCallbackEXT(&callbackCreateInfo, nullptr, debugReportCallback);
}

bool CheckExtensionAvailability( const char *extensionName, 
                                 std::vector<vk::ExtensionProperties> const& availableExtensions ) {
  for( size_t i = 0; i < availableExtensions.size(); ++i ) {
    if( !strcmp(availableExtensions[i].extensionName, extensionName) ) {
      return true;
    }
  }
  return false;
}

bool CheckLayerAvailability( const char *layerName, 
                             std::vector<vk::LayerProperties> const& availableLayers ) {
  for( size_t i = 0; i < availableLayers.size(); ++i ) {
    if( !strcmp(availableLayers[i].layerName, layerName) ) {
      return true;
    }
  }
  return false;
}

void CreateSemaphore(vk::Device const& device, vk::Semaphore* semaphore) {
  vk::SemaphoreCreateInfo semaphoreCreateInfo = {
    vk::SemaphoreCreateFlags()
  };

  if( device.createSemaphore( &semaphoreCreateInfo, nullptr, semaphore ) != vk::Result::eSuccess ) {
    throw std::string("Could not create semaphores!");
  }
}

void CreateFence(vk::Device const& device, vk::Fence* fence, bool signaled) {
  vk::FenceCreateInfo fenceCreateInfo;

  if ( signaled )
  {
    fenceCreateInfo.setFlags( vk::FenceCreateFlagBits::eSignaled );
  }

  if( device.createFence( &fenceCreateInfo, nullptr, fence ) != vk::Result::eSuccess ) {
    throw std::string("Could not create a fence!");   
  }
}

vk::ShaderModule CreateShaderModule(vk::Device const& device, const char* filename ) {
  const std::vector<char> code = GetBinaryFileContents( filename );
  if( code.size() == 0 ) {
    LsError();
  }

  vk::ShaderModuleCreateInfo shader_module_create_info = {
    vk::ShaderModuleCreateFlags(),                  // VkShaderModuleCreateFlags      flags
    code.size(),                                    // size_t                         codeSize
    reinterpret_cast<const uint32_t*>(&code[0])     // const uint32_t                *pCode
  };

  vk::ShaderModule shader_module;
  if( device.createShaderModule( &shader_module_create_info, nullptr, &shader_module ) != vk::Result::eSuccess ) {
    throw std::string("Could not create shader module from a \"") + std::string(filename) + std::string("\" file!");
  }
  return shader_module;
}