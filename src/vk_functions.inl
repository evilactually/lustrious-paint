#if !defined(VK_EXPORTED_FUNCTION)
#define VK_EXPORTED_FUNCTION( fun )
#endif

#if !defined(VK_GLOBAL_LEVEL_FUNCTION)
#define VK_GLOBAL_LEVEL_FUNCTION( fun )
#endif

#if !defined(VK_INSTANCE_LEVEL_FUNCTION)
#define VK_INSTANCE_LEVEL_FUNCTION( fun )
#endif

#if !defined(VK_DEVICE_LEVEL_FUNCTION)
#define VK_DEVICE_LEVEL_FUNCTION( fun )
#endif

#ifdef VK_USE_CORE

VK_EXPORTED_FUNCTION( vkGetInstanceProcAddr )

VK_GLOBAL_LEVEL_FUNCTION( vkCreateInstance )
VK_GLOBAL_LEVEL_FUNCTION( vkEnumerateInstanceExtensionProperties )
VK_GLOBAL_LEVEL_FUNCTION( vkEnumerateInstanceLayerProperties )

VK_INSTANCE_LEVEL_FUNCTION( vkEnumeratePhysicalDevices )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceProperties )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceFeatures )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceQueueFamilyProperties )
VK_INSTANCE_LEVEL_FUNCTION( vkCreateDevice )
VK_INSTANCE_LEVEL_FUNCTION( vkGetDeviceProcAddr )
VK_INSTANCE_LEVEL_FUNCTION( vkDestroyInstance )
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceFormatProperties)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceImageFormatProperties)
VK_INSTANCE_LEVEL_FUNCTION(vkEnumerateDeviceLayerProperties)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceSparseImageFormatProperties)
VK_INSTANCE_LEVEL_FUNCTION( vkEnumerateDeviceExtensionProperties )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceMemoryProperties )

