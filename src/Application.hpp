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
        uint32_t graphics_queue_family_index;
        uint32_t present_queue_family_index;
        vk::Queue queue;
        std::vector<const char*> instance_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };
        std::vector<const char*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        vk::SurfaceKHR presentation_surface;

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
    
        bool CheckExtensionAvailability( const char *extension_name, const std::vector<vk::ExtensionProperties> &available_instance_extensions ) {
            for( size_t i = 0; i < available_instance_extensions.size(); ++i ) {
                if( strcmp( available_instance_extensions[i].extensionName, extension_name ) == 0 ) {
                    return true;
                }
            }
            return false;
        }

        void CreateInstance() {
            uint32_t instance_extensions_count = 0;
            if( (vk::enumerateInstanceExtensionProperties( nullptr, &instance_extensions_count, nullptr ) != vk::Result::eSuccess) ||
                (instance_extensions_count == 0) ) {
                std::cout << "Error occurred during instance instance_extensions enumeration!" << std::endl;
                abort();
            }

            std::vector<vk::ExtensionProperties> available_instance_extensions( instance_extensions_count );
            if( vk::enumerateInstanceExtensionProperties( nullptr, &instance_extensions_count, &available_instance_extensions[0] ) != vk::Result::eSuccess ) {
                std::cout << "Error occurred during instance instance_extensions enumeration!" << std::endl;
                abort();
            }

            for( size_t i = 0; i < instance_extensions.size(); ++i ) {
                if( !CheckExtensionAvailability( instance_extensions[i], available_instance_extensions ) ) {
                    std::cout << "Could not find instance extension named \"" << instance_extensions[i] << "\"!" << std::endl;
                    abort();
                }
            }

            vk::ApplicationInfo appliactionInfo(Ls::Info::PRODUCT_NAME,
                                                Ls::Info::VERSION,
                                                NULL,
                                                NULL,
                                                VK_API_VERSION_1_0);
            vk::InstanceCreateInfo instanceCreateInfo(vk::InstanceCreateFlags(),
                                                      &appliactionInfo,
                                                      0,
                                                      NULL,
                                                      static_cast<uint32_t>(instance_extensions.size()),
                                                      &instance_extensions[0]);
            instance = createInstance(instanceCreateInfo);
        }

        void CreatePresentationSurface() {
            vk::Win32SurfaceCreateInfoKHR surface_create_info(
                vk::Win32SurfaceCreateFlagsKHR(),                 // VkWin32SurfaceCreateFlagsKHR     flags
                hInstance,                                        // HINSTANCE                        hinstance
                windowHandle                                      // HWND                             hwnd
            );

            if( instance.createWin32SurfaceKHR( &surface_create_info, nullptr, &presentation_surface ) != vk::Result::eSuccess ) {
                std::cout << "Could not create presentation surface!" << std::endl;
                abort();
            }
        }

        bool FindQueueFamilies(vk::PhysicalDevice physical_device, uint32_t* selected_graphics_queue_family_index, uint32_t* selected_present_queue_family_index) {
            uint32_t queue_families_count = 0;
            physical_device.getQueueFamilyProperties( &queue_families_count, nullptr );
            if( queue_families_count == 0 ) {
                //std::cout << "Physical device " << physical_device << " doesn't have any queue families!" << std::endl;
                return false;
            }

            std::vector<vk::QueueFamilyProperties> queue_family_properties( queue_families_count );
            std::vector<VkBool32>                  queue_present_support( queue_families_count );

            physical_device.getQueueFamilyProperties( &queue_families_count, &queue_family_properties[0] );

            uint32_t graphics_queue_family_index = UINT32_MAX;
            uint32_t present_queue_family_index = UINT32_MAX;

            for( uint32_t i = 0; i < queue_families_count; ++i ) {
                physical_device.getSurfaceSupportKHR( i, presentation_surface, &queue_present_support[i] );

                if( (queue_family_properties[i].queueCount > 0) &&
                    (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) ) {
                    // Select first queue that supports graphics
                    if( graphics_queue_family_index == UINT32_MAX ) {
                        graphics_queue_family_index = i;
                    }

                    // If there is queue that supports both graphics and present - prefer it
                    if( queue_present_support[i] ) {
                        *selected_graphics_queue_family_index = i;
                        *selected_present_queue_family_index = i;
                        return true;
                    }
                }
            }

            // We don't have queue that supports both graphics and present so we have to use separate queues
            for( uint32_t i = 0; i < queue_families_count; ++i ) {
                if( queue_present_support[i] ) {
                    present_queue_family_index = i;
                    break;
                }
            }

            // If this device doesn't support queues with graphics and present capabilities don't use it
            if( (graphics_queue_family_index == UINT32_MAX) ||
                (present_queue_family_index == UINT32_MAX) ) {
              //std::cout << "Could not find queue family with required properties on physical device " << physical_device << "!" << std::endl;
              return false;
            }

            *selected_graphics_queue_family_index = graphics_queue_family_index;
            *selected_present_queue_family_index = present_queue_family_index;
            return true;
        }

        bool CheckPhysicalDeviceProperties( vk::PhysicalDevice physical_device, uint32_t* queue_family_index, uint32_t* present_queue_family_index ) {
            uint32_t extensions_count = 0;
            if( (physical_device.enumerateDeviceExtensionProperties( nullptr, &extensions_count, nullptr ) != vk::Result::eSuccess) ||
                (extensions_count == 0) ) {
                //std::cout << "Error occurred during physical device " << physical_device << " extensions enumeration!" << std::endl;
                return false;
            }

            std::vector<vk::ExtensionProperties> available_extensions( extensions_count );
            if( physical_device.enumerateDeviceExtensionProperties( nullptr, &extensions_count, &available_extensions[0] ) != vk::Result::eSuccess ) {
                //std::cout << "Error occurred during physical device " << physical_device << " extensions enumeration!" << std::endl;
                return false;
            }

            for( size_t i = 0; i < device_extensions.size(); ++i ) {
                if( !CheckExtensionAvailability( device_extensions[i], available_extensions ) ) {
                    //std::cout << "Physical device " << physical_device << " doesn't support extension named \"" << device_extensions[i] << "\"!" << std::endl;
                    return false;
                }
            }

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

            if (!FindQueueFamilies(physical_device, queue_family_index, present_queue_family_index)) {
                std::cout << "Could not find queue family with required properties on physical device " << device_properties.deviceName << "!" << std::endl;
                return false;
            }
            return true;
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

            vk::PhysicalDevice* selected_physical_device = nullptr;
            uint32_t selected_graphics_queue_family_index = UINT32_MAX;
            uint32_t selected_present_queue_family_index = UINT32_MAX;
            for( uint32_t i = 0; i < num_devices; ++i ) {
                if( CheckPhysicalDeviceProperties( physical_devices[i], &selected_graphics_queue_family_index, &selected_present_queue_family_index ) ) {
                    selected_physical_device = &physical_devices[i];
                }
            }
            if( selected_physical_device == nullptr ) {
                std::cout << "Could not select physical device based on the chosen properties!" << std::endl;
                abort();
            }

            std::vector<float> queue_priorities = { 1.0f };
            std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

            queue_create_infos.push_back({vk::DeviceQueueCreateFlags(),                   // DeviceQueueCreateFlags       flags
                                          selected_graphics_queue_family_index,           // uint32_t                     queueFamilyIndex
                                          static_cast<uint32_t>(queue_priorities.size()), // uint32_t                     queueCount
                                          &queue_priorities[0]});                         // const float                  *pQueuePriorities

            if (selected_graphics_queue_family_index != selected_present_queue_family_index) {
                queue_create_infos.push_back({vk::DeviceQueueCreateFlags(),               // DeviceQueueCreateFlags       flags
                                          selected_present_queue_family_index,            // uint32_t                     queueFamilyIndex
                                          static_cast<uint32_t>(queue_priorities.size()), // uint32_t                     queueCount
                                          &queue_priorities[0]});                         // const float                  *pQueuePriorities    
            }

            vk::DeviceCreateInfo device_create_info(vk::DeviceCreateFlags(),                          // VkDeviceCreateFlags                flags
                                                    static_cast<uint32_t>(queue_create_infos.size()), // uint32_t                           queueCreateInfoCount
                                                    &queue_create_infos[0],                           // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
                                                    0,                                                // uint32_t                           enabledLayerCount
                                                    nullptr,                                          // const char * const                *ppEnabledLayerNames
                                                    static_cast<uint32_t>(device_extensions.size()),  // uint32_t                           enabledExtensionCount
                                                    &device_extensions[0],                            // const char * const                *ppEnabledExtensionNames
                                                    nullptr);                                         // const VkPhysicalDeviceFeatures    *pEnabledFeatures

            if( selected_physical_device->createDevice( &device_create_info, nullptr, &Ls::Application::device ) != vk::Result::eSuccess ) {
                std::cout << "Could not create Vulkan device!" << std::endl;
                abort();
            }

            Ls::Application::graphics_queue_family_index = selected_graphics_queue_family_index;
            Ls::Application::present_queue_family_index = selected_present_queue_family_index;
        }

        void GetQueue() {
            device.getQueue( graphics_queue_family_index, 0, &queue );
        }

        void FreeDevice() {
            if( device ) {
                device.waitIdle();
                device.destroy( nullptr );
            }
        }

        void FreeInstance() {
            if( instance ) {
                instance.destroy( nullptr );    
            }
        }
    }
}