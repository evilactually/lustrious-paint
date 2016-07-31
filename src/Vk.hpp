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

#define VK_USE_CORE
#define VK_USE_KHR_DISPLAY
#define VK_USE_KHR_SURFACE
#define VK_USE_KHR_WIN32_SURFACE
#define VK_USE_EXT_DEBUG_REPORT
#define VK_USE_KHR_DISPLAY_SWAPCHAIN
#define VK_USE_KHR_SWAPCHAIN

#include "VulkanFunctions.inl"

#undef VK_USE_CORE
#undef VK_USE_KHR_DISPLAY
#undef VK_USE_KHR_SURFACE
#undef VK_USE_KHR_WIN32_SURFACE
#undef VK_USE_EXT_DEBUG_REPORT
#undef VK_USE_KHR_DISPLAY_SWAPCHAIN
#undef VK_USE_KHR_SWAPCHAIN

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

    void UnloadVulkanLibrary() {
        if ( VulkanLibrary ) {
            FreeLibrary( VulkanLibrary );
        }
    }

    void LoadExportedEntryPoints() {
        #define VK_EXPORTED_FUNCTION( fun )                                                  \
            if(!(fun = (PFN_##fun)GetProcAddress( VulkanLibrary, #fun ))) {                  \
                std::cout << "Could not load exported function: ##fun!" << std::endl;        \
                abort();                                                                     \
            }
        #define VK_USE_CORE
        #include "VulkanFunctions.inl"
        #undef VK_USE_CORE
    }

    void LoadGlobalLevelEntryPoints() {
        #define VK_GLOBAL_LEVEL_FUNCTION( fun )                                              \
            if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {               \
                std::cout << "Could not load global level function: ##fun!" << std::endl;    \
                abort();                                                                     \
            }
        #define VK_USE_CORE
        #include "VulkanFunctions.inl"
        #undef VK_USE_CORE
    }

    // void LoadInstanceLevelEntryPoints(vk::Instance instance) {
    //     #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                            \
    //         if( !(fun = (PFN_##fun)instance.getProcAddr( #fun )) ) {                         \
    //             throw std::runtime_error("Could not load instnace level function: ##fun!");  \
    //             std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
    //             abort();                                                                     \
    //         }
    //     #define VK_USE_CORE
    //     #include "VulkanFunctions.inl"
    //     #undef VK_USE_CORE
    // }

    void LoadInstanceLevelEntryPoints(vk::Instance instance, std::vector<const char*> extensions) {
        // Core
        #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                            \
            if( !(fun = (PFN_##fun)instance.getProcAddr( #fun )) ) {                         \
                throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                abort();                                                                     \
            }
        #define VK_USE_CORE
        #include "VulkanFunctions.inl"
        #undef VK_USE_CORE

        // Extensions
        bool KHR_DISPLAY = false;
        bool KHR_SURFACE = false;
        bool KHR_WIN32_SURFACE = false;
        bool EXT_DEBUG_REPORT = false;
        bool KHR_DISPLAY_SWAPCHAIN = false;
        bool KHR_SWAPCHAIN = false;

        // Scan the list and set extension flags
        for (int i = 0; i < static_cast<int>(extensions.size()); ++i)
        {
            if (strcmp(extensions[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {
                KHR_DISPLAY = true;
            } else if (strcmp(extensions[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
                KHR_SURFACE = true;
            } else if (strcmp(extensions[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
                KHR_WIN32_SURFACE = true;
            } else if (strcmp(extensions[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
                EXT_DEBUG_REPORT = true;
            } else if (strcmp(extensions[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
                KHR_DISPLAY_SWAPCHAIN = true;
            } else if (strcmp(extensions[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                KHR_SWAPCHAIN = true;
            }
        }

        #define LOAD_FUNCTION( fun )                                                             \
        if( !(fun = (PFN_##fun)instance.getProcAddr( #fun )) ) {                                 \
                    throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                    std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                    abort();                                                                     \
                }

        #define VK_USE_KHR_DISPLAY
        #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
            if ( KHR_DISPLAY )                                                                   \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_KHR_DISPLAY

        #define VK_USE_KHR_SURFACE
        #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
            if ( KHR_SURFACE )                                                                   \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_KHR_SURFACE    

        #define VK_USE_KHR_WIN32_SURFACE
        #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
            if ( KHR_WIN32_SURFACE )                                                             \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_KHR_WIN32_SURFACE

        #define VK_USE_EXT_DEBUG_REPORT
        #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
            if ( EXT_DEBUG_REPORT )                                                              \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_EXT_DEBUG_REPORT

        #undef LOAD_FUNCTION
    }

    void LoadDeviceLevelEntryPoints(vk::Device device, std::vector<const char*> extensions) {
        // Core
        #define VK_DEVICE_LEVEL_FUNCTION( fun )                                              \
            if( !(fun = (PFN_##fun)device.getProcAddr( #fun )) ) {                           \
                throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                abort();                                                                     \
            }
        #define VK_USE_CORE
        #include "VulkanFunctions.inl"
        #undef VK_USE_CORE

        // Extensions
        bool KHR_DISPLAY = false;
        bool KHR_SURFACE = false;
        bool KHR_WIN32_SURFACE = false;
        bool EXT_DEBUG_REPORT = false;
        bool KHR_DISPLAY_SWAPCHAIN = false;
        bool KHR_SWAPCHAIN = false;

        // Scan the list and set extension flags
        for (int i = 0; i < static_cast<int>(extensions.size()); ++i)
        {
            if (strcmp(extensions[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {
                KHR_DISPLAY = true;
            } else if (strcmp(extensions[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
                KHR_SURFACE = true;
            } else if (strcmp(extensions[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
                KHR_WIN32_SURFACE = true;
            } else if (strcmp(extensions[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
                EXT_DEBUG_REPORT = true;
            } else if (strcmp(extensions[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
                KHR_DISPLAY_SWAPCHAIN = true;
            } else if (strcmp(extensions[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                KHR_SWAPCHAIN = true;
            }
        }

        #define LOAD_FUNCTION( fun )                                                             \
        if( !(fun = (PFN_##fun)device.getProcAddr( #fun )) ) {                                 \
                    throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                    std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                    abort();                                                                     \
                }

        #define VK_USE_KHR_DISPLAY
        #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
            if ( KHR_DISPLAY )                                                                 \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_KHR_DISPLAY

        #define VK_USE_KHR_SURFACE
        #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
            if ( KHR_SURFACE )                                                                 \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_KHR_SURFACE    

        #define VK_USE_KHR_WIN32_SURFACE
        #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
            if ( KHR_WIN32_SURFACE )                                                           \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_KHR_WIN32_SURFACE

        #define VK_USE_EXT_DEBUG_REPORT
        #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
            if ( EXT_DEBUG_REPORT )                                                            \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_EXT_DEBUG_REPORT

        #define VK_USE_KHR_DISPLAY_SWAPCHAIN
        #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
           if ( KHR_DISPLAY_SWAPCHAIN )                                                        \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_KHR_DISPLAY_SWAPCHAIN

        #define VK_USE_KHR_SWAPCHAIN
        #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
           if ( KHR_SWAPCHAIN )                                                        \
                LOAD_FUNCTION(fun)
        #include "VulkanFunctions.inl"
        #undef VK_USE_SWAPCHAIN

        #undef LOAD_FUNCTION
    }
}
