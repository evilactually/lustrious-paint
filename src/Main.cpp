#include <windows.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <array>
#include <vector>
#include <assert.h>
#include "vulkan/vk_cpp.hpp"
#include "Version.h"
#include "Tools.h"

#include <iostream>

#define ENABLE_VALIDATION true

// Vulkan instance, stores all per-application states
vk::Instance       mInstance;
// Physical device (GPU) that Vulkan will use
vk::PhysicalDevice mPhysicalDevice;
// Logical device, application's view of the physical device (GPU)
vk::Device         device;
// Handle to the device graphics queue that command buffers are submitted to
vk::Queue          graphicsQueue;
// Stores physical device properties (for e.g. checking device limits)
vk::PhysicalDeviceProperties physicalDeviceProperties;
// Stores all available memory (type) properties for the physical device
vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
// Depth format is selected during Vulkan initialization
vk::Format         depthFormat;
// Pipeline stage flags for the submit info structure
vk::PipelineStageFlags submitPipelineStages = vk::PipelineStageFlagBits::eBottomOfPipe;

// Synchronization semaphores
struct {
	// Swap chain image presentation
	vk::Semaphore presentComplete;
	// Command buffer submission and execution
	vk::Semaphore renderComplete;
} semaphores;

void SetupVulkanDebugging(vk::Instance mInstance, vk::DebugReportFlagsEXT flags, vk::DebugReportCallbackEXT callback) {
	// TODO
}

int FindPresentQueueFamilyIndex(vk::PhysicalDevice physicalDevice) {
	// Get available queue family properties
	uint32_t queueCount;
	physicalDevice.getQueueFamilyProperties(&queueCount, nullptr);
	assert(queueCount >= 1);

	std::vector <vk::QueueFamilyProperties> queueProps(queueCount);
	physicalDevice.getQueueFamilyProperties(&queueCount, queueProps.data());

//
//	// Iterate over each queue to learn whether it supports presenting:
//	// Find a queue with present support
//	// Will be used to present the swap chain images to the windowing system
//	std::vector<VkBool32> supportsPresent(queueCount);
//	for (uint32_t i = 0; i < queueCount; i++)
//	{
//		fpGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, surface, &supportsPresent[i]);
//	}
//
//	// Search for a graphics and a present queue in the array of queue
//	// families, try to find one that supports both
//	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
//	uint32_t presentQueueNodeIndex = UINT32_MAX;
//	for (uint32_t i = 0; i < queueCount; i++)
//	{
//		if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
//		{
//			if (graphicsQueueNodeIndex == UINT32_MAX)
//			{
//				graphicsQueueNodeIndex = i;
//			}
//
//			if (supportsPresent[i] == VK_TRUE)
//			{
//				graphicsQueueNodeIndex = i;
//				presentQueueNodeIndex = i;
//				break;
//			}
//		}
//	}
//	if (presentQueueNodeIndex == UINT32_MAX)
//	{
//		// If there's no queue that supports both present and graphics
//		// try to find a separate present queue
//		for (uint32_t i = 0; i < queueCount; ++i)
//		{
//			if (supportsPresent[i] == VK_TRUE)
//			{
//				presentQueueNodeIndex = i;
//				break;
//			}
//		}
//	}
//
//	// Exit if either a graphics or a presenting queue hasn't been found
//	//if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
//	//{
//	//	vkTools::exitFatal("Could not find a graphics and/or presenting queue!", "Fatal error");
//	//}
}

void InitializeVulkan() {
	// Create instance
	uint32_t version = (VERSION_REVISION << 16) | (VERSION_MINOR << 8) | VERSION_MAJOR;
	auto appInfo = vk::ApplicationInfo(PRODUCT_NAME, VERSION, PRODUCT_NAME, version, VK_API_VERSION_1_0);

	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME,
		                                           VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	std::vector<const char*> validationLayerNames;

	if (ENABLE_VALIDATION)
	{
		validationLayerNames.push_back("VK_LAYER_LUNARG_standard_validation");
		enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	auto instanceCreateInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), 
		                                             &appInfo, 
		                                             validationLayerNames.size(), 
		                                             validationLayerNames.data(),
		                                             enabledExtensions.size(),
		                                             enabledExtensions.data());
	mInstance = vk::createInstance(instanceCreateInfo);

	// Physical device
	mPhysicalDevice = mInstance.enumeratePhysicalDevices()[0];
	
	// Find a queue that supports graphics operations
	uint32_t graphicsQueueIndex = 0;
	auto queueProperties = mPhysicalDevice.getQueueFamilyProperties();
	assert(queueProperties.size() > 0);
	for (graphicsQueueIndex = 0; graphicsQueueIndex < queueProperties.size(); graphicsQueueIndex++)
	{
		if (queueProperties[graphicsQueueIndex].queueFlags() & vk::QueueFlagBits::eGraphics) {
			break;
		}
	}
	assert(graphicsQueueIndex < queueProperties.size());

	// Create logical device and request a single queue from graphics family
	std::array<float, 1> queuePriorities = { 0.0f };
	auto queueCreateInfo = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), graphicsQueueIndex, 1, queuePriorities.data());

	auto deviceCreateInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 
		                                         1, &queueCreateInfo, 
		                                         validationLayerNames.size(), validationLayerNames.data(), 
		                                         enabledExtensions.size(), enabledExtensions.data(), 
		                                         nullptr);
	device = mPhysicalDevice.createDevice(deviceCreateInfo, nullptr);
	physicalDeviceProperties = mPhysicalDevice.getProperties();
	physicalDeviceMemoryProperties = mPhysicalDevice.getMemoryProperties();
	graphicsQueue = device.getQueue(graphicsQueueIndex, 0);
	VkBool32 validDepthFormat = Tools::getSupportedDepthFormat(mPhysicalDevice, &depthFormat);
	assert(validDepthFormat);

	// NO SWAP CHAIN OBJECT, ALL METHODS ALREADY AVAILABLE

	// Create synchronization objects
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queue
	semaphores.presentComplete = device.createSemaphore(vk::SemaphoreCreateInfo());
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	semaphores.presentComplete = device.createSemaphore(vk::SemaphoreCreateInfo());

	// NO SUBMIT INFO SETUP HERE, BECAUSE WE DON'T HAVE COMMAND BUFFERS TO SUBMIT YET

	// Setup vulkan debugging
	SetupVulkanDebugging(mInstance, vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning, vk::DebugReportCallbackEXT());
    
	// INIT SWAPCHAIN HERE

	// Create command pool
    //vk::CommandPoolCreateInfo()
}

// make rig command buffer
// compile shaders
// update rig
// get pressure, tilt
// winproc
// SetupConsicle

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	InitializeVulkan();
}