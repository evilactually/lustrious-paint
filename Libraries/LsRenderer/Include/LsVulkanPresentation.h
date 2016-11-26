#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>
#include <LsOptional.h>

void CreatePresentationSurface(vk::Instance instance, HINSTANCE hInstance, HWND windowHandle, vk::SurfaceKHR* presentationSurface) {
  vk::Win32SurfaceCreateInfoKHR surface_create_info(
    vk::Win32SurfaceCreateFlagsKHR(), // vk::Win32SurfaceCreateFlagsKHR   flags
    hInstance,                        // HINSTANCE                        hinstance
    windowHandle                      // HWND                             hwnd
  );

  if( instance.createWin32SurfaceKHR( &surface_create_info, nullptr, presentationSurface ) != vk::Result::eSuccess ) {
    throw std::string("Could not create presentation surface!");
  }
}

vk::SurfaceFormatKHR GetSwapChainFormat( std::vector<vk::SurfaceFormatKHR> &surface_formats ) {
  // If the list contains only one entry with undefined format
  // it means that there are no preferred surface formats and any can be chosen
  if( surface_formats.size() == 1 &&
      surface_formats[0].format == vk::Format::eUndefined ) {
    return {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
  }

  // Check if list contains most widely used R8 G8 B8 A8 format
  // with nonlinear color space
  for( vk::SurfaceFormatKHR &surface_format : surface_formats ) {
    if( surface_format.format == vk::Format::eR8G8B8A8Unorm ) {
      return surface_format;
    }
  }

  // Return the first format from the list
  return surface_formats[0];
}

LsOptional<vk::Extent2D> GetSwapChainExtent( vk::SurfaceCapabilitiesKHR &surface_capabilities, HWND windowHandle ) {
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
    LsOptional<vk::Extent2D>::None();
  }
  
  // Most of the cases we define size of the swap_chain images equal to current window's size
  return surface_capabilities.currentExtent;
}

LsOptional<vk::ImageUsageFlags> GetSwapChainUsageFlags( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
  // Color attachment flag must always be supported, don't have to check
  // We can define other usage flags but we always need to check if they are supported
  vk::ImageUsageFlags additionalImageUsageFlags = vk::ImageUsageFlagBits::eTransferDst;

  if( surface_capabilities.supportedUsageFlags & additionalImageUsageFlags ) {
    return vk::ImageUsageFlagBits::eColorAttachment | additionalImageUsageFlags;
  }
  std::cout << "Image usage flags are not supported by the swap chain:" << vk::to_string(additionalImageUsageFlags) << std::endl;
  return LsOptional<vk::ImageUsageFlags>::None();
}

vk::SurfaceTransformFlagBitsKHR GetSwapChainTransform( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
  if( surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ) {
    return vk::SurfaceTransformFlagBitsKHR::eIdentity;
  } else {
    return surface_capabilities.currentTransform;
  }
}

LsOptional<vk::PresentModeKHR> GetSwapChainPresentMode( std::vector<vk::PresentModeKHR> &present_modes ) {
  for( vk::PresentModeKHR &present_mode : present_modes ) {
    if( present_mode == vk::PresentModeKHR::eImmediate ) {
      std::cout << "Using immediate present mode" << std::endl; 
      return present_mode;
    }
  }
  // FIFO present mode is always available
  // MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
  for( vk::PresentModeKHR &present_mode : present_modes ) {
    if( present_mode == vk::PresentModeKHR::eMailbox ) {
      std::cout << "Using mailbox present mode" << std::endl; 
      return present_mode;
    }
  }
  for( vk::PresentModeKHR &present_mode : present_modes ) {
    if( present_mode == vk::PresentModeKHR::eFifo ) {
      std::cout << "Using fifo present mode" << std::endl; 
      return present_mode;
    }
  }
  return LsOptional<vk::PresentModeKHR>();
}

