#pragma once 

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

#include "LsVulkanCommon.h"

bool CheckInstanceLayers(std::vector<const char*> const& requiredLayers) {
  uint32_t availableLayerCount;
  vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(availableLayerCount);
  vkEnumerateInstanceLayerProperties(&availableLayerCount, &availableLayers[0]);
  availableLayers.resize(availableLayerCount);

  for(auto requiredLayer:requiredLayers) {
    if ( !CheckLayerAvailability(requiredLayer, availableLayers) ) {
      std::cout << "Required instance layer not found: " << requiredLayer << std::endl; 
      return false;
    }
  }

  return true;
}

bool CheckInstanceExtensions(std::vector<const char*> requiredExtensions) {
  uint32_t availableExtensionsCount = 0;
  VkResult result = vkEnumerateInstanceExtensionProperties(
    nullptr, 
    &availableExtensionsCount,
    nullptr );

  if ( result != VK_SUCCESS || availableExtensionsCount == 0 ) {
    throw std::string("Error occurred during instance extension enumeration!");
  }

  std::vector<VkExtensionProperties> availableExtensions( availableExtensionsCount );

  result = vkEnumerateInstanceExtensionProperties(
    nullptr, 
    &availableExtensionsCount,
    availableExtensions.data());

  availableExtensions.resize(availableExtensionsCount);

  if( result != VK_SUCCESS ) {
    throw std::string("Error occurred during instance extension enumeration!");
  }

  for(auto requiredExtension:requiredExtensions) {
    if( !CheckExtensionAvailability(requiredExtension, availableExtensions) ) {
      std::cout << "Required instance extension missing: " << requiredExtension << std::endl;
      return false;
    }
  }

  return true;
}

//-------------------------------------------------------------------------------
// @ CreateInstance()
//-------------------------------------------------------------------------------
// Create Vulkan instance with all required instance extensions
//-------------------------------------------------------------------------------
void CreateInstance( std::vector<const char*> const& extensions, 
                     std::vector<const char*> const& layers,
                     VkInstance* instance ) {
  if ( !CheckInstanceExtensions(extensions) ) {
    throw std::string("Instance missing required extensions");
  }

  if ( !CheckInstanceLayers(layers) ) {
    throw std::string("Instance missing required layers");
  }

  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = NULL;  // TODO: appliactionInfo field is optional, but fill it out later
  instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
  instanceCreateInfo.ppEnabledLayerNames = layers.data();
  instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
  
  if ( vkCreateInstance( &instanceCreateInfo, nullptr, instance) != VK_SUCCESS ) {
    throw std::string( "Failed to create Vulkan instance!" );
  }
}
