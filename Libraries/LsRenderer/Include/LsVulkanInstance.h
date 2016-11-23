#pragma once 

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

#include "LsVulkanCommon.h"

bool CheckInstanceLayers(std::vector<const char*> const& requiredLayers) {
  uint32_t availableLayerCount;
  vk::enumerateInstanceLayerProperties(&availableLayerCount, nullptr);
  std::vector<vk::LayerProperties> availableLayers(availableLayerCount);
  vk::enumerateInstanceLayerProperties(&availableLayerCount, &availableLayers[0]);
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
  vk::Result result = vk::enumerateInstanceExtensionProperties(
    nullptr, 
    &availableExtensionsCount,
    nullptr );

  if ( result != vk::Result::eSuccess || availableExtensionsCount == 0 ) {
    throw std::string("Error occurred during instance extension enumeration!");
  }

  std::vector<vk::ExtensionProperties> availableExtensions( availableExtensionsCount );

  result = vk::enumerateInstanceExtensionProperties(
    nullptr, 
    &availableExtensionsCount,
    availableExtensions.data());

  availableExtensions.resize(availableExtensionsCount);

  if( result != vk::Result::eSuccess ) {
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
                     vk::Instance* instance ) {
  if ( !CheckInstanceExtensions(extensions) ) {
    throw std::string("Instance missing required extensions");
  }

  if ( !CheckInstanceLayers(layers) ) {
    throw std::string("Instance missing required layers");
  }

  vk::InstanceCreateInfo instanceCreateInfo(
    vk::InstanceCreateFlags(),
    NULL, // appliactionInfo field is optional
    static_cast<uint32_t>(layers.size()),
    layers.data(),
    static_cast<uint32_t>(extensions.size()),
    extensions.data() );

  if ( createInstance( &instanceCreateInfo, nullptr, instance) != vk::Result::eSuccess ) {
    throw std::string( "Failed to create Vulkan instance!" );
  }
}
