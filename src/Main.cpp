#include <windows.h>
#include "Vk.hpp"
#include "Version.h"
#include "Optional.hpp"

//#include "Application.hpp"

//void assert(bool flag, char *msg = "") {
//    if (!flag) {
//        OutputDebugStringA("ASSERT: ");
//        OutputDebugStringA(msg);
//        OutputDebugStringA("\n");
//        int *base = 0;
//        *base = 1;
//    }
//}

namespace Ls {
    vk::Instance instance;
    int width = 800;
    int height = 600;
    HINSTANCE hInstance;
    MSG msg;
    HWND windowHandle;
    vk::Device device;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    vk::Queue graphics_queue;
	vk::Queue present_queue;
    std::vector<const char*> instance_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    vk::SurfaceKHR presentation_surface;
    vk::Semaphore image_available_semaphore;
    vk::Semaphore rendering_finished_semaphore;
    vk::PhysicalDevice physical_device;
    vk::SwapchainKHR swapchain;
    bool canRender = false;

    void Abort(std::string& msg) {
        MessageBox(windowHandle,
                   msg.c_str(),
                   "Error",
                   MB_OK | MB_ICONERROR);
        exit(1);
    }

    void Abort(const char* msg) {
        Abort(std::string(msg));
    }

    void Abort() {
        exit(1);
    }

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
            Abort("Error occurred during instance instance_extensions enumeration!");
        }

        std::vector<vk::ExtensionProperties> available_instance_extensions( instance_extensions_count );
        if( vk::enumerateInstanceExtensionProperties( nullptr, &instance_extensions_count, &available_instance_extensions[0] ) != vk::Result::eSuccess ) {
            Abort("Error occurred during instance instance_extensions enumeration!");
        }

        for( size_t i = 0; i < instance_extensions.size(); ++i ) {
            if( !CheckExtensionAvailability( instance_extensions[i], available_instance_extensions ) ) {
                Abort(std::string("Could not find instance extension named \"") + 
                      std::string(instance_extensions[i]) + 
                      std::string("\"!"));
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
            vk::Win32SurfaceCreateFlagsKHR(),                 // vk::Win32SurfaceCreateFlagsKHR   flags
            hInstance,                                        // HINSTANCE                        hinstance
            windowHandle                                      // HWND                             hwnd
        );

        if( instance.createWin32SurfaceKHR( &surface_create_info, nullptr, &presentation_surface ) != vk::Result::eSuccess ) {
            Abort("Could not create presentation surface!");
        }
    }

    bool FindQueueFamilies(vk::PhysicalDevice physical_device, uint32_t* selected_graphics_queue_family_index, uint32_t* selected_present_queue_family_index) {
        uint32_t queue_families_count = 0;
        physical_device.getQueueFamilyProperties( &queue_families_count, nullptr );
        if( queue_families_count == 0 ) {
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
            return false;
        }

        std::vector<vk::ExtensionProperties> available_extensions( extensions_count );
        if( physical_device.enumerateDeviceExtensionProperties( nullptr, &extensions_count, &available_extensions[0] ) != vk::Result::eSuccess ) {
            return false;
        }

        for( size_t i = 0; i < device_extensions.size(); ++i ) {
            if( !CheckExtensionAvailability( device_extensions[i], available_extensions ) ) {
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
            return false;
        }

        if (!FindQueueFamilies(physical_device, queue_family_index, present_queue_family_index)) {
            return false;
        }
        return true;
    }

    void CreateDevice() {
        uint32_t num_devices = 0;
        if( (instance.enumeratePhysicalDevices(&num_devices, NULL) != vk::Result::eSuccess) || (num_devices == 0) ) {
            Abort("Error occurred during physical devices enumeration!");
        }

        std::vector<vk::PhysicalDevice> physical_devices( num_devices );
        if( (instance.enumeratePhysicalDevices(&num_devices, physical_devices.data()) != vk::Result::eSuccess) || (num_devices == 0) ) {
            Abort("Error occurred during physical devices enumeration!");
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
            Abort("Could not select physical device based on the chosen properties!");
        }

        std::vector<float> queue_priorities = { 1.0f };
        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

        queue_create_infos.push_back({vk::DeviceQueueCreateFlags(),                   // vk::DeviceQueueCreateFlags   flags
                                      selected_graphics_queue_family_index,           // uint32_t                     queueFamilyIndex
                                      static_cast<uint32_t>(queue_priorities.size()), // uint32_t                     queueCount
                                      &queue_priorities[0]});                         // const float                  *pQueuePriorities

        if (selected_graphics_queue_family_index != selected_present_queue_family_index) {
            queue_create_infos.push_back({vk::DeviceQueueCreateFlags(),               // vk::DeviceQueueCreateFlags   flags
                                      selected_present_queue_family_index,            // uint32_t                     queueFamilyIndex
                                      static_cast<uint32_t>(queue_priorities.size()), // uint32_t                     queueCount
                                      &queue_priorities[0]});                         // const float                  *pQueuePriorities    
        }

        vk::DeviceCreateInfo device_create_info(vk::DeviceCreateFlags(),                          // vk::DeviceCreateFlags              flags
                                                static_cast<uint32_t>(queue_create_infos.size()), // uint32_t                           queueCreateInfoCount
                                                &queue_create_infos[0],                           // const VkDeviceQueueCreateInfo      *pQueueCreateInfos
                                                0,                                                // uint32_t                           enabledLayerCount
                                                nullptr,                                          // const char * const                 *ppEnabledLayerNames
                                                static_cast<uint32_t>(device_extensions.size()),  // uint32_t                           enabledExtensionCount
                                                &device_extensions[0],                            // const char * const                 *ppEnabledExtensionNames
                                                nullptr);                                         // const vk::PhysicalDeviceFeatures   *pEnabledFeatures

        if( selected_physical_device->createDevice( &device_create_info, nullptr, &Ls::device ) != vk::Result::eSuccess ) {
            Abort("Could not create Vulkan device!");
        }

        Ls::graphics_queue_family_index = selected_graphics_queue_family_index;
        Ls::present_queue_family_index = selected_present_queue_family_index;
        Ls::physical_device = *selected_physical_device;
    }

    void GetQueues() {
        device.getQueue( graphics_queue_family_index, 0, &graphics_queue );
        device.getQueue( present_queue_family_index, 0, &present_queue );
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

    void CreateSemaphores() {
        vk::SemaphoreCreateInfo semaphore_create_info = {
            vk::SemaphoreCreateFlags()
        };

        if( (device.createSemaphore( &semaphore_create_info, nullptr, &image_available_semaphore ) != vk::Result::eSuccess) ||
            (device.createSemaphore( &semaphore_create_info, nullptr, &rendering_finished_semaphore ) != vk::Result::eSuccess) ) {
            Abort("Could not create semaphores!");
        }
    }

    uint32_t GetSwapChainNumImages( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
        // Set of images defined in a swap chain may not always be available for application to render to:
        // One may be displayed and one may wait in a queue to be presented
        // If application wants to use more images at the same time it must ask for more images
        uint32_t image_count = surface_capabilities.minImageCount + 1;
        if( (surface_capabilities.maxImageCount > 0) &&
            (image_count > surface_capabilities.maxImageCount) ) {
          image_count = surface_capabilities.maxImageCount;
        }
        return image_count;
    }

    vk::SurfaceFormatKHR GetSwapChainFormat( std::vector<vk::SurfaceFormatKHR> &surface_formats ) {
        // If the list contains only one entry with undefined format
        // it means that there are no preferred surface formats and any can be chosen
        if( (surface_formats.size() == 1) &&
            (surface_formats[0].format == vk::Format::eUndefined) ) {
            return vk::SurfaceFormatKHR( vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear );
        }

        // Check if list contains most widely used R8 G8 B8 A8 format
        // with nonlinear color space
        for( vk::SurfaceFormatKHR &surface_format : surface_formats ) {
            if( surface_format.format == vk::Format::eR8G8B8A8Unorm ) {
                return surface_format;
            }
        }

        // Return the first format from the list
        return surface_formats[0];
    }

    Optional<vk::Extent2D> GetSwapChainExtent( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
        // Special value of surface extent is width == height == -1
        // If this is so we define the size by ourselves but it must fit within defined confines
        if( surface_capabilities.currentExtent.width == -1 ) {
            VkExtent2D swap_chain_extent = { 640, 480 };
            if( swap_chain_extent.width < surface_capabilities.minImageExtent.width ) {
                swap_chain_extent.width = surface_capabilities.minImageExtent.width;
            }
            if( swap_chain_extent.height < surface_capabilities.minImageExtent.height ) {
                swap_chain_extent.height = surface_capabilities.minImageExtent.height;
            }
            if( swap_chain_extent.width > surface_capabilities.maxImageExtent.width ) {
                swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
            }
            if( swap_chain_extent.height > surface_capabilities.maxImageExtent.height ) {
                swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
            }
            return swap_chain_extent;
        }

        if (surface_capabilities.currentExtent.width == 0 || surface_capabilities.currentExtent.height == 0 ) {
            Optional<vk::Extent2D>();
        }
        // Most of the cases we define size of the swap_chain images equal to current window's size
        return surface_capabilities.currentExtent;
    }

    Optional<vk::ImageUsageFlags> GetSwapChainUsageFlags( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
        // Color attachment flag must always be supported, don't have to check
        // We can define other usage flags but we always need to check if they are supported
        if( surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst ) {
            return vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
        }
        return Optional<vk::ImageUsageFlags>();
    }

    vk::SurfaceTransformFlagBitsKHR GetSwapChainTransform( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
        if( surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ) {
            return vk::SurfaceTransformFlagBitsKHR::eIdentity;
        } else {
            return surface_capabilities.currentTransform;
        }
    }

    Optional<vk::PresentModeKHR> GetSwapChainPresentMode( std::vector<vk::PresentModeKHR> &present_modes ) {
        // FIFO present mode is always available
        // MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
        for( vk::PresentModeKHR &present_mode : present_modes ) {
            if( present_mode == vk::PresentModeKHR::eMailbox ) {
                return present_mode;
            }
        }
        for( vk::PresentModeKHR &present_mode : present_modes ) {
            if( present_mode == vk::PresentModeKHR::eFifo ) {
                return present_mode;
            }
        }
        return Optional<vk::PresentModeKHR>();
    }

    void CreateSwapChain() {
        canRender = false;
        device.waitIdle();

        vk::SurfaceCapabilitiesKHR surface_capabilities;
        if( physical_device.getSurfaceCapabilitiesKHR(presentation_surface, &surface_capabilities) != vk::Result::eSuccess ) {
            Abort("Could not check presentation surface capabilities!");
        }

        uint32_t formats_count;
        if( (physical_device.getSurfaceFormatsKHR( presentation_surface, &formats_count, nullptr ) != vk::Result::eSuccess) ||
            (formats_count == 0) ) {
            Abort("Error occurred during presentation surface formats enumeration!");
        }

        std::vector<vk::SurfaceFormatKHR> surface_formats( formats_count );
        if( physical_device.getSurfaceFormatsKHR( presentation_surface, &formats_count, &surface_formats[0] ) != vk::Result::eSuccess ) {
            Abort("Error occurred during presentation surface formats enumeration!");
        }

        uint32_t present_modes_count;
        if( (physical_device.getSurfacePresentModesKHR( presentation_surface, &present_modes_count, nullptr ) != vk::Result::eSuccess) ||
            (present_modes_count == 0) ) {
            Abort("Error occurred during presentation surface present modes enumeration!");
        }

        std::vector<vk::PresentModeKHR> present_modes( present_modes_count );
        if( physical_device.getSurfacePresentModesKHR( presentation_surface , &present_modes_count, &present_modes[0] ) != vk::Result::eSuccess ) {
            Abort("Error occurred during presentation surface present modes enumeration!");
        }

        uint32_t                        desired_number_of_images = GetSwapChainNumImages( surface_capabilities );
        vk::SurfaceFormatKHR            desired_format = GetSwapChainFormat( surface_formats );
        Optional<vk::Extent2D>          desired_extent = GetSwapChainExtent( surface_capabilities );
        Optional<vk::ImageUsageFlags>   desired_usage = GetSwapChainUsageFlags( surface_capabilities );
        vk::SurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform( surface_capabilities );
        Optional<vk::PresentModeKHR>    desired_present_mode = GetSwapChainPresentMode( present_modes );
        vk::SwapchainKHR                old_swapchain = swapchain;

        if( !desired_usage ) {
            Abort("Surface does not support any suitable usage flags!");
        }
        if( !desired_present_mode ) {
            Abort("Surface does not support any suitable present modes!");
        }

        if( !desired_extent ) {
            // Current surface size is (0, 0) so we can't create a swap chain and render anything (CanRender == false)
            // But we don't wont to kill the application as this situation may occur i.e. when window gets minimized
            return;
        }

        vk::SwapchainCreateInfoKHR swap_chain_create_info(
            vk::SwapchainCreateFlagsKHR(),                // vk::SwapchainCreateFlagsKHR      flags
            presentation_surface,                         // vk::SurfaceKHR                   surface
            desired_number_of_images,                     // uint32_t                         minImageCount
            desired_format.format,                        // vk::Format                       imageFormat
            desired_format.colorSpace,                    // vk::ColorSpaceKHR                imageColorSpace
            desired_extent,                               // vk::Extent2D                     imageExtent
            1,                                            // uint32_t                         imageArrayLayers
            desired_usage,                                // vk::ImageUsageFlags              imageUsage
            vk::SharingMode::eExclusive,                  // vk::SharingMode                  imageSharingMode
            0,                                            // uint32_t                         queueFamilyIndexCount
            nullptr,                                      // const uint32_t                   *pQueueFamilyIndices
            desired_transform,                            // vk::SurfaceTransformFlagBitsKHR  preTransform
            vk::CompositeAlphaFlagBitsKHR::eOpaque,       // vk::CompositeAlphaFlagBitsKHR    compositeAlpha
            desired_present_mode,                         // vk::PresentModeKHR               presentMode
            VK_TRUE,                                      // vk::Bool32                       clipped
            old_swapchain                                 // vk::SwapchainKHR                 oldSwapchain
        );

        if( device.createSwapchainKHR( &swap_chain_create_info, nullptr, &swapchain ) != vk::Result::eSuccess ) {
            Abort("Could not create swap chain!");
        }

        if( !old_swapchain ) {
            device.destroySwapchainKHR( old_swapchain, nullptr );
        }
        canRender = true;
    }
}



int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Ls::hInstance = hInstance;
    Ls::width = 800;
    Ls::height = 600;

    Ls::CreateMainWindow();
    vk::LoadVulkanLibrary();
    vk::LoadExportedEntryPoints();
    vk::LoadGlobalLevelEntryPoints();
    Ls::CreateInstance();
    vk::LoadInstanceLevelEntryPoints(Ls::instance, Ls::instance_extensions);
    Ls::CreateDevice();
    vk::LoadDeviceLevelEntryPoints(Ls::device, Ls::device_extensions);
	Ls::GetQueues();
    Ls::CreatePresentationSurface();
    
    while (Ls::Update()) {};
    return Ls::msg.wParam;

    Ls::FreeDevice();
    Ls::FreeInstance();
    vk::UnloadVulkanLibrary();
}

