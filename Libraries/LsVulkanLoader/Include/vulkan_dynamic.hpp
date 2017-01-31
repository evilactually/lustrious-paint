//===============================================================================
// @ vulkan_dynamic.h
// 
// A wrapper for vulkan.h header from LunarG SDK that uses dynamically loaded
// commands
//
//===============================================================================

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------
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
#define VK_USE_KHR_DEBUG_MARKER
#define VK_USE_AMD_DRAW_INDIRECT_COUNT
#define VK_USE_NV_EXTERNAL_MEMORY_WIN32
#define VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES

#include "LsVulkanCommands.inl"

#undef VK_USE_CORE
#undef VK_USE_KHR_DISPLAY
#undef VK_USE_KHR_SURFACE
#undef VK_USE_KHR_WIN32_SURFACE
#undef VK_USE_EXT_DEBUG_REPORT
#undef VK_USE_KHR_DISPLAY_SWAPCHAIN
#undef VK_USE_KHR_SWAPCHAIN
#undef VK_USE_KHR_DEBUG_MARKER
#undef VK_USE_AMD_DRAW_INDIRECT_COUNT
#undef VK_USE_NV_EXTERNAL_MEMORY_WIN32
#undef VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES