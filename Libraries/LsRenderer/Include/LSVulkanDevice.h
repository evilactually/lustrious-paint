#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

#include "LsPushConstantTypes.h"

bool CheckPhysicalDeviceExtensions(VkPhysicalDevice physicalDevice, std::vector<const char*> const& requiredExtensions) {
  // No extensions were requested
  if (requiredExtensions.size() == 0) {
    return true;
  }

  uint32_t availableExtensionsCount = 0;
  VkResult result = vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &availableExtensionsCount, nullptr );
  if( result != VK_SUCCESS || availableExtensionsCount == 0 ) {
    return false;
  }

  std::vector<VkExtensionProperties> availableExtensions( availableExtensionsCount );
  result = vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &availableExtensionsCount, &availableExtensions[0] );
  if( result != VK_SUCCESS ) {
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

bool CheckPhysicalDeviceProperties(VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties(physicalDevice, &device_properties);

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

bool CheckPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceFeatures device_features;
  vkGetPhysicalDeviceFeatures(physicalDevice, &device_features);
  return device_features.wideLines = VK_TRUE;
}

bool FindQueueFamilies(VkPhysicalDevice physicalDevice, 
                       VkSurfaceKHR presentationSurface,
                       uint32_t* selected_graphics_queue_family_index,
                       uint32_t* selected_present_queue_family_index) {
  uint32_t queue_families_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queue_families_count, nullptr );
  if( queue_families_count == 0 ) {
      return false;
  }

  std::vector<VkQueueFamilyProperties> queue_family_properties( queue_families_count );
  std::vector<VkBool32>                  queue_present_support( queue_families_count );

  vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queue_families_count, &queue_family_properties[0] );

  uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
  uint32_t presentQueueFamilyIndex = UINT32_MAX;

  for( uint32_t i = 0; i < queue_families_count; ++i ) {
      vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, i, presentationSurface, &queue_present_support[i] );

      if( (queue_family_properties[i].queueCount > 0) &&
          (queue_family_properties[i].queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT )) ) {
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

bool FindComputeQueueFamily(VkPhysicalDevice physicalDevice, uint32_t* computeQueueFamily)
{
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
  assert(queueFamilyCount >= 1);

  std::vector<VkQueueFamilyProperties> queueFamilyProperties;
  queueFamilyProperties.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    // Some devices have dedicated compute queues, so we first try to find a queue that supports compute and not graphics
  bool computeQueueFound = false;
  for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
  {
    if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
    {
      *computeQueueFamily = i;
      computeQueueFound = true;
      return true;
      break;
    }
  }

  // If there is no dedicated compute queue, just find the first queue family that supports compute
  if (!computeQueueFound)
  {
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
    {
      if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
      {
        *computeQueueFamily = i;
        computeQueueFound = true;
        return true;
        break;
      }
    }
  } 

  return false;
}

bool TryCreateDevice(VkPhysicalDevice physicalDevice,
                     VkSurfaceKHR presentationSurface,
                     std::vector<const char*> const& extensions,
                     VkDevice* device,
                     uint32_t* graphicsQueueFamily,
                     uint32_t* presentQueueFamily,
                     uint32_t* computeQueueFamily) {
  uint32_t selectedGraphicsQueueFamily;
  uint32_t selectedPresentQueueFamily;
  if ( CheckPhysicalDeviceExtensions(physicalDevice, extensions) &&
       CheckPhysicalDeviceProperties(physicalDevice) &&
       CheckPhysicalDeviceFeatures(physicalDevice) &&
       FindQueueFamilies(physicalDevice, presentationSurface, &selectedGraphicsQueueFamily, &selectedPresentQueueFamily) &&
       FindComputeQueueFamily(physicalDevice, computeQueueFamily) ) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<float> queuePriorities = { 1.0f }; // This is required, even for single queue
	
    // Fill out common fields
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pQueuePriorities = queuePriorities.data();
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pNext = NULL;
	
	// Push graphics queue create request
	queueCreateInfo.queueFamilyIndex = selectedGraphicsQueueFamily;
	queueCreateInfos.push_back(queueCreateInfo);
    
    // Create present queue if it is from separate queue family
    if ( selectedPresentQueueFamily != selectedGraphicsQueueFamily ) {
	  // Push present queue create request
   	  queueCreateInfo.queueFamilyIndex = selectedPresentQueueFamily;
      queueCreateInfos.push_back(queueCreateInfo);
    }

  // Add compute family queue
  queueCreateInfo.queueFamilyIndex = *computeQueueFamily;
  queueCreateInfos.push_back(queueCreateInfo);

	VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.wideLines = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	
    if( vkCreateDevice( physicalDevice, &deviceCreateInfo, nullptr, device ) != VK_SUCCESS ) {
      throw std::string("Could not create Vulkan device!");
    }

    *graphicsQueueFamily = selectedGraphicsQueueFamily;
    *presentQueueFamily = selectedPresentQueueFamily;

    return true;
  }
  return false;
}

void CreateDevice(VkInstance instance,
                  VkSurfaceKHR presentationSurface,
                  std::vector<const char*> const& extensions,
                  VkPhysicalDevice* physicalDevice,
                  VkDevice* device,
                  uint32_t* graphicsQueueFamily,
                  uint32_t* presentQueueFamily,
                  uint32_t* computeQueueFamily) {
  uint32_t physicalDeviceCount = 0;
  VkResult result = vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, NULL);
  if( result != VK_SUCCESS || physicalDeviceCount == 0 ) {
    throw std::string("Error occurred during physical devices enumeration!");
  }

  std::vector<VkPhysicalDevice> physicalDevices( physicalDeviceCount );
  result = vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, physicalDevices.data());
  if( result != VK_SUCCESS || physicalDeviceCount == 0 ) {
    throw std::string("Error occurred during physical devices enumeration!");
  }

  for( uint32_t i = 0; i < physicalDeviceCount; ++i ) {
    if( TryCreateDevice(physicalDevices[i], presentationSurface, extensions, device, graphicsQueueFamily, presentQueueFamily, computeQueueFamily) ) {
      *physicalDevice = physicalDevices[i];
      return;
    }
  }

  throw std::string("No device found!");
}