uint32_t GetSwapChainNumImages( vk::SurfaceCapabilitiesKHR &surface_capabilities, vk::PresentModeKHR chosen_present_mode) {
  uint32_t image_count;

  // select desired image cound based on present mode
  switch (chosen_present_mode) {
    case vk::PresentModeKHR::eMailbox:
    image_count = 3;
    break;
    case vk::PresentModeKHR::eFifo:
    image_count = 3;
    break;
    case vk::PresentModeKHR::eFifoRelaxed:
    image_count = 3;
    break;
    case vk::PresentModeKHR::eImmediate:
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

bool CreateSwapChain(vk::PhysicalDevice& physicalDevice,
                     vk::Device& device,
                     vk::SurfaceKHR& presentationSurface,
                     HWND windowHandle,
                     vk::SwapchainKHR* swapChain,
                     vk::Format* swapChainFormat,
                     vk::Extent2D* swapChainExtent) {
  device.waitIdle();

  vk::SurfaceCapabilitiesKHR surface_capabilities;
  if( physicalDevice.getSurfaceCapabilitiesKHR(presentationSurface, &surface_capabilities) != vk::Result::eSuccess ) {
    throw std::string("Could not check presentation surface capabilities!");
  }

  uint32_t formats_count;
  if( physicalDevice.getSurfaceFormatsKHR( presentationSurface, &formats_count, nullptr ) != vk::Result::eSuccess ||
      formats_count == 0 ) {
    throw std::string("Error occurred during presentation surface formats enumeration!");
  }

  std::vector<vk::SurfaceFormatKHR> surface_formats( formats_count );
  if( physicalDevice.getSurfaceFormatsKHR( presentationSurface, &formats_count, surface_formats.data() ) != vk::Result::eSuccess ) {
    throw std::string("Error occurred during presentation surface formats enumeration!");
  }

  uint32_t present_modes_count;
  if( physicalDevice.getSurfacePresentModesKHR( presentationSurface, &present_modes_count, nullptr ) != vk::Result::eSuccess ||
      present_modes_count == 0 ) {
    throw std::string("Error occurred during presentation surface present modes enumeration!");
  }

  std::vector<vk::PresentModeKHR> present_modes( present_modes_count );
  if( physicalDevice.getSurfacePresentModesKHR( presentationSurface, &present_modes_count, &present_modes[0] ) != vk::Result::eSuccess ) {
    throw std::string("Error occurred during presentation surface present modes enumeration!");
  }

  vk::SurfaceFormatKHR                desired_format = GetSwapChainFormat( surface_formats );
  LsOptional<vk::Extent2D>            desired_extent = GetSwapChainExtent( surface_capabilities, windowHandle );
  LsOptional<vk::ImageUsageFlags>     desired_usage = GetSwapChainUsageFlags( surface_capabilities );
  vk::SurfaceTransformFlagBitsKHR     desired_transform = GetSwapChainTransform( surface_capabilities );
  LsOptional<vk::PresentModeKHR>      desired_present_mode = GetSwapChainPresentMode( present_modes );
  vk::SwapchainKHR                    old_swapchain = *swapChain;

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

  vk::SwapchainCreateInfoKHR swap_chain_create_info(
    vk::SwapchainCreateFlagsKHR(),                // vk::SwapchainCreateFlagsKHR      flags
    presentationSurface,                          // vk::SurfaceKHR                   surface
    desired_number_of_images,                     // uint32_t                         minImageCount
    desired_format.format,                        // vk::Format                       imageFormat
    desired_format.colorSpace,                    // vk::ColorSpaceKHR                imageColorSpace
    desired_extent,                               // vk::Extent2D                     imageExtent
    1,                                            // uint32_t                         imageArrayLayers
    desired_usage,                                // vk::ImageUsageFlags              imageUsage
    vk::SharingMode::eExclusive,                  // vk::SharingMode                  imageSharingMode
    0,                                            // uint32_t                         queueFamilyIndexCount
    nullptr,                                      // const uint32_t                   *pQueueFamilyIndices
    desired_transform,                            // vk::SurfaceTransformFlagBitsKHR  preTransform
    vk::CompositeAlphaFlagBitsKHR::eOpaque,       // vk::CompositeAlphaFlagBitsKHR    compositeAlpha
    desired_present_mode,                         // vk::PresentModeKHR               presentMode
    VK_TRUE,                                      // vk::Bool32                       clipped
    old_swapchain                                 // vk::SwapchainKHR                 oldSwapchain
    );

  if( device.createSwapchainKHR( &swap_chain_create_info, nullptr, swapChain ) != vk::Result::eSuccess ) {
    throw std::string("Could not create swap chain!");
  }

  if( old_swapchain ) {
    device.destroySwapchainKHR( old_swapchain, nullptr );
  }

  *swapChainFormat = desired_format.format; // for creating attachment, image views, etc
  *swapChainExtent = desired_extent;        // for framebuffers

  return true;
}

std::vector<vk::Image> GetSwapChainImages(vk::Device& device, vk::SwapchainKHR& swapChain) {
  uint32_t image_count = 0;
  if( device.getSwapchainImagesKHR( swapChain, &image_count, nullptr ) != vk::Result::eSuccess ||
      image_count == 0 ) {
    throw std::string("Could not get the number of swap chain images!");
  }

  std::vector<vk::Image> images(image_count);
  if( device.getSwapchainImagesKHR( swapChain, &image_count, images.data() ) != vk::Result::eSuccess ) {
    throw std::string("Could not get swap chain images!");
  }

  images.resize(image_count);
  
  return images;
}

std::vector<vk::ImageView> CreateSwapChainImageViews(vk::Device& device, std::vector<vk::Image>& images, vk::Format format) {
  std::vector<vk::ImageView> imageViews;

  for(auto image:images) {
    vk::ImageViewCreateInfo image_view_create_info(
      vk::ImageViewCreateFlags(),        // VkImageViewCreateFlags         flags
      image,                             // VkImage                        image
      vk::ImageViewType::e2D,            // VkImageViewType                viewType
      format,                            // VkFormat                       format
      {                                  // VkComponentMapping             components
        vk::ComponentSwizzle::eIdentity, // VkComponentSwizzle             r
        vk::ComponentSwizzle::eIdentity, // VkComponentSwizzle             g
        vk::ComponentSwizzle::eIdentity, // VkComponentSwizzle             b
        vk::ComponentSwizzle::eIdentity  // VkComponentSwizzle             a
      },
      {                                  // VkImageSubresourceRange        subresourceRange
        vk::ImageAspectFlagBits::eColor, // VkImageAspectFlags             aspectMask
        0,                               // uint32_t                       baseMipLevel
        1,                               // uint32_t                       levelCount
        0,                               // uint32_t                       baseArrayLayer
        1                                // uint32_t                       layerCount
      }
    );

    vk::ImageView imageView;
    if( device.createImageView( &image_view_create_info, nullptr, &imageView ) != vk::Result::eSuccess ) {
      throw std::string("Could not create image views!");
    }

    imageViews.push_back(imageView);
  }

  return imageViews;
}