VK_DEVICE_LEVEL_FUNCTION( vkGetDeviceQueue )
VK_DEVICE_LEVEL_FUNCTION( vkDeviceWaitIdle )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyDevice )
VK_DEVICE_LEVEL_FUNCTION( vkCreateSemaphore )
VK_DEVICE_LEVEL_FUNCTION( vkCreateCommandPool )
VK_DEVICE_LEVEL_FUNCTION( vkAllocateCommandBuffers )
VK_DEVICE_LEVEL_FUNCTION( vkBeginCommandBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkCmdPipelineBarrier )
VK_DEVICE_LEVEL_FUNCTION( vkCmdClearColorImage )
VK_DEVICE_LEVEL_FUNCTION( vkEndCommandBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkQueueSubmit )
VK_DEVICE_LEVEL_FUNCTION( vkFreeCommandBuffers )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyCommandPool )
VK_DEVICE_LEVEL_FUNCTION( vkDestroySemaphore )
VK_DEVICE_LEVEL_FUNCTION( vkCreateImageView )
VK_DEVICE_LEVEL_FUNCTION( vkCreateRenderPass )
VK_DEVICE_LEVEL_FUNCTION( vkCreateFramebuffer )
VK_DEVICE_LEVEL_FUNCTION( vkCreateShaderModule )
VK_DEVICE_LEVEL_FUNCTION( vkCreatePipelineLayout )
VK_DEVICE_LEVEL_FUNCTION( vkCreateGraphicsPipelines )
VK_DEVICE_LEVEL_FUNCTION( vkCmdBeginRenderPass )
VK_DEVICE_LEVEL_FUNCTION( vkCmdBindPipeline )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDraw )
VK_DEVICE_LEVEL_FUNCTION( vkCmdEndRenderPass )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyShaderModule )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyPipelineLayout )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyPipeline )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyRenderPass )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyFramebuffer )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyImageView )
VK_DEVICE_LEVEL_FUNCTION( vkCreateFence )
VK_DEVICE_LEVEL_FUNCTION( vkCreateBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkGetBufferMemoryRequirements )
VK_DEVICE_LEVEL_FUNCTION( vkAllocateMemory )
VK_DEVICE_LEVEL_FUNCTION( vkBindBufferMemory )
VK_DEVICE_LEVEL_FUNCTION( vkMapMemory )
VK_DEVICE_LEVEL_FUNCTION( vkFlushMappedMemoryRanges )
VK_DEVICE_LEVEL_FUNCTION( vkUnmapMemory )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetViewport )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetScissor )
VK_DEVICE_LEVEL_FUNCTION( vkCmdBindVertexBuffers )
VK_DEVICE_LEVEL_FUNCTION( vkWaitForFences )
VK_DEVICE_LEVEL_FUNCTION( vkResetFences )
VK_DEVICE_LEVEL_FUNCTION( vkFreeMemory )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyFence )
VK_DEVICE_LEVEL_FUNCTION( vkCmdCopyBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkCreateImage )
VK_DEVICE_LEVEL_FUNCTION( vkGetImageMemoryRequirements )
VK_DEVICE_LEVEL_FUNCTION( vkBindImageMemory )
VK_DEVICE_LEVEL_FUNCTION( vkCreateSampler )
VK_DEVICE_LEVEL_FUNCTION( vkCmdCopyBufferToImage )
VK_DEVICE_LEVEL_FUNCTION( vkCreateDescriptorSetLayout )
VK_DEVICE_LEVEL_FUNCTION( vkCreateDescriptorPool )
VK_DEVICE_LEVEL_FUNCTION( vkAllocateDescriptorSets )
VK_DEVICE_LEVEL_FUNCTION( vkUpdateDescriptorSets )
VK_DEVICE_LEVEL_FUNCTION( vkCmdBindDescriptorSets )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyDescriptorPool )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyDescriptorSetLayout )
VK_DEVICE_LEVEL_FUNCTION( vkDestroySampler )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyImage )
VK_DEVICE_LEVEL_FUNCTION( vkResetCommandBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetLineWidth )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetDepthBias )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetDepthBounds )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetStencilCompareMask )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetStencilWriteMask )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetStencilReference )
VK_DEVICE_LEVEL_FUNCTION( vkCmdBindIndexBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkCmdCopyImage )
VK_DEVICE_LEVEL_FUNCTION( vkCmdBlitImage )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetBlendConstants )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDrawIndexed )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDrawIndirect )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDrawIndexedIndirect )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDispatch )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDispatchIndirect )
VK_DEVICE_LEVEL_FUNCTION( vkCmdCopyImageToBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkCmdUpdateBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkCmdFillBuffer )
VK_DEVICE_LEVEL_FUNCTION( vkCmdClearDepthStencilImage )
VK_DEVICE_LEVEL_FUNCTION( vkCmdClearAttachments )
VK_DEVICE_LEVEL_FUNCTION( vkCmdResolveImage )
VK_DEVICE_LEVEL_FUNCTION( vkCmdSetEvent )
VK_DEVICE_LEVEL_FUNCTION( vkCmdResetEvent )
VK_DEVICE_LEVEL_FUNCTION( vkCmdWaitEvents )
VK_DEVICE_LEVEL_FUNCTION( vkCmdBeginQuery )
VK_DEVICE_LEVEL_FUNCTION( vkCmdEndQuery )
VK_DEVICE_LEVEL_FUNCTION( vkCmdResetQueryPool )
VK_DEVICE_LEVEL_FUNCTION( vkCmdWriteTimestamp )
VK_DEVICE_LEVEL_FUNCTION( vkCmdCopyQueryPoolResults )
VK_DEVICE_LEVEL_FUNCTION( vkCmdPushConstants )
VK_DEVICE_LEVEL_FUNCTION( vkCmdNextSubpass )
VK_DEVICE_LEVEL_FUNCTION( vkCmdExecuteCommands )
VK_DEVICE_LEVEL_FUNCTION( vkQueueWaitIdle )
VK_DEVICE_LEVEL_FUNCTION( vkQueueBindSparse )
VK_DEVICE_LEVEL_FUNCTION( vkInvalidateMappedMemoryRanges )
VK_DEVICE_LEVEL_FUNCTION( vkGetDeviceMemoryCommitment )
VK_DEVICE_LEVEL_FUNCTION( vkGetImageSparseMemoryRequirements )
VK_DEVICE_LEVEL_FUNCTION( vkGetFenceStatus )
VK_DEVICE_LEVEL_FUNCTION( vkCreateEvent )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyEvent )
VK_DEVICE_LEVEL_FUNCTION( vkGetEventStatus )
VK_DEVICE_LEVEL_FUNCTION( vkSetEvent )
VK_DEVICE_LEVEL_FUNCTION( vkResetEvent )
VK_DEVICE_LEVEL_FUNCTION( vkCreateQueryPool )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyQueryPool )
VK_DEVICE_LEVEL_FUNCTION( vkGetQueryPoolResults )
VK_DEVICE_LEVEL_FUNCTION( vkCreateBufferView )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyBufferView )
VK_DEVICE_LEVEL_FUNCTION( vkGetImageSubresourceLayout )
VK_DEVICE_LEVEL_FUNCTION( vkCreatePipelineCache )
VK_DEVICE_LEVEL_FUNCTION( vkDestroyPipelineCache )
VK_DEVICE_LEVEL_FUNCTION( vkGetPipelineCacheData )
VK_DEVICE_LEVEL_FUNCTION( vkMergePipelineCaches )
VK_DEVICE_LEVEL_FUNCTION( vkCreateComputePipelines )
VK_DEVICE_LEVEL_FUNCTION( vkResetDescriptorPool )
VK_DEVICE_LEVEL_FUNCTION( vkFreeDescriptorSets )
VK_DEVICE_LEVEL_FUNCTION( vkGetRenderAreaGranularity )
VK_DEVICE_LEVEL_FUNCTION( vkResetCommandPool )

