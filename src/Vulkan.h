#pragma once

#include <set>
#include <vector>
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

            uint32_t layerCount = 0;
            vkEnumerateInstanceLayerProperties(&layerCount, NULL);
            Assert(layerCount != 0, "Failed to find any layer in your system.");
            VkLayerProperties *layersAvailable = new VkLayerProperties[layerCount];
            vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable);

            bool foundValidation = false;
            for( int i = 0; i < layerCount; ++i ) {
                if(strcmp(layersAvailable[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
                    foundValidation = true;
                }
            }
            Assert(foundValidation, "Could not find validation layer.");
            const char *layers[] = { "VK_LAYER_LUNARG_standard_validation" };

            VkInstanceCreateInfo instanceInfo = {};
            instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceInfo.pApplicationInfo = &applicationInfo;
            instanceInfo.enabledLayerCount = 1;
            instanceInfo.ppEnabledLayerNames = layers;
            instanceInfo.enabledExtensionCount = 0;
            instanceInfo.ppEnabledExtensionNames = NULL;

            result = vkCreateInstance(&instanceInfo, NULL, &context.instance);
            CheckVulkanResult(result, "vkCreateInstance");

        }

        void Render(){

        }
    }
}

