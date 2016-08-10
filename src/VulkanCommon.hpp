#pragma once

#include "Vk.hpp"
#include <vector>

// ************************************************************ //
// QueueParameters                                              //
//                                                              //
// Vulkan Queue's parameters container class                    //
// ************************************************************ //
struct QueueParameters {
    vk::Queue                     Handle;
    uint32_t                      FamilyIndex;

    QueueParameters():
        Handle(),
        FamilyIndex(0) {}
};

// ************************************************************ //
// ImageParameters                                              //
//                                                              //
// Vulkan Image's parameters container class                    //
// ************************************************************ //
struct ImageParameters {
    vk::Image                       Handle;
    vk::ImageView                   View;
    vk::Sampler                     Sampler;
    vk::DeviceMemory                Memory;

    ImageParameters():
        Handle(),
        View(),
        Sampler(),
        Memory() {}
};

// ************************************************************ //
// SwapChainParameters                                          //
//                                                              //
// Vulkan SwapChain's parameters container class                //
// ************************************************************ //
struct SwapChainParameters {
    vk::SwapchainKHR                  Handle;
    vk::Format                        Format;
    std::vector<ImageParameters>      Images;
    vk::Extent2D                      Extent;

    SwapChainParameters():
        Handle(),
        Format(vk::Format::eUndefined),
        Images(),
        Extent() {}
};

// ************************************************************ //
// VulkanCommonParameters                                       //
//                                                              //
// General Vulkan parameters' container class                   //
// ************************************************************ //
struct VulkanCommonParameters {
  vk::Instance                    Instance;
  vk::PhysicalDevice              PhysicalDevice;
  vk::Device                      Device;
  QueueParameters                 GraphicsQueue;
  QueueParameters                 PresentQueue;
  vk::SurfaceKHR                  PresentationSurface;
  SwapChainParameters             SwapChain;

  VulkanCommonParameters() :
    Instance(),
    PhysicalDevice(),
    Device(),
    GraphicsQueue(),
    PresentQueue(),
    PresentationSurface(),
    SwapChain() {}
};