#pragma once

#include "Windows.h"
#include <vector>
#include "Version.h"
#include "Vk.hpp"

namespace Ls {
    namespace Application {
        vk::Instance instance;
        int width = 800;
        int height = 600;
        HINSTANCE hInstance;
        MSG msg;
        HWND windowHandle;
        vk::Device device;
        uint32_t queue_family_index;

        LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
            switch (uMsg) {
            case WM_CLOSE: {
                PostQuitMessage(0);
                break;
            }
            case WM_PAINT: {
                break;
            }
            default: {
                break;
            }
            }

            // a pass-through for now. We will return to this callback
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        void CreateMainWindow() {
            WNDCLASSEX windowClass = {};
            windowClass.cbSize = sizeof(WNDCLASSEX);
            windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
            windowClass.lpfnWndProc = WindowProc;
            windowClass.hInstance = hInstance;
            windowClass.lpszClassName = "LsMainWindow";
            RegisterClassEx(&windowClass);
    
            windowHandle = CreateWindowEx(NULL,
                "LsMainWindow",
                "Lustrious Paint",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                100,
                100,
                width,
                height,
                NULL,
                NULL,
                hInstance,
                NULL);
        }

        bool Update() {
            bool running = true;
            PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);
            if (msg.message == WM_QUIT) {
                running = false;
            }
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            RedrawWindow(windowHandle, NULL, NULL, RDW_INTERNALPAINT);
            return running;
        }
    
        void CreateInstance() {
            vk::ApplicationInfo appliactionInfo(Ls::Info::PRODUCT_NAME,
                                                Ls::Info::VERSION,
                                                NULL, 
                                                NULL,
                                                VK_API_VERSION_1_0);
            vk::InstanceCreateInfo instanceCreateInfo(vk::InstanceCreateFlags(), &appliactionInfo, 0, NULL, 0, NULL);
            instance = createInstance(instanceCreateInfo);
        }

        bool CheckPhysicalDeviceProperties( vk::PhysicalDevice physical_device, uint32_t &queue_family_index ) {
            vk::PhysicalDeviceProperties device_properties;
            vk::PhysicalDeviceFeatures   device_features;

            physical_device.getProperties(&device_properties);
            physical_device.getFeatures(&device_features);

            uint32_t major_version = VK_VERSION_MAJOR( device_properties.apiVersion );
            uint32_t minor_version = VK_VERSION_MINOR( device_properties.apiVersion );
            uint32_t patch_version = VK_VERSION_PATCH( device_properties.apiVersion );

            if( (major_version < 1) ||
                (device_properties.limits.maxImageDimension2D < 4096) ) {
                std::cout << "Physical device " << device_properties.deviceName << " doesn't support required parameters!" << std::endl;
                return false;
            }

            uint32_t queue_families_count = 0;
            physical_device.getQueueFamilyProperties( &queue_families_count, nullptr );
            if( queue_families_count == 0 ) {
                std::cout << "Physical device " << device_properties.deviceName << " doesn't have any queue families!" << std::endl;
                return false;
            }

            std::vector<vk::QueueFamilyProperties> queue_family_properties( queue_families_count );
            physical_device.getQueueFamilyProperties( &queue_families_count, queue_family_properties.data() );
            for( uint32_t i = 0; i < queue_families_count; ++i ) {
                if( (queue_family_properties[i].queueCount > 0) &&
                    (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) ) {
                    queue_family_index = i;
                    return true;
                }
            }

            std::cout << "Could not find queue family with required properties on physical device " << device_properties.deviceName << "!" << std::endl;
            return false;
          }

        void CreateDevice() {
            uint32_t num_devices = 0;
            if( (instance.enumeratePhysicalDevices(&num_devices, NULL) != vk::Result::eSuccess) || (num_devices == 0) ) {
                std::cout << "Error occurred during physical devices enumeration!" << std::endl;
                abort();
            }

            std::vector<vk::PhysicalDevice> physical_devices( num_devices );
            if( (instance.enumeratePhysicalDevices(&num_devices, physical_devices.data()) != vk::Result::eSuccess) || (num_devices == 0) ) {
                std::cout << "Error occurred during physical devices enumeration!" << std::endl;
                abort();
            }
            std::cout << "Error occurred during physical devices enumeration!" << std::endl;

            vk::PhysicalDevice* selected_physical_device = nullptr;
            uint32_t selected_queue_family_index = UINT32_MAX;
            for( uint32_t i = 0; i < num_devices; ++i ) {
                if( CheckPhysicalDeviceProperties( physical_devices[i], selected_queue_family_index ) ) {
                    selected_physical_device = &physical_devices[i];
                }
            }
            if( selected_physical_device == nullptr ) {
                std::cout << "Could not select physical device based on the chosen properties!" << std::endl;
                abort();
            }

            std::vector<float> queue_priorities = { 1.0f };
            vk::DeviceQueueCreateInfo queue_create_info(vk::DeviceQueueCreateFlags(),                   // DeviceQueueCreateFlags       flags
                                                        selected_queue_family_index,                    // uint32_t                     queueFamilyIndex
                                                        static_cast<uint32_t>(queue_priorities.size()), // uint32_t                     queueCount
                                                        &queue_priorities[0]);                          // const float                  *pQueuePriorities

            vk::DeviceCreateInfo device_create_info(vk::DeviceCreateFlags(),   // VkDeviceCreateFlags                flags
                                                    1,                         // uint32_t                           queueCreateInfoCount
                                                    &queue_create_info,        // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
                                                    0,                         // uint32_t                           enabledLayerCount
                                                    nullptr,                   // const char * const                *ppEnabledLayerNames
                                                    0,                         // uint32_t                           enabledExtensionCount
                                                    nullptr,                   // const char * const                *ppEnabledExtensionNames
                                                    nullptr);                  // const VkPhysicalDeviceFeatures    *pEnabledFeatures

            if( selected_physical_device->createDevice( &device_create_info, nullptr, &Ls::Application::device ) != vk::Result::eSuccess ) {
                std::cout << "Could not create Vulkan device!" << std::endl;
                abort();
            }

            Ls::Application::queue_family_index = selected_queue_family_index;
        }
    }
}