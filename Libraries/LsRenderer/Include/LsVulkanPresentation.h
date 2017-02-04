#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>
#include <LsOptional.h>

void CreatePresentationSurface(VkInstance instance, HINSTANCE hInstance, HWND windowHandle, VkSurfaceKHR* presentationSurface) {
  VkWin32SurfaceCreateInfoKHR surface_create_info = {};
  surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  surface_create_info.hinstance = hInstance;
  surface_create_info.hwnd = windowHandle;

  if( vkCreateWin32SurfaceKHR( instance, &surface_create_info, nullptr, presentationSurface ) != VK_SUCCESS ) {
    throw std::string("Could not create presentation surface!");
  }
}

VkSurfaceFormatKHR GetSwapChainFormat( std::vector<VkSurfaceFormatKHR> &surface_formats ) {
  // If the list contains only one entry with undefined format
  // it means that there are no preferred surface formats and any can be chosen
  if( surface_formats.size() == 1 &&
      surface_formats[0].format == VK_FORMAT_UNDEFINED ) {
    return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
  }

  // Check if list contains most widely used R8 G8 B8 A8 format
  // with nonlinear color space
  for( VkSurfaceFormatKHR &surface_format : surface_formats ) {
    if( surface_format.format == VK_COLORSPACE_SRGB_NONLINEAR_KHR ) {
      return surface_format;
    }
  }

  // Return the first format from the list
  return surface_formats[0];
}

LsOptional<VkExtent2D> GetSwapChainExtent( VkSurfaceCapabilitiesKHR &surface_capabilities, HWND windowHandle ) {
  // Special value of surface extent is width == height == -1
  // If this is so we define the size by ourselves but it must fit within defined confines
  if( surface_capabilities.currentExtent.width == -1) {
    // Try setting extent to the size of window client area
    RECT window_rect = {}; 
    GetClientRect(windowHandle, &window_rect);
    VkExtent2D swap_chain_extent = { static_cast<uint32_t>(window_rect.right - window_rect.left),
    static_cast<uint32_t>(window_rect.bottom - window_rect.top) };

    if( swap_chain_extent.width < surface_capabilities.minImageExtent.width ) {
      swap_chain_extent.width = surface_capabilities.minImageExtent.width;
    }
    if( swap_chain_extent.height < surface_capabilities.minImageExtent.height ) {
      swap_chain_extent.height = surface_capabilities.minImageExtent.height;
    }
    if( swap_chain_extent.width > surface_capabilities.maxImageExtent.width ) {
      swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
    }
    if( swap_chain_extent.height > surface_capabilities.maxImageExtent.height ) {
      swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
    }
    return swap_chain_extent;
  }

  if (surface_capabilities.currentExtent.width == 0 || surface_capabilities.currentExtent.height == 0 ) {
    LsOptional<VkExtent2D>::None();
  }
  
  // Most of the cases we define size of the swap_chain images equal to current window's size
  return surface_capabilities.currentExtent;
}

LsOptional<VkImageUsageFlags> GetSwapChainUsageFlags( VkSurfaceCapabilitiesKHR &surface_capabilities ) {
  // Color attachment flag must always be supported, don't have to check
  // We can define other usage flags but we always need to check if they are supported
  VkImageUsageFlags additionalImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  if( surface_capabilities.supportedUsageFlags & additionalImageUsageFlags ) {
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | additionalImageUsageFlags;
  }
  std::cout << "Image usage flags are not supported by the swap chain." << std::endl;
  return LsOptional<VkImageUsageFlags>::None();
}

VkSurfaceTransformFlagBitsKHR GetSwapChainTransform( VkSurfaceCapabilitiesKHR &surface_capabilities ) {
  if( surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) {
    return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    return surface_capabilities.currentTransform;
  }
}

