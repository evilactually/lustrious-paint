#include <windows.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <array>
#include <vector>
#include <assert.h>
#include "Version.h"
#include "Tools.h"
#include "vulkan/vk_cpp.hpp"

#include <iostream>

#define ENABLE_VALIDATION true

vk::Instance       instance;
vk::PhysicalDevice physicalDevice;
vk::Device         device;
vk::Queue          graphicsQueue;
vk::PhysicalDeviceProperties physicalDeviceProperties;
vk::Format         depthFormat;

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
	instance = vk::createInstance(instanceCreateInfo);

	// Physical device
	physicalDevice = instance.enumeratePhysicalDevices()[0];
	
	// Find a queue that supports graphics operations
	uint32_t graphicsQueueIndex = 0;
	auto queueProperties = physicalDevice.getQueueFamilyProperties();
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
	device = physicalDevice.createDevice(deviceCreateInfo, nullptr);
	physicalDeviceProperties = physicalDevice.getProperties();
	graphicsQueue = device.getQueue(graphicsQueueIndex, 0);
	VkBool32 validDepthFormat = Tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	assert(validDepthFormat);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	InitializeVulkan();
}