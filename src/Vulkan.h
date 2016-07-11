#pragma once

#include <set>
#include <iterator>
#include <algorithm>
#include <vulkan/vulkan.h>
#include "Version.h"
#include "Utils.h"

namespace Ls {
	namespace Vulkan {
		struct {
			// Vulkan instance, stores all per-application states
			VkInstance instance;
		} context;

		void Initialize(){
			VkResult result;

			VkApplicationInfo applicationInfo;
			applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			applicationInfo.pNext = NULL;
			applicationInfo.pApplicationName = Version::PRODUCT_NAME;
			applicationInfo.pEngineName = NULL;
			applicationInfo.engineVersion = 1;
			applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

			VkInstanceCreateInfo instanceInfo = {};
			instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceInfo.pApplicationInfo = &applicationInfo;
			instanceInfo.enabledLayerCount = 0;
			instanceInfo.ppEnabledLayerNames = NULL;
			instanceInfo.enabledExtensionCount = 0;
		    instanceInfo.ppEnabledExtensionNames = NULL;

		    result = vkCreateInstance(&instanceInfo, NULL, &context.instance);
		    Assert(result, "vkCreateInstance");

		    std::set<int> s1({1,2,3});
		    std::set<int> s2({2,3,4,5});
		    std::set<int> intersect;
		    set_intersection(s1.begin(),s1.end(),s2.begin(),s2.end(), std::inserter(intersect,intersect.begin()));
		}

		void Render(){

		}
	}
}