LsOptional<VkPresentModeKHR> GetSwapChainPresentMode( std::vector<VkPresentModeKHR> &present_modes ) {
  for( VkPresentModeKHR &present_mode : present_modes ) {
    if( present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR ) {
      std::cout << "Using immediate present mode" << std::endl; 
      return present_mode;
    }
  }
  // FIFO present mode is always available
  // MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
  for( VkPresentModeKHR &present_mode : present_modes ) {
    if( present_mode == VK_PRESENT_MODE_MAILBOX_KHR ) {
      std::cout << "Using mailbox present mode" << std::endl; 
      return present_mode;
    }
  }
  for( VkPresentModeKHR &present_mode : present_modes ) {
    if( present_mode == VK_PRESENT_MODE_FIFO_KHR ) {
      std::cout << "Using fifo present mode" << std::endl; 
      return present_mode;
    }
  }
  return LsOptional<VkPresentModeKHR>();
}

uint32_t GetSwapChainNumImages( VkSurfaceCapabilitiesKHR &surface_capabilities, VkPresentModeKHR chosen_present_mode) {
  uint32_t image_count;

  // select desired image count based on present mode
  switch (chosen_present_mode) {
    case VK_PRESENT_MODE_MAILBOX_KHR:
    image_count = 3;
    break;
    case VK_PRESENT_MODE_FIFO_KHR:
    image_count = 3;
    break;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    image_count = 3;
    break;
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
    image_count = 2;
    break;
  }

  std::cout << "Using image count " << image_count << " for swap chain" << std::endl;

  // clamp image count to fit surface capabilites
  if (image_count < surface_capabilities.minImageCount) {
    image_count = surface_capabilities.minImageCount;
  } else if ( surface_capabilities.maxImageCount != 0 && 
    image_count > surface_capabilities.maxImageCount ) {
    image_count = surface_capabilities.maxImageCount;
  }
  return image_count;
}

bool CreateSwapChain(VkPhysicalDevice& physicalDevice,
                     VkDevice& device,
                     VkSurfaceKHR& presentationSurface,
                     HWND windowHandle,
                     VkSwapchainKHR* swapChain,
                     VkFormat* swapChainFormat,
                     VkExtent2D* swapChainExtent) {
  vkDeviceWaitIdle(device);

  VkSurfaceCapabilitiesKHR surface_capabilities;
  if( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physicalDevice, presentationSurface, &surface_capabilities) != VK_SUCCESS ) {
    throw std::string("Could not check presentation surface capabilities!");
  }

  uint32_t formats_count;
  if( vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, presentationSurface, &formats_count, nullptr ) != VK_SUCCESS ||
      formats_count == 0 ) {
    throw std::string("Error occurred during presentation surface formats enumeration!");
  }

  std::vector<VkSurfaceFormatKHR> surface_formats( formats_count );
  if( vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, presentationSurface, &formats_count, surface_formats.data() ) != VK_SUCCESS ) {
    throw std::string("Error occurred during presentation surface formats enumeration!");
  }

  uint32_t present_modes_count;
  if( vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, presentationSurface, &present_modes_count, nullptr ) != VK_SUCCESS ||
      present_modes_count == 0 ) {
    throw std::string("Error occurred during presentation surface present modes enumeration!");
  }

  std::vector<VkPresentModeKHR> present_modes( present_modes_count );
  if( vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, presentationSurface, &present_modes_count, &present_modes[0] ) != VK_SUCCESS ) {
    throw std::string("Error occurred during presentation surface present modes enumeration!");
  }

  VkSurfaceFormatKHR             desired_format = GetSwapChainFormat( surface_formats );
  LsOptional<VkExtent2D>         desired_extent = GetSwapChainExtent( surface_capabilities, windowHandle );
  LsOptional<VkImageUsageFlags>  desired_usage = GetSwapChainUsageFlags( surface_capabilities );
  VkSurfaceTransformFlagBitsKHR  desired_transform = GetSwapChainTransform( surface_capabilities );
  LsOptional<VkPresentModeKHR>   desired_present_mode = GetSwapChainPresentMode( present_modes );
  VkSwapchainKHR                 old_swapchain = *swapChain;

  if( !desired_usage ) {
    throw std::string("Surface does not support any suitable usage flags!");
  }
  if( !desired_present_mode ) {
    throw std::string("Surface does not support any suitable present modes!");
  }

  uint32_t desired_number_of_images = GetSwapChainNumImages( surface_capabilities, desired_present_mode );
  
  if( !desired_extent ) {
    // Current surface size is (0, 0) so we can't create a swap chain and render anything
    // But we don't wont to kill the application as this situation may occur i.e. when window gets minimized
    return false;
  }

  VkSwapchainCreateInfoKHR swap_chain_create_info = {};
  swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swap_chain_create_info.flags = 0;
  swap_chain_create_info.surface = presentationSurface;
  swap_chain_create_info.minImageCount = desired_number_of_images;
  swap_chain_create_info.imageFormat = desired_format.format;
  swap_chain_create_info.imageColorSpace = desired_format.colorSpace;
  swap_chain_create_info.imageExtent = desired_extent;
  swap_chain_create_info.imageArrayLayers = 1;
  swap_chain_create_info.imageUsage = desired_usage;
  swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swap_chain_create_info.queueFamilyIndexCount = 0;
  swap_chain_create_info.pQueueFamilyIndices = nullptr;
  swap_chain_create_info.preTransform = desired_transform;
  swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swap_chain_create_info.presentMode = desired_present_mode;
  swap_chain_create_info.clipped = VK_TRUE;
  swap_chain_create_info.oldSwapchain = old_swapchain;

  if( vkCreateSwapchainKHR( device, &swap_chain_create_info, nullptr, swapChain ) != VK_SUCCESS ) {
    throw std::string("Could not create swap chain!");
  }

  if( old_swapchain ) {
    vkDestroySwapchainKHR( device, old_swapchain, nullptr );
  }

  *swapChainFormat = desired_format.format; // for creating attachment, image views, etc
  *swapChainExtent = desired_extent;        // for framebuffers

  return true;
}

