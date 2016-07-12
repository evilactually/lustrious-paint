#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;

#include "VulkanCommands.inl"

#include "vulkan/vk_cpp.hpp"


