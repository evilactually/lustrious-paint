#include <windows.h>
#include "Vk.hpp"
#include "Version.h"
#include "Optional.hpp"
#include "win32_console.hpp"

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
    HWND windowHandle = nullptr;
    vk::Device device;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;
    vk::Queue graphicsQueue;
	vk::Queue presentQueue;
    std::vector<const char*> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<const char*> instanceLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };
    vk::SurfaceKHR presentationSurface;
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderingFinishedSemaphore;
    vk::PhysicalDevice physicalDevice;
    vk::SwapchainKHR swapChain;
    vk::CommandPool presentQueueCmdPool;
    std::vector<vk::CommandBuffer> presentQueueCmdBuffers;
    bool can_render = false;
    vk::DebugReportCallbackEXT debugReportCallback;

    void Abort(std::string& msg) {
        MessageBox(nullptr, // HWND
                   msg.c_str(),
                   "Error",
                   MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY); // MB_DEFAULT_DESKTOP_ONLY required, otherwise message box will not show up
        exit(1);                                                    // given how WM_PAINT is handled.
    }

    void Abort(const char* msg) {
        Abort(std::string(msg));
    }

    void Abort() {
        exit(1);
    }

    void Error() {
        Abort(std::string("Error occurred, view log for details."));   
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback( VkDebugReportFlagsEXT flags, 
        VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, 
        int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData ) {
        std::cout << pLayerPrefix << ": " << pMessage << std::endl;
        return VK_FALSE;
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

    void CheckValidationAvailability() {
        uint32_t availableLayerCount;
        vk::enumerateInstanceLayerProperties(&availableLayerCount, nullptr);
        std::vector<vk::LayerProperties> availableLayers(availableLayerCount);
        vk::enumerateInstanceLayerProperties(&availableLayerCount, &availableLayers[0]);
        availableLayers.resize(availableLayerCount);

        for( size_t i = 0; i < instanceLayers.size(); ++i ) {
            bool found = false;
            for (size_t j = 0; j < availableLayers.size(); ++j)
            {
                if ( strcmp( instanceLayers[i], availableLayers[j].layerName ) == 0 ) {
                    found = true;
                    break;
                }
            }
            if ( !found )
            {
                std::cout << "Instance layer " << instanceLayers[i] << " not found!" << std::endl; 
                Error();
            }
        }
    }

    void CreateDebugReportCallback() {
        vk::DebugReportCallbackCreateInfoEXT callbackCreateInfo(
            vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning,
            MyDebugReportCallback,
            nullptr);
        instance.createDebugReportCallbackEXT(&callbackCreateInfo, nullptr, &debugReportCallback);
    }

    void CreateInstance() {
        uint32_t instance_extensions_count = 0;
        if( (vk::enumerateInstanceExtensionProperties( nullptr, &instance_extensions_count, nullptr ) != vk::Result::eSuccess) ||
            (instance_extensions_count == 0) ) {
            std::cout << "Error occurred during instance instanceExtensions enumeration!" << std::endl;
            Error();
        }

        std::vector<vk::ExtensionProperties> available_instance_extensions( instance_extensions_count );
        if( vk::enumerateInstanceExtensionProperties( nullptr, &instance_extensions_count, &available_instance_extensions[0] ) != vk::Result::eSuccess ) {
            std::cout << "Error occurred during instance instanceExtensions enumeration!" << std::endl;
            Error();
        }

        for( size_t i = 0; i < instanceExtensions.size(); ++i ) {
            if( !CheckExtensionAvailability( instanceExtensions[i], available_instance_extensions ) ) {
                Abort(std::string("Could not find instance extension named \"") + 
                      std::string(instanceExtensions[i]) + 
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
                                                  static_cast<uint32_t>(instanceLayers.size()),
                                                  &instanceLayers[0],
                                                  static_cast<uint32_t>(instanceExtensions.size()),
                                                  &instanceExtensions[0]);
        if (createInstance(&instanceCreateInfo, nullptr, &instance) != vk::Result::eSuccess ) {
            std::cout << "Failed to create Vulkan instance!" << std::endl;
            Error();
        }
    }

    void CreatePresentationSurface() {
        vk::Win32SurfaceCreateInfoKHR surface_create_info(
            vk::Win32SurfaceCreateFlagsKHR(),                 // vk::Win32SurfaceCreateFlagsKHR   flags
            hInstance,                                        // HINSTANCE                        hinstance
            windowHandle                                      // HWND                             hwnd
        );

        if( instance.createWin32SurfaceKHR( &surface_create_info, nullptr, &presentationSurface ) != vk::Result::eSuccess ) {
            std::cout << "Could not create presentation surface!" << std::endl;
            Error();
        }
    }

    bool FindQueueFamilies(vk::PhysicalDevice physicalDevice, uint32_t* selected_graphics_queue_family_index, uint32_t* selected_present_queue_family_index) {
        uint32_t queue_families_count = 0;
        physicalDevice.getQueueFamilyProperties( &queue_families_count, nullptr );
        if( queue_families_count == 0 ) {
            return false;
        }

        std::vector<vk::QueueFamilyProperties> queue_family_properties( queue_families_count );
        std::vector<VkBool32>                  queue_present_support( queue_families_count );

        physicalDevice.getQueueFamilyProperties( &queue_families_count, &queue_family_properties[0] );

        uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
        uint32_t presentQueueFamilyIndex = UINT32_MAX;

        for( uint32_t i = 0; i < queue_families_count; ++i ) {
            physicalDevice.getSurfaceSupportKHR( i, presentationSurface, &queue_present_support[i] );

            if( (queue_family_properties[i].queueCount > 0) &&
                (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) ) {
                // Select first queue that supports graphics
                if( graphicsQueueFamilyIndex == UINT32_MAX ) {
                    graphicsQueueFamilyIndex = i;
                }

                // If there is queue that supports both graphics and present - prefer it
                if( queue_present_support[i] ) {
                    *selected_graphics_queue_family_index = i;
                    *selected_present_queue_family_index = i;
                    std::cout << "Graphics queue family index " << i << std::endl;
                    std::cout << "Present queue family index " << i << std::endl;
                    return true;
                }
            }
        }

        // We don't have queue that supports both graphics and present so we have to use separate queues
        for( uint32_t i = 0; i < queue_families_count; ++i ) {
            if( queue_present_support[i] ) {
                presentQueueFamilyIndex = i;
                break;
            }
        }

        // If this device doesn't support queues with graphics and present capabilities don't use it
        if( (graphicsQueueFamilyIndex == UINT32_MAX) ||
            (presentQueueFamilyIndex == UINT32_MAX) ) {
          return false;
        }

        *selected_graphics_queue_family_index = graphicsQueueFamilyIndex;
        *selected_present_queue_family_index = presentQueueFamilyIndex;
        std::cout << "Graphics queue family index " << graphicsQueueFamilyIndex << std::endl;
        std::cout << "Present queue family index " << presentQueueFamilyIndex << std::endl;
        return true;
    }

    bool CheckPhysicalDeviceProperties( vk::PhysicalDevice physicalDevice, uint32_t* queue_family_index, uint32_t* presentQueueFamilyIndex ) {
        uint32_t extensions_count = 0;
        if( (physicalDevice.enumerateDeviceExtensionProperties( nullptr, &extensions_count, nullptr ) != vk::Result::eSuccess) ||
            (extensions_count == 0) ) {
            return false;
        }

        std::vector<vk::ExtensionProperties> available_extensions( extensions_count );
        if( physicalDevice.enumerateDeviceExtensionProperties( nullptr, &extensions_count, &available_extensions[0] ) != vk::Result::eSuccess ) {
            return false;
        }

        for( size_t i = 0; i < deviceExtensions.size(); ++i ) {
            if( !CheckExtensionAvailability( deviceExtensions[i], available_extensions ) ) {
                return false;
            }
        }

        vk::PhysicalDeviceProperties device_properties;
        vk::PhysicalDeviceFeatures   device_features;

        physicalDevice.getProperties(&device_properties);
        physicalDevice.getFeatures(&device_features);

        uint32_t major_version = VK_VERSION_MAJOR( device_properties.apiVersion );
        uint32_t minor_version = VK_VERSION_MINOR( device_properties.apiVersion );
        uint32_t patch_version = VK_VERSION_PATCH( device_properties.apiVersion );

        if( (major_version < 1) ||
            (device_properties.limits.maxImageDimension2D < 4096) ) {
            return false;
        }

        if (!FindQueueFamilies(physicalDevice, queue_family_index, presentQueueFamilyIndex)) {
            return false;
        }
        return true;
    }

    void CreateDevice() {
        uint32_t num_devices = 0;
        if( (instance.enumeratePhysicalDevices(&num_devices, NULL) != vk::Result::eSuccess) || (num_devices == 0) ) {
            std::cout << "Error occurred during physical devices enumeration!" << std::endl;
            Error();
        }

        std::vector<vk::PhysicalDevice> physical_devices( num_devices );
        if( (instance.enumeratePhysicalDevices(&num_devices, physical_devices.data()) != vk::Result::eSuccess) || (num_devices == 0) ) {
            std::cout << "Error occurred during physical devices enumeration!" << std::endl;
            Error();
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
            Error();
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
                                                static_cast<uint32_t>(deviceExtensions.size()),  // uint32_t                           enabledExtensionCount
                                                &deviceExtensions[0],                            // const char * const                 *ppEnabledExtensionNames
                                                nullptr);                                         // const vk::PhysicalDeviceFeatures   *pEnabledFeatures

        if( selected_physical_device->createDevice( &device_create_info, nullptr, &Ls::device ) != vk::Result::eSuccess ) {
            std::cout << "Could not create Vulkan device!" << std::endl;
            Error();
        }

        Ls::graphicsQueueFamilyIndex = selected_graphics_queue_family_index;
        Ls::presentQueueFamilyIndex = selected_present_queue_family_index;
        Ls::physicalDevice = *selected_physical_device;
    }

    void GetQueues() {
        device.getQueue( graphicsQueueFamilyIndex, 0, &graphicsQueue );
        device.getQueue( presentQueueFamilyIndex, 0, &presentQueue );
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

        if( (device.createSemaphore( &semaphore_create_info, nullptr, &imageAvailableSemaphore ) != vk::Result::eSuccess) ||
            (device.createSemaphore( &semaphore_create_info, nullptr, &renderingFinishedSemaphore ) != vk::Result::eSuccess) ) {
            std::cout << "Could not create semaphores!" << std::endl;
            Error();
        }
    }

    uint32_t GetSwapChainNumImages( vk::SurfaceCapabilitiesKHR &surface_capabilities, vk::PresentModeKHR chosen_present_mode) {
        uint32_t image_count;

        // select desired image cound based on present mode
        switch (chosen_present_mode) {
            case vk::PresentModeKHR::eMailbox:
                image_count = 3;
                break;
            case vk::PresentModeKHR::eFifo:
                image_count = 3;
                break;
            case vk::PresentModeKHR::eFifoRelaxed:
                image_count = 3;
                break;
            case vk::PresentModeKHR::eImmediate:
                image_count = 2;
                break;
        }

        std::cout << "Using image count " << image_count << " for swap chain" << std::endl; 

        // clamp image count to fit surface capabilites
        if (image_count < surface_capabilities.minImageCount) {
            image_count = surface_capabilities.minImageCount;
        } else if ( surface_capabilities.maxImageCount != 0 && 
                    image_count > surface_capabilities.maxImageCount ) {
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
        if( surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst ) { // TODO: Why can't I get eTransferDst?
            return vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
		}
		std::cout << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the swap chain!" << std::endl;
		Error();
		
	    return Optional<vk::ImageUsageFlags>::None();
		//return vk::ImageUsageFlagBits::eColorAttachment;
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
                std::cout << "Using mailbox present mode" << std::endl; 
                return present_mode;
            }
        }
        for( vk::PresentModeKHR &present_mode : present_modes ) {
            if( present_mode == vk::PresentModeKHR::eFifo ) {
                std::cout << "Using fifo present mode" << std::endl; 
                return present_mode;
            }
        }
        return Optional<vk::PresentModeKHR>();
    }

    void CreateSwapChain() {
        can_render = false;
        device.waitIdle();

        vk::SurfaceCapabilitiesKHR surface_capabilities;
        if( physicalDevice.getSurfaceCapabilitiesKHR(presentationSurface, &surface_capabilities) != vk::Result::eSuccess ) {
            std::cout << "Could not check presentation surface capabilities!" << std::endl;
            Error();
        }

        uint32_t formats_count;
        if( (physicalDevice.getSurfaceFormatsKHR( presentationSurface, &formats_count, nullptr ) != vk::Result::eSuccess) ||
            (formats_count == 0) ) {
            std::cout << "Error occurred during presentation surface formats enumeration!" << std::endl;
            Error();
        }

        std::vector<vk::SurfaceFormatKHR> surface_formats( formats_count );
        if( physicalDevice.getSurfaceFormatsKHR( presentationSurface, &formats_count, &surface_formats[0] ) != vk::Result::eSuccess ) {
            std::cout << "Error occurred during presentation surface formats enumeration!" << std::endl;
            Error();
        }

        uint32_t present_modes_count;
        if( (physicalDevice.getSurfacePresentModesKHR( presentationSurface, &present_modes_count, nullptr ) != vk::Result::eSuccess) ||
            (present_modes_count == 0) ) {
            std::cout << "Error occurred during presentation surface present modes enumeration!" << std::endl;
            Error();
        }

        std::vector<vk::PresentModeKHR> present_modes( present_modes_count );
        if( physicalDevice.getSurfacePresentModesKHR( presentationSurface , &present_modes_count, &present_modes[0] ) != vk::Result::eSuccess ) {
            std::cout << "Error occurred during presentation surface present modes enumeration!" << std::endl;
            Error();
        }

        vk::SurfaceFormatKHR            desired_format = GetSwapChainFormat( surface_formats );
        Optional<vk::Extent2D>          desired_extent = GetSwapChainExtent( surface_capabilities );
        Optional<vk::ImageUsageFlags>   desired_usage = GetSwapChainUsageFlags( surface_capabilities );
        vk::SurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform( surface_capabilities );
        Optional<vk::PresentModeKHR>    desired_present_mode = GetSwapChainPresentMode( present_modes );
        vk::SwapchainKHR                old_swapchain = swapChain;

        if( !desired_usage ) {
            std::cout << "Surface does not support any suitable usage flags!" << std::endl;
            Error();
        }
        if( !desired_present_mode ) {
            std::cout << "Surface does not support any suitable present modes!" << std::endl;
            Error();
        }

        uint32_t desired_number_of_images = GetSwapChainNumImages( surface_capabilities, desired_present_mode );

        if( !desired_extent ) {
            // Current surface size is (0, 0) so we can't create a swap chain and render anything (can_render == false)
            // But we don't wont to kill the application as this situation may occur i.e. when window gets minimized
            return;
        }

        vk::SwapchainCreateInfoKHR swap_chain_create_info(
            vk::SwapchainCreateFlagsKHR(),                // vk::SwapchainCreateFlagsKHR      flags
            presentationSurface,                          // vk::SurfaceKHR                   surface
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

        if( device.createSwapchainKHR( &swap_chain_create_info, nullptr, &swapChain ) != vk::Result::eSuccess ) {
            std::cout << "Could not create swap chain!" << std::endl;
            Error();
        }

        if( old_swapchain ) {
            device.destroySwapchainKHR( old_swapchain, nullptr );
        }
        can_render = true;
    }

    void RecordCommandBuffers() {
        uint32_t image_count = static_cast<uint32_t>(presentQueueCmdBuffers.size());

        std::vector<vk::Image> swap_chain_images( image_count );
        if( device.getSwapchainImagesKHR( swapChain, &image_count, &swap_chain_images[0] ) != vk::Result::eSuccess ) {
            std::cout << "Could not get swap chain images!" << std::endl;
            Error();
        }

        vk::CommandBufferBeginInfo cmd_buffer_begin_info(
            vk::CommandBufferUsageFlagBits::eSimultaneousUse, // VkCommandBufferUsageFlags              flags
            nullptr                                           // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
        );

		const std::array<float, 4> color = { 1.0f, 0.8f, 0.4f, 0.0f };
        vk::ClearColorValue clear_color(
			color
        );

        vk::ImageSubresourceRange image_subresource_range(
            vk::ImageAspectFlagBits::eColor,              // VkImageAspectFlags                     aspectMask
            0,                                            // uint32_t                               baseMipLevel
            1,                                            // uint32_t                               levelCount
            0,                                            // uint32_t                               baseArrayLayer
            1                                             // uint32_t                               layerCount
        );

        for( uint32_t i = 0; i < image_count; ++i ) {
            vk::ImageMemoryBarrier barrier_from_present_to_clear(
				vk::AccessFlagBits(),                       // VkAccessFlags                          srcAccessMask, eMemoryRead fails validation
                vk::AccessFlagBits::eTransferWrite,         // VkAccessFlags                          dstAccessMask
                vk::ImageLayout::eUndefined,                // VkImageLayout                          oldLayout
                vk::ImageLayout::eTransferDstOptimal,       // VkImageLayout                          newLayout
                VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
                VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
                swap_chain_images[i],                       // VkImage                                image
                image_subresource_range                     // VkImageSubresourceRange                subresourceRange
            );

            vk::ImageMemoryBarrier barrier_from_clear_to_present(
                vk::AccessFlagBits::eTransferWrite,         // VkAccessFlags                          srcAccessMask
                vk::AccessFlagBits::eMemoryRead,            // VkAccessFlags                          dstAccessMask
                vk::ImageLayout::eTransferDstOptimal,       // VkImageLayout                          oldLayout
                vk::ImageLayout::ePresentSrcKHR,            // VkImageLayout                          newLayout
                VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
                VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
                swap_chain_images[i],                       // VkImage                                image
                image_subresource_range                     // VkImageSubresourceRange                subresourceRange
            );

            presentQueueCmdBuffers[i].begin( &cmd_buffer_begin_info );
            presentQueueCmdBuffers[i].pipelineBarrier( vk::PipelineStageFlagBits::eTransfer, 
                                                       vk::PipelineStageFlagBits::eTransfer,
                                                       vk::DependencyFlagBits(),
                                                       0,
                                                       nullptr,
                                                       0,
                                                       nullptr,
                                                       1,
                                                       &barrier_from_present_to_clear );

            presentQueueCmdBuffers[i].clearColorImage( swap_chain_images[i],
				                                       vk::ImageLayout::eTransferDstOptimal,
				                                       &clear_color,
				                                       1,
				                                       &image_subresource_range );

            presentQueueCmdBuffers[i].pipelineBarrier( vk::PipelineStageFlagBits::eTransfer, 
                                                       vk::PipelineStageFlagBits::eBottomOfPipe,
                                                       vk::DependencyFlagBits(),
                                                       0,
                                                       nullptr,
                                                       0,
                                                       nullptr,
                                                       1,
                                                       &barrier_from_clear_to_present);

            if( presentQueueCmdBuffers[i].end() != vk::Result::eSuccess ) {
                std::cout << "Could not record command buffers!" << std::endl;
                Error();
            }
        }
    }

    void CreateCommandBuffers() {
        vk::CommandPoolCreateInfo cmd_pool_create_info(vk::CommandPoolCreateFlags(), presentQueueFamilyIndex);
        if (device.createCommandPool(&cmd_pool_create_info, nullptr, &presentQueueCmdPool) != vk::Result::eSuccess) {
            std::cout << "Could not create a command pool!" << std::endl;
            Error();
        }

        uint32_t image_count = 0;
        if( (device.getSwapchainImagesKHR( swapChain, &image_count, nullptr ) != vk::Result::eSuccess) ||
            (image_count == 0) ) {
            std::cout << "Could not get the number of swap chain images!" << std::endl;
            Error();
        }

        presentQueueCmdBuffers.resize( image_count );

        vk::CommandBufferAllocateInfo cmd_buffer_allocate_info(
            presentQueueCmdPool,              // VkCommandPool                commandPool
            vk::CommandBufferLevel::ePrimary, // VkCommandBufferLevel         level
            image_count);                     // uint32_t                     bufferCount

        if( device.allocateCommandBuffers( &cmd_buffer_allocate_info, &presentQueueCmdBuffers[0] ) != vk::Result::eSuccess ) {
            std::cout << "Could not allocate command buffers!" << std::endl;
            Error();
        }

        RecordCommandBuffers();
    }
    
    void FreeCommandBuffers() {
        if( Ls::device ) {
            Ls::device.waitIdle();
            if( (presentQueueCmdBuffers.size() > 0) && presentQueueCmdBuffers[0] ) {
                device.freeCommandBuffers( presentQueueCmdPool, static_cast<uint32_t>(presentQueueCmdBuffers.size()), &presentQueueCmdBuffers[0] );
                presentQueueCmdBuffers.clear();
                std::cout << "Command buffers freed" << std::endl;
            }
            if( presentQueueCmdPool ) {
                device.destroyCommandPool( presentQueueCmdPool, nullptr );
                presentQueueCmdPool = vk::CommandPool();
                std::cout << "Command pool destroyed" << std::endl;
            }
        }
    }

    void OnWindowSizeChanged() {
         if ( device ) {
             FreeCommandBuffers();
             CreateSwapChain();
             CreateCommandBuffers();
         }
    }

    void Draw() {
        uint32_t image_index;
        vk::Result result = device.acquireNextImageKHR( swapChain, UINT64_MAX, imageAvailableSemaphore, vk::Fence(), &image_index );
        switch( result ) {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                break;
            case vk::Result::eErrorOutOfDateKHR:
                std::cout << "Swap chain out of date" << std::endl;
                OnWindowSizeChanged();
                return;
            default:
                std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
                Error();
        }

        vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
        vk::SubmitInfo submit_info(
            1,                                     // uint32_t                     waitSemaphoreCount
            &imageAvailableSemaphore,              // const VkSemaphore           *pWaitSemaphores
            &wait_dst_stage_mask,                  // const VkPipelineStageFlags  *pWaitDstStageMask;
            1,                                     // uint32_t                     commandBufferCount
            &presentQueueCmdBuffers[image_index],  // const VkCommandBuffer       *pCommandBuffers
            1,                                     // uint32_t                     signalSemaphoreCount
            &renderingFinishedSemaphore            // const VkSemaphore           *pSignalSemaphores
        );

        if( presentQueue.submit( 1, &submit_info, vk::Fence() ) != vk::Result::eSuccess ) {
            std::cout << "Submit to queue failed!" << std::endl;
            Error();
        }

        vk::PresentInfoKHR present_info(
            1,                                     // uint32_t                     waitSemaphoreCount
            &renderingFinishedSemaphore,           // const VkSemaphore           *pWaitSemaphores
            1,                                     // uint32_t                     swapchainCount
            &swapChain,                            // const VkSwapchainKHR        *pSwapchains
            &image_index,                          // const uint32_t              *pImageIndices
            nullptr                                // VkResult                    *pResults
        );
        result = presentQueue.presentKHR( &present_info );

        switch( result ) {
          case vk::Result::eSuccess:
            break;
          case vk::Result::eErrorOutOfDateKHR:
          case vk::Result::eSuboptimalKHR:
            return OnWindowSizeChanged();
          default:
            std::cout << "Problem occurred during image presentation!" << std::endl;
            Error();
        }
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

        switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        case WM_PAINT:
            Draw();
            return 0;
            break;
        case WM_SIZE:
            OnWindowSizeChanged();
            return 0;
            break;
        default:
            break;
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
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Ls::hInstance = hInstance;
    Ls::width = 800;
    Ls::height = 600;

    AttachConsole();
    Ls::CreateMainWindow();
    
    vk::LoadVulkanLibrary();
    vk::LoadExportedEntryPoints();
    vk::LoadGlobalLevelEntryPoints();
    Ls::CheckValidationAvailability();

    Ls::CreateInstance();
    vk::LoadInstanceLevelEntryPoints(Ls::instance, Ls::instanceExtensions);

    Ls::CreateDebugReportCallback(); // needs an instance level function
	Ls::CreatePresentationSurface(); // need this for device creation

	Ls::CreateDevice();
    vk::LoadDeviceLevelEntryPoints(Ls::device, Ls::deviceExtensions);

	Ls::CreateSemaphores();
	Ls::GetQueues();
	Ls::CreateSwapChain();
    Ls::CreateCommandBuffers();
    
    while (Ls::Update()) {};
    
    Ls::FreeDevice();
    Ls::FreeInstance();
    vk::UnloadVulkanLibrary();
    return Ls::msg.wParam;
}