std::vector<VkImage> GetSwapChainImages(VkDevice& device, VkSwapchainKHR& swapChain) {
  uint32_t image_count = 0;
  if( vkGetSwapchainImagesKHR( device, swapChain, &image_count, nullptr ) != VK_SUCCESS ||
      image_count == 0 ) {
    throw std::string("Could not get the number of swap chain images!");
  }

  std::vector<VkImage> images(image_count);
  if(vkGetSwapchainImagesKHR( device, swapChain, &image_count, images.data() ) != VK_SUCCESS ) {
    throw std::string("Could not get swap chain images!");
  }

  images.resize(image_count);
  
  return images;
}

std::vector<VkImageView> CreateSwapChainImageViews(VkDevice& device, std::vector<VkImage>& images, VkFormat format) {
  std::vector<VkImageView> imageViews;

  for(auto image:images) {
	  VkImageViewCreateInfo image_view_create_info = {};
	  image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	  image_view_create_info.flags = 0;
	  image_view_create_info.image = image;
	  image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	  image_view_create_info.format = format;
	  image_view_create_info.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY, // VkComponentSwizzle             r
		VK_COMPONENT_SWIZZLE_IDENTITY, // VkComponentSwizzle             g
		VK_COMPONENT_SWIZZLE_IDENTITY, // VkComponentSwizzle             b
		VK_COMPONENT_SWIZZLE_IDENTITY  // VkComponentSwizzle             a
	  };
	  image_view_create_info.subresourceRange = {
		VK_IMAGE_ASPECT_COLOR_BIT,     // VkImageAspectFlags             aspectMask
		0,                             // uint32_t                       baseMipLevel
		1,                             // uint32_t                       levelCount
		0,                             // uint32_t                       baseArrayLayer
		1                              // uint32_t                       layerCount
	  };

    VkImageView imageView;
    if( vkCreateImageView( device, &image_view_create_info, nullptr, &imageView ) != VK_SUCCESS ) {
      throw std::string("Could not create image views!");
    }

    imageViews.push_back(imageView);
  }

  return imageViews;
}
