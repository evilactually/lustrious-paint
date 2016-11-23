#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

#include "LsPushConstantTypes.h"

bool CheckPhysicalDeviceExtensions(vk::PhysicalDevice physicalDevice, std::vector<const char*> const& requiredExtensions) {
  // No extensions were requested
  if (requiredExtensions.size() == 0) {
    return true;
  }

  uint32_t availableExtensionsCount = 0;
  vk::Result result = physicalDevice.enumerateDeviceExtensionProperties( nullptr, &availableExtensionsCount, nullptr );
  if( result != vk::Result::eSuccess || availableExtensionsCount == 0 ) {
    return false;
  }

  std::vector<vk::ExtensionProperties> availableExtensions( availableExtensionsCount );
  result = physicalDevice.enumerateDeviceExtensionProperties( nullptr, &availableExtensionsCount, &availableExtensions[0] );
  if( result != vk::Result::eSuccess ) {
    return false;
  }

  for(auto requiredExtension:requiredExtensions) {
    if( !CheckExtensionAvailability(requiredExtension, availableExtensions) ) {
      std::cout << "Required device extension missing: " << requiredExtension << std::endl;
      return false;
    }
  }

  // All extensions must have been found by this point
  return true;
}

bool CheckPhysicalDeviceProperties(vk::PhysicalDevice physicalDevice) {
  vk::PhysicalDeviceProperties device_properties;
  physicalDevice.getProperties(&device_properties);

  uint32_t major_version = VK_VERSION_MAJOR( device_properties.apiVersion );
  uint32_t minor_version = VK_VERSION_MINOR( device_properties.apiVersion );
  uint32_t patch_version = VK_VERSION_PATCH( device_properties.apiVersion );
  if ( major_version < 1 ) {
    return false;
  }

  if ( device_properties.limits.maxImageDimension2D < 4096 ) {
    return false;
  }
  
  uint32_t maxPushConstantsSize = device_properties.limits.maxPushConstantsSize;
  if ( maxPushConstantsSize < sizeof(PointPushConstants) ||
       maxPushConstantsSize < sizeof(LinePushConstants) ) {
    std::cout << "Maximum push constant size is too small: " << maxPushConstantsSize << std::endl;
    return false;
  }

  return true;
}

bool CheckPhysicalDeviceFeatures(vk::PhysicalDevice physicalDevice) {
  vk::PhysicalDeviceFeatures device_features;
  physicalDevice.getFeatures(&device_features);
  return device_features.wideLines = VK_TRUE;
}

bool FindQueueFamilies(vk::PhysicalDevice physicalDevice, 
                       vk::SurfaceKHR presentationSurface,
                       uint32_t* selected_graphics_queue_family_index,
                       uint32_t* selected_present_queue_family_index) {
  uint32_t queue_families_count = 0;
  physicalDevice.getQueueFamilyProperties( &queue_families_count, nullptr );
  if( queue_families_count == 0 ) {
      return false;
  }

  std::vector<vk::QueueFamilyProperties> queue_family_properties( queue_families_count );
  std::vector<VkBool32>                  queue_present_support( queue_families_count );

  physicalDevice.getQueueFamilyProperties( &queue_families_count, &queue_family_properties[0] );

  uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
  uint32_t presentQueueFamilyIndex = UINT32_MAX;

  for( uint32_t i = 0; i < queue_families_count; ++i ) {
      physicalDevice.getSurfaceSupportKHR( i, presentationSurface, &queue_present_support[i] );

      if( (queue_family_properties[i].queueCount > 0) &&
          (queue_family_properties[i].queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer)) ) {
          // Select first queue that supports graphics
          if( graphicsQueueFamilyIndex == UINT32_MAX ) {
              graphicsQueueFamilyIndex = i;
          }
          // If there is queue that supports both graphics and present - prefer it
          if( queue_present_support[i] ) {
              *selected_graphics_queue_family_index = i;
              *selected_present_queue_family_index = i;
              std::cout << "Graphics queue family index " << i << std::endl;
              std::cout << "Present queue family index " << i << std::endl;
              return true;
          }
      }
  }

  // We don't have queue that supports both graphics and present so we have to use separate queues
  for( uint32_t i = 0; i < queue_families_count; ++i ) {
      if( queue_present_support[i] ) {
          presentQueueFamilyIndex = i;
          break;
      }
  }

  // If this device doesn't support queues with graphics and present capabilities don't use it
  if( (graphicsQueueFamilyIndex == UINT32_MAX) ||
      (presentQueueFamilyIndex == UINT32_MAX) ) {
    return false;
  }

  *selected_graphics_queue_family_index = graphicsQueueFamilyIndex;
  *selected_present_queue_family_index = presentQueueFamilyIndex;
  std::cout << "Graphics queue family index " << graphicsQueueFamilyIndex << std::endl;
  std::cout << "Present queue family index " << presentQueueFamilyIndex << std::endl;
  return true;
}

