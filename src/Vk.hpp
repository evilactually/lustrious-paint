#pragma once

#include <stdexcept>
#include <iostream>
#include <windows.h>

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;

#define USE_KHR_SWAPCHAIN_EXTENSION
#define USE_KHR_DISPLAY_EXTENSION
#define USE_KHR_SURFACE_EXTENSION
#define USE_KHR_WIN32_SURFACE_EXTENSION
#define USE_EXT_DEBUG_REPORT_EXTENSION
#define USE_KHR_DISPLAY_SWAPCHAIN_EXTENSION

#include "VulkanFunctions.inl"

#undef USE_KHR_SWAPCHAIN_EXTENSION
#undef USE_KHR_DISPLAY_EXTENSION
#undef USE_KHR_SURFACE_EXTENSION
#undef USE_KHR_WIN32_SURFACE_EXTENSION
#undef USE_EXT_DEBUG_REPORT_EXTENSION
#undef USE_KHR_DISPLAY_SWAPCHAIN_EXTENSION

#include "vulkan/vk_cpp.hpp"

extern HMODULE VulkanLibrary;

namespace vk
{
    void LoadVulkanLibrary() {
        VulkanLibrary = LoadLibrary("vulkan-1.dll");
        if (VulkanLibrary == nullptr) {
            throw std::runtime_error("Could not load Vulkan library!");
        }
    }

    void LoadExportedEntryPoints() {
        #define VK_EXPORTED_FUNCTION( fun )                                                  \
            if(!(fun = (PFN_##fun)GetProcAddress( VulkanLibrary, #fun ))) {                  \
                std::cout << "Could not load exported function: ##fun!" << std::endl;        \
                abort();                                                                     \
            }

        #include "VulkanFunctions.inl"
    }

    void LoadGlobalLevelEntryPoints() {
        #define VK_GLOBAL_LEVEL_FUNCTION( fun )                                              \
            if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {               \
                std::cout << "Could not load global level function: ##fun!" << std::endl;    \
                abort();                                                                     \
            }

        #include "VulkanFunctions.inl"
    }

    void LoadInstanceLevelEntryPoints(vk::Instance instance) {
        #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                            \
            if( !(fun = (PFN_##fun)instance.getProcAddr( #fun )) ) {                         \
                throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                abort();                                                                     \
            }

        #include "VulkanFunctions.inl"
    }

    void LoadDeviceLevelEntryPoints(vk::Device device) {
        #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                      \
            if( !(fun = (PFN_##fun)device.getProcAddr( #fun )) ) {                                   \
                std::cout << "Could not load device level function: " << #fun << "!" << std::endl;   \
                abort();                                                                             \
            }

        #include "VulkanFunctions.inl"
    }
}