#endif // #ifdef VK_USE_CORE

#ifdef VK_USE_KHR_DISPLAY
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceDisplayPropertiesKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceDisplayPlanePropertiesKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkGetDisplayPlaneSupportedDisplaysKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkGetDisplayModePropertiesKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkCreateDisplayModeKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkGetDisplayPlaneCapabilitiesKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkCreateDisplayPlaneSurfaceKHR )
#endif

#ifdef VK_USE_KHR_SURFACE
VK_INSTANCE_LEVEL_FUNCTION( vkDestroySurfaceKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceSurfaceSupportKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceSurfaceCapabilitiesKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceSurfaceFormatsKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceSurfacePresentModesKHR )
#endif

#ifdef VK_USE_KHR_WIN32_SURFACE
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceWin32PresentationSupportKHR )
VK_INSTANCE_LEVEL_FUNCTION( vkCreateWin32SurfaceKHR )
#endif

#ifdef VK_USE_EXT_DEBUG_REPORT
VK_INSTANCE_LEVEL_FUNCTION( vkCreateDebugReportCallbackEXT )
VK_INSTANCE_LEVEL_FUNCTION( vkDestroyDebugReportCallbackEXT )
VK_INSTANCE_LEVEL_FUNCTION( vkDebugReportMessageEXT )
#endif

#ifdef VK_USE_KHR_DISPLAY_SWAPCHAIN
VK_DEVICE_LEVEL_FUNCTION( vkCreateSharedSwapchainsKHR )
#endif

#ifdef VK_USE_KHR_SWAPCHAIN
VK_DEVICE_LEVEL_FUNCTION( vkCreateSwapchainKHR )
VK_DEVICE_LEVEL_FUNCTION( vkDestroySwapchainKHR )
VK_DEVICE_LEVEL_FUNCTION( vkGetSwapchainImagesKHR )
VK_DEVICE_LEVEL_FUNCTION( vkAcquireNextImageKHR )
VK_DEVICE_LEVEL_FUNCTION( vkQueuePresentKHR )
#endif

#ifdef VK_USE_KHR_DEBUG_MARKER
VK_DEVICE_LEVEL_FUNCTION( vkDebugMarkerSetObjectTagEXT )
VK_DEVICE_LEVEL_FUNCTION( vkDebugMarkerSetObjectNameEXT )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDebugMarkerBeginEXT )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDebugMarkerEndEXT )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDebugMarkerInsertEXT )
#endif

#ifdef VK_USE_AMD_DRAW_INDIRECT_COUNT
VK_DEVICE_LEVEL_FUNCTION( vkCmdDrawIndirectCountAMD )
VK_DEVICE_LEVEL_FUNCTION( vkCmdDrawIndexedIndirectCountAMD )
#endif

#ifdef VK_USE_NV_EXTERNAL_MEMORY_WIN32
VK_DEVICE_LEVEL_FUNCTION( vkGetMemoryWin32HandleNV )
#endif

#ifdef VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES
VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceExternalImageFormatPropertiesNV )
#endif

#undef VK_EXPORTED_FUNCTION
#undef VK_GLOBAL_LEVEL_FUNCTION
#undef VK_INSTANCE_LEVEL_FUNCTION
#undef VK_DEVICE_LEVEL_FUNCTION