
#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#define VK_EXPORTED_FUNCTION( fun ) PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION( fun) PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION( fun ) PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION( fun ) PFN_##fun fun;

#define VK_USE_CORE
#define VK_USE_KHR_DISPLAY
#define VK_USE_KHR_SURFACE
#define VK_USE_KHR_WIN32_SURFACE
#define VK_USE_EXT_DEBUG_REPORT
#define VK_USE_KHR_DISPLAY_SWAPCHAIN
#define VK_USE_KHR_SWAPCHAIN

#include "vk_functions.inl"

#undef VK_USE_CORE
#undef VK_USE_KHR_DISPLAY
#undef VK_USE_KHR_SURFACE
#undef VK_USE_KHR_WIN32_SURFACE
#undef VK_USE_EXT_DEBUG_REPORT
#undef VK_USE_KHR_DISPLAY_SWAPCHAIN
#undef VK_USE_KHR_SWAPCHAIN

namespace vk {
  HMODULE VulkanLibrary;
}