bool TryCreateDevice(vk::PhysicalDevice physicalDevice,
                     vk::SurfaceKHR presentationSurface,
                     std::vector<const char*> const& extensions,
                     vk::Device* device,
                     uint32_t* graphicsQueueFamily,
                     uint32_t* presentQueueFamily) {
  uint32_t selectedGraphicsQueueFamily;
  uint32_t selectedPresentQueueFamily;
  if ( CheckPhysicalDeviceExtensions(physicalDevice, extensions) &&
       CheckPhysicalDeviceProperties(physicalDevice) &&
       CheckPhysicalDeviceFeatures(physicalDevice) &&
       FindQueueFamilies(physicalDevice, presentationSurface, &selectedGraphicsQueueFamily, &selectedPresentQueueFamily) ) {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::vector<float> queuePriorities = { 1.0f }; // This is required, even for single queue
    queueCreateInfos.push_back({vk::DeviceQueueCreateFlags(),  // vk::DeviceQueueCreateFlags   flags
                                selectedGraphicsQueueFamily,   // uint32_t                     queueFamilyIndex
                                1,                             // uint32_t                     queueCount
                                queuePriorities.data()});      // const float                  *pQueuePriorities
    
    // Create present queue if it is from separate queue family
    if ( selectedPresentQueueFamily != selectedGraphicsQueueFamily ) {
      queueCreateInfos.push_back({vk::DeviceQueueCreateFlags(),// vk::DeviceQueueCreateFlags   flags
                                  selectedPresentQueueFamily,  // uint32_t                     queueFamilyIndex
                                  1,                           // uint32_t                     queueCount
                                  queuePriorities.data()});    // const float                  *pQueuePriorities
    }

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.wideLines = VK_TRUE;

    vk::DeviceCreateInfo deviceCreateInfo(
      vk::DeviceCreateFlags(),                                 // vk::DeviceCreateFlags              flags
      static_cast<uint32_t>(queueCreateInfos.size()),          // uint32_t                           queueCreateInfoCount
      queueCreateInfos.data(),                                 // const VkDeviceQueueCreateInfo      *pQueueCreateInfos
      0,                                                       // uint32_t                           enabledLayerCount
      nullptr,                                                 // const char * const                 *ppEnabledLayerNames
      static_cast<uint32_t>(extensions.size()),                 // uint32_t                           enabledExtensionCount
      extensions.data(),                                        // const char * const                 *ppEnabledExtensionNames
      &deviceFeatures);                                        // const vk::PhysicalDeviceFeatures   *pEnabledFeatures

    if( physicalDevice.createDevice( &deviceCreateInfo, nullptr, device ) != vk::Result::eSuccess ) {
      throw std::string("Could not create Vulkan device!");
    }

    *graphicsQueueFamily = selectedGraphicsQueueFamily;
    *presentQueueFamily = selectedPresentQueueFamily;

    return true;
  }
  return false;
}

void CreateDevice(vk::Instance instance,
                  vk::SurfaceKHR presentationSurface,
                  std::vector<const char*> const& extensions,
                  vk::PhysicalDevice* physicalDevice,
                  vk::Device* device,
                  uint32_t* graphicsQueueFamily,
                  uint32_t* presentQueueFamily) {
  uint32_t physicalDeviceCount = 0;
  vk::Result result = instance.enumeratePhysicalDevices(&physicalDeviceCount, NULL);
  if( result != vk::Result::eSuccess || physicalDeviceCount == 0 ) {
    throw std::string("Error occurred during physical devices enumeration!");
  }

  std::vector<vk::PhysicalDevice> physicalDevices( physicalDeviceCount );
  result = instance.enumeratePhysicalDevices(&physicalDeviceCount, physicalDevices.data());
  if( result != vk::Result::eSuccess || physicalDeviceCount == 0 ) {
    throw std::string("Error occurred during physical devices enumeration!");
  }

  for( uint32_t i = 0; i < physicalDeviceCount; ++i ) {
    if( TryCreateDevice(physicalDevices[i], presentationSurface, extensions, device, graphicsQueueFamily, presentQueueFamily) ) {
      *physicalDevice = physicalDevices[i];
      return;
    }
  }

  throw std::string("No device found!");
}