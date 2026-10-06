//===============================================================================
// @ LsVulkanLoader.cpp
// 
// Dynamic loader for vulkan
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>
#include "vulkan_dynamic.hpp"
#include <SDL3/SDL_vulkan.h>

//-------------------------------------------------------------------------------
//-- Globals --------------------------------------------------------------------
//-------------------------------------------------------------------------------

bool LsVulkanLibraryLoaded = false;

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

// SDL owns the Vulkan library, so the loader stays in sync with the one SDL uses for surfaces
void LsLoadVulkanLibrary() {
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        std::cout << "Could not load Vulkan library: " << SDL_GetError() << std::endl;
        throw 1;
    }
    LsVulkanLibraryLoaded = true;
}

void LsUnloadVulkanLibrary() {
    if ( LsVulkanLibraryLoaded ) {
        SDL_Vulkan_UnloadLibrary();
        LsVulkanLibraryLoaded = false;
    }
}

void LsLoadExportedEntryPoints() {
    // vkGetInstanceProcAddr is the only exported function, everything else is loaded through it
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
    if (!vkGetInstanceProcAddr) {
        std::cout << "Could not load exported function: vkGetInstanceProcAddr! " << SDL_GetError() << std::endl;
        throw 1;
    }
}

void LsLoadGlobalLevelEntryPoints() {
    #define VK_GLOBAL_LEVEL_FUNCTION( fun )                                              \
        if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {               \
            std::cout << "Could not load global level function: ##fun!" << std::endl;    \
            throw 1;                                                                     \
        }
    #define VK_USE_CORE
    #include "LsVulkanCommands.inl"
    #undef VK_USE_CORE
}

void LsLoadInstanceLevelEntryPoints(VkInstance instance, std::vector<const char*> extensions) {
    // Core
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                            \
        if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( instance, #fun )) ) {                        \
            throw std::runtime_error("Could not load instnace level function: ##fun!");  \
            std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
            throw 1;                                                                     \
        }
    #define VK_USE_CORE
    #include "LsVulkanCommands.inl"
    #undef VK_USE_CORE

    // Extensions
    bool KHR_DISPLAY = false;
    bool KHR_SURFACE = false;
    bool KHR_WIN32_SURFACE = false;
    bool EXT_DEBUG_REPORT = false;
    bool KHR_DISPLAY_SWAPCHAIN = false;
    bool KHR_SWAPCHAIN = false;
    bool NV_EXTERNAL_MEMORY_CAPABILITIES = false;

    // Scan the list and set extension flags
    for (int i = 0; i < static_cast<int>(extensions.size()); ++i)
    {
        if (strcmp(extensions[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {
            KHR_DISPLAY = true;
        } else if (strcmp(extensions[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            KHR_SURFACE = true;
        } else if (strcmp(extensions[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            EXT_DEBUG_REPORT = true;
        } else if (strcmp(extensions[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
            KHR_DISPLAY_SWAPCHAIN = true;
        } else if (strcmp(extensions[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            KHR_SWAPCHAIN = true;
        }
    }

    #define LOAD_FUNCTION( fun )                                                             \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( instance, #fun )) ) {                      \
                throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                throw 1;                                                                     \
            }

    #define VK_USE_KHR_DISPLAY
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_DISPLAY )                                                                   \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_DISPLAY

    #define VK_USE_KHR_SURFACE
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_SURFACE )                                                                   \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_SURFACE    

    #define VK_USE_KHR_WIN32_SURFACE
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_WIN32_SURFACE )                                                             \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_WIN32_SURFACE

    #define VK_USE_EXT_DEBUG_REPORT
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( EXT_DEBUG_REPORT )                                                              \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_EXT_DEBUG_REPORT

    #define VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( NV_EXTERNAL_MEMORY_CAPABILITIES )                                               \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES

    #undef LOAD_FUNCTION
}

void LsLoadDeviceLevelEntryPoints(VkDevice device, std::vector<const char*> extensions) {
    // Core
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                              \
        if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( device, #fun )) ) {                \
            throw std::runtime_error("Could not load instnace level function: ##fun!");  \
            std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
            throw 1;                                                                     \
        }
    #define VK_USE_CORE
    #include "LsVulkanCommands.inl"
    #undef VK_USE_CORE

    // Extensions
    bool KHR_DISPLAY = false;
    bool KHR_SURFACE = false;
    bool KHR_WIN32_SURFACE = false;
    bool EXT_DEBUG_REPORT = false;
    bool KHR_DISPLAY_SWAPCHAIN = false;
    bool KHR_SWAPCHAIN = false;
    bool KHR_DEBUG_MARKER = false;
    bool AMD_DRAW_INDIRECT_COUNT = false;
    bool NV_EXTERNAL_MEMORY_WIN32 = false;

    // Scan the list and set extension flags
    for (int i = 0; i < static_cast<int>(extensions.size()); ++i)
    {
        if (strcmp(extensions[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {
            KHR_DISPLAY = true;
        } else if (strcmp(extensions[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            KHR_SURFACE = true;
        } else if (strcmp(extensions[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            EXT_DEBUG_REPORT = true;
        } else if (strcmp(extensions[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
            KHR_DISPLAY_SWAPCHAIN = true;
        } else if (strcmp(extensions[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            KHR_SWAPCHAIN = true;
        }
    }

    #define LOAD_FUNCTION( fun )                                                             \
    if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( device, #fun )) ) {                                   \
                throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                throw 1;                                                                     \
            }

    // NOTE: Unfortunately the preprocessor does not support defining macros from other macros
    //       so it is not possible to move out this kind of code into a macro to avoid repetition
    #define VK_USE_KHR_DISPLAY
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_DISPLAY )                                                                 \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_DISPLAY

    #define VK_USE_KHR_SURFACE
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_SURFACE )                                                                 \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_SURFACE    

    #define VK_USE_KHR_WIN32_SURFACE
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_WIN32_SURFACE )                                                           \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_WIN32_SURFACE

    #define VK_USE_EXT_DEBUG_REPORT
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
        if ( EXT_DEBUG_REPORT )                                                            \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_EXT_DEBUG_REPORT

    #define VK_USE_KHR_DISPLAY_SWAPCHAIN
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( KHR_DISPLAY_SWAPCHAIN )                                                        \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_DISPLAY_SWAPCHAIN

    #define VK_USE_KHR_SWAPCHAIN
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( KHR_SWAPCHAIN )                                                                \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_SWAPCHAIN

    #define VK_USE_KHR_DEBUG_MARKER
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( KHR_DEBUG_MARKER )                                                             \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_DEBUG_MARKER

    #define VK_USE_AMD_DRAW_INDIRECT_COUNT
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( AMD_DRAW_INDIRECT_COUNT )                                                      \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_AMD_DRAW_INDIRECT_COUNT

    #define VK_USE_NV_EXTERNAL_MEMORY_WIN32
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( NV_EXTERNAL_MEMORY_WIN32 )                                                     \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_NV_EXTERNAL_MEMORY_WIN32

    #undef LOAD_FUNCTION
}