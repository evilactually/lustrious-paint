#include <windows.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "Version.h"

#define ENABLE_VALIDATION true

VkInstance       gInstance;
VkPhysicalDevice gPhysicalDevice;
VkDevice         gDevice;
VkQueue          gGraphicsQueue;

void InitializeVulkan() {
	// Create instance
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = PRODUCT_NAME;
	appInfo.pEngineName = PRODUCT_NAME;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	
	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME,
		                                           VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

	std::vector<const char*> validationLayerNames = { "VK_LAYER_LUNARG_standard_validation" };

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	
	if (enabledExtensions.size() > 0)
	{
		if (ENABLE_VALIDATION)
		{
			enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
		instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}

	if (ENABLE_VALIDATION)
	{
		instanceCreateInfo.enabledLayerCount = validationLayerNames.size();
		instanceCreateInfo.ppEnabledLayerNames = validationLayerNames.data();
	}

	vkCreateInstance(&instanceCreateInfo, nullptr, &gInstance);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	InitializeVulkan();
}