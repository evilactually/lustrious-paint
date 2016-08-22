#include <windows.h>
#include "vk.hpp"
#include "version.h"
#include "Optional.hpp"
#include "win32_console.hpp"
#include "utility.h"
#include <map>
#include "assert.h"
//#include "mesh.h"

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
    bool dialogShowing = false;
    bool canRender = false;
    
    HINSTANCE hInstance;
    MSG msg;
    HWND windowHandle = nullptr;

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    struct {
        uint32_t  familyIndex;
        vk::Queue handle;
    } graphicsQueue;

    struct {
        uint32_t  familyIndex;
        vk::Queue handle;
    } presentQueue;

    struct {
        vk::SurfaceKHR   presentationSurface;
        vk::SwapchainKHR swapChain;
        vk::Format       format;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> imageViews;
        vk::Extent2D extent;
        uint32_t acquiredImageIndex;
    } swapChainInfo;

    struct {
        vk::Semaphore imageAvailable;
        vk::Semaphore renderingFinished;
    } semaphores;

    vk::Fence submitCompleteFence;

    //vk::CommandPool presentQueueCmdPool;                   // depricated, don't need command buffer to present
    //std::vector<vk::CommandBuffer> presentQueueCmdBuffers; // depricated
    vk::CommandPool graphicsCommandPool; 
    std::vector<vk::CommandBuffer> graphicsCommandBuffers;
    vk::CommandBuffer commandBuffer;            // No pre-recording, so reusing same command buffer
    uint32_t commandBufferIndex;
    
    vk::RenderPass renderPass;
    std::vector<vk::Framebuffer> framebuffers;
    vk::Pipeline graphicsPipeline;
    vk::Pipeline linePipeline;
    vk::Pipeline pointPipeline;
    vk::PipelineLayout pipelineLayout;
    vk::PipelineLayout linePipelineLayout;
    vk::PipelineLayout pointPipelineLayout;

    vk::DebugReportCallbackEXT debugReportCallback;

    std::vector<const char*> INSTANCE_EXTENSIONS = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };
    std::vector<const char*> DEVICE_EXTENSIONS = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<const char*> INSTANCE_LAYERS = {
      "VK_LAYER_LUNARG_standard_validation"
    };
    std::map<std::string, vk::ShaderModule> shaders;

    struct LinePushConstants {
      float positions[4];
      float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
      //float size = 1.0f;
      //float size2 = 1.0f;
    };

    struct PointPushConstants {
      float positions[2];
      float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
      float size = 1.0f;
      //float size2 = 1.0f;
      //float data[2+4+1];
    };

    struct {
      float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
      float lineWidth = 1.0f;
      float pointSize = 1.0f;
      bool drawing = false; // indicates that command buffer is ready to draw

    } drawingContext;

    VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback( VkDebugReportFlagsEXT flags, 
        VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, 
        int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData ) {
        std::cout << pLayerPrefix << ": " << pMessage << std::endl;
        return VK_FALSE;
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

        for( size_t i = 0; i < INSTANCE_LAYERS.size(); ++i ) {
            bool found = false;
            for (size_t j = 0; j < availableLayers.size(); ++j)
            {
                if ( strcmp( INSTANCE_LAYERS[i], availableLayers[j].layerName ) == 0 ) {
                    found = true;
                    break;
                }
            }
            if ( !found )
            {
                std::cout << "Instance layer " << INSTANCE_LAYERS[i] << " not found!" << std::endl; 
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
            std::cout << "Error occurred during instance INSTANCE_EXTENSIONS enumeration!" << std::endl;
            Error();
        }

        std::vector<vk::ExtensionProperties> available_instance_extensions( instance_extensions_count );
        if( vk::enumerateInstanceExtensionProperties( nullptr, &instance_extensions_count, &available_instance_extensions[0] ) != vk::Result::eSuccess ) {
            std::cout << "Error occurred during instance INSTANCE_EXTENSIONS enumeration!" << std::endl;
            Error();
        }

        for( size_t i = 0; i < INSTANCE_EXTENSIONS.size(); ++i ) {
            if( !CheckExtensionAvailability( INSTANCE_EXTENSIONS[i], available_instance_extensions ) ) {
                Abort(std::string("Could not find instance extension named \"") + 
                      std::string(INSTANCE_EXTENSIONS[i]) + 
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
                                                  static_cast<uint32_t>(INSTANCE_LAYERS.size()),
                                                  &INSTANCE_LAYERS[0],
                                                  static_cast<uint32_t>(INSTANCE_EXTENSIONS.size()),
                                                  &INSTANCE_EXTENSIONS[0]);
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

        if( instance.createWin32SurfaceKHR( &surface_create_info, nullptr, &swapChainInfo.presentationSurface ) != vk::Result::eSuccess ) {
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
            physicalDevice.getSurfaceSupportKHR( i, swapChainInfo.presentationSurface, &queue_present_support[i] );

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

        for( size_t i = 0; i < DEVICE_EXTENSIONS.size(); ++i ) {
            if( !CheckExtensionAvailability( DEVICE_EXTENSIONS[i], available_extensions ) ) {
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

        vk::PhysicalDeviceFeatures device_features;
        device_features.wideLines = VK_TRUE;

        vk::DeviceCreateInfo device_create_info(vk::DeviceCreateFlags(),                          // vk::DeviceCreateFlags              flags
                                                static_cast<uint32_t>(queue_create_infos.size()), // uint32_t                           queueCreateInfoCount
                                                &queue_create_infos[0],                           // const VkDeviceQueueCreateInfo      *pQueueCreateInfos
                                                0,                                                // uint32_t                           enabledLayerCount
                                                nullptr,                                          // const char * const                 *ppEnabledLayerNames
                                                static_cast<uint32_t>(DEVICE_EXTENSIONS.size()),  // uint32_t                           enabledExtensionCount
                                                &DEVICE_EXTENSIONS[0],                            // const char * const                 *ppEnabledExtensionNames
                                                &device_features);                                // const vk::PhysicalDeviceFeatures   *pEnabledFeatures

        if( selected_physical_device->createDevice( &device_create_info, nullptr, &Ls::device ) != vk::Result::eSuccess ) {
            std::cout << "Could not create Vulkan device!" << std::endl;
            Error();
        }

        Ls::graphicsQueue.familyIndex = selected_graphics_queue_family_index;
        Ls::presentQueue.familyIndex = selected_present_queue_family_index;
        Ls::physicalDevice = *selected_physical_device;
    }

    void GetQueues() {
        device.getQueue( graphicsQueue.familyIndex, 0, &graphicsQueue.handle );
        device.getQueue( presentQueue.familyIndex, 0, &presentQueue.handle );
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

        if( (device.createSemaphore( &semaphore_create_info, nullptr, &semaphores.imageAvailable ) != vk::Result::eSuccess) ||
            (device.createSemaphore( &semaphore_create_info, nullptr, &semaphores.renderingFinished ) != vk::Result::eSuccess) ) {
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
        if( surface_capabilities.currentExtent.width == -1 || TRUE) {
          // Try setting extent to the size of window client area
          RECT window_rect = {}; 
          GetClientRect(windowHandle, &window_rect);
          VkExtent2D swap_chain_extent = { static_cast<uint32_t>(window_rect.right - window_rect.left),
                                           static_cast<uint32_t>(window_rect.bottom - window_rect.top) };

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
          Optional<vk::Extent2D>::None();
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
      return Optional<vk::ImageUsageFlags>::None();
    }

    vk::SurfaceTransformFlagBitsKHR GetSwapChainTransform( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
      if( surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ) {
        return vk::SurfaceTransformFlagBitsKHR::eIdentity;
      } else {
        return surface_capabilities.currentTransform;
      }
    }

    Optional<vk::PresentModeKHR> GetSwapChainPresentMode( std::vector<vk::PresentModeKHR> &present_modes ) {
      for( vk::PresentModeKHR &present_mode : present_modes ) {
        if( present_mode == vk::PresentModeKHR::eImmediate ) {
          std::cout << "Using immediate present mode" << std::endl; 
          return present_mode;
        }
      }
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

    void CreateSwapChainImageViews() {
        swapChainInfo.imageViews.resize(swapChainInfo.images.size());
        for( size_t i = 0; i < swapChainInfo.images.size(); ++i ) {
            vk::ImageViewCreateInfo image_view_create_info(
                vk::ImageViewCreateFlags(),             // VkImageViewCreateFlags         flags
                swapChainInfo.images[i],                // VkImage                        image
                vk::ImageViewType::e2D,                 // VkImageViewType                viewType
                swapChainInfo.format,                   // VkFormat                       format
                {                                       // VkComponentMapping             components
                    vk::ComponentSwizzle::eIdentity,    // VkComponentSwizzle             r
                    vk::ComponentSwizzle::eIdentity,    // VkComponentSwizzle             g
                    vk::ComponentSwizzle::eIdentity,    // VkComponentSwizzle             b
                    vk::ComponentSwizzle::eIdentity     // VkComponentSwizzle             a
                },
                {                                       // VkImageSubresourceRange        subresourceRange
                    vk::ImageAspectFlagBits::eColor,    // VkImageAspectFlags             aspectMask
                    0,                                  // uint32_t                       baseMipLevel
                    1,                                  // uint32_t                       levelCount
                    0,                                  // uint32_t                       baseArrayLayer
                    1                                   // uint32_t                       layerCount
                }
            );

            if( device.createImageView( &image_view_create_info, nullptr, &swapChainInfo.imageViews[i] ) != vk::Result::eSuccess ) {
                std::cout << "Could not create image view for framebuffer!" << std::endl;
                Error();
            }
        }
        canRender = true;
    }

    void CreateSwapChain() {
        canRender = false;
        device.waitIdle();

        vk::SurfaceCapabilitiesKHR surface_capabilities;
        if( physicalDevice.getSurfaceCapabilitiesKHR(swapChainInfo.presentationSurface, &surface_capabilities) != vk::Result::eSuccess ) {
            std::cout << "Could not check presentation surface capabilities!" << std::endl;
            Error();
        }

        uint32_t formats_count;
        if( (physicalDevice.getSurfaceFormatsKHR( swapChainInfo.presentationSurface, &formats_count, nullptr ) != vk::Result::eSuccess) ||
            (formats_count == 0) ) {
            std::cout << "Error occurred during presentation surface formats enumeration!" << std::endl;
            Error();
        }

        std::vector<vk::SurfaceFormatKHR> surface_formats( formats_count );
        if( physicalDevice.getSurfaceFormatsKHR( swapChainInfo.presentationSurface, &formats_count, &surface_formats[0] ) != vk::Result::eSuccess ) {
            std::cout << "Error occurred during presentation surface formats enumeration!" << std::endl;
            Error();
        }

        uint32_t present_modes_count;
        if( (physicalDevice.getSurfacePresentModesKHR( swapChainInfo.presentationSurface, &present_modes_count, nullptr ) != vk::Result::eSuccess) ||
            (present_modes_count == 0) ) {
            std::cout << "Error occurred during presentation surface present modes enumeration!" << std::endl;
            Error();
        }

        std::vector<vk::PresentModeKHR> present_modes( present_modes_count );
        if( physicalDevice.getSurfacePresentModesKHR( swapChainInfo.presentationSurface , &present_modes_count, &present_modes[0] ) != vk::Result::eSuccess ) {
            std::cout << "Error occurred during presentation surface present modes enumeration!" << std::endl;
            Error();
        }

        vk::SurfaceFormatKHR            desired_format = GetSwapChainFormat( surface_formats );
        Optional<vk::Extent2D>          desired_extent = GetSwapChainExtent( surface_capabilities );
        Optional<vk::ImageUsageFlags>   desired_usage = GetSwapChainUsageFlags( surface_capabilities );
        vk::SurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform( surface_capabilities );
        Optional<vk::PresentModeKHR>    desired_present_mode = GetSwapChainPresentMode( present_modes );
        vk::SwapchainKHR                old_swapchain = swapChainInfo.swapChain;

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
            // Current surface size is (0, 0) so we can't create a swap chain and render anything (canRender == false)
            // But we don't wont to kill the application as this situation may occur i.e. when window gets minimized
            return;
        }

        vk::SwapchainCreateInfoKHR swap_chain_create_info(
            vk::SwapchainCreateFlagsKHR(),                // vk::SwapchainCreateFlagsKHR      flags
            swapChainInfo.presentationSurface,            // vk::SurfaceKHR                   surface
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

        if( device.createSwapchainKHR( &swap_chain_create_info, nullptr, &swapChainInfo.swapChain ) != vk::Result::eSuccess ) {
            std::cout << "Could not create swap chain!" << std::endl;
            Error();
        }

        if( old_swapchain ) {
            device.destroySwapchainKHR( old_swapchain, nullptr );
        }

        swapChainInfo.format = desired_format.format; // for creating attachment, image views, etc
        swapChainInfo.extent = desired_extent;        // for framebuffers

        // get swap chain images
        uint32_t image_count = 0;
        if( (device.getSwapchainImagesKHR( swapChainInfo.swapChain, &image_count, nullptr ) != vk::Result::eSuccess) ||
            (image_count == 0) ) {
            std::cout << "Could not get the number of swap chain images!" << std::endl;
            Error();
        }

        swapChainInfo.images.resize( static_cast<size_t>(image_count) );
        if( device.getSwapchainImagesKHR( swapChainInfo.swapChain, &image_count, &swapChainInfo.images[0] ) != vk::Result::eSuccess ) {
            std::cout << "Could not get swap chain images!" << std::endl;
            Error();
        }

        CreateSwapChainImageViews();

        canRender = true;
    }

    void CreateCommandBuffers() {
        vk::CommandPoolCreateInfo cmd_pool_create_info(
          vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // allow resetting command buffer
          graphicsQueue.familyIndex
        );

        if (device.createCommandPool(&cmd_pool_create_info, nullptr, &graphicsCommandPool) != vk::Result::eSuccess) {
            std::cout << "Could not create a command pool!" << std::endl;
            Error();
        }

        // uint32_t image_count = 0;
        // if( (device.getSwapchainImagesKHR( swapChainInfo.swapChain, &image_count, nullptr ) != vk::Result::eSuccess) ||
        //     (image_count == 0) ) {
        //     std::cout << "Could not get the number of swap chain images!" << std::endl;
        //     Error();
        // }

        graphicsCommandBuffers.resize( swapChainInfo.images.size() );

        vk::CommandBufferAllocateInfo cmd_buffer_allocate_info(
            graphicsCommandPool,                               // VkCommandPool                commandPool
            vk::CommandBufferLevel::ePrimary,                  // VkCommandBufferLevel         level
            static_cast<size_t>(swapChainInfo.images.size())   // uint32_t                     bufferCount
        );

        if( device.allocateCommandBuffers( &cmd_buffer_allocate_info, &graphicsCommandBuffers[0] ) != vk::Result::eSuccess ) {
            std::cout << "Could not allocate command buffers!" << std::endl;
            Error();
        }

        vk::CommandBufferAllocateInfo cmd_buffer_allocate_info_2(
          graphicsCommandPool,                               // VkCommandPool                commandPool
          vk::CommandBufferLevel::ePrimary,                  // VkCommandBufferLevel         level
          1);                                                // uint32_t                     bufferCount

        if( device.allocateCommandBuffers( &cmd_buffer_allocate_info_2, &commandBuffer ) != vk::Result::eSuccess ) {
            std::cout << "Could not allocate command buffers!" << std::endl;
            Error();
        }
    }
    
    void FreeCommandBuffers() {
        if( Ls::device ) {
            Ls::device.waitIdle();
            if( (graphicsCommandBuffers.size() > 0) && graphicsCommandBuffers[0] ) {
                device.freeCommandBuffers( graphicsCommandPool, static_cast<uint32_t>(graphicsCommandBuffers.size()), &graphicsCommandBuffers[0] );
                graphicsCommandBuffers.clear();
                std::cout << "Command buffers freed" << std::endl;
            }
            if( graphicsCommandPool ) {
                device.destroyCommandPool( graphicsCommandPool, nullptr );
                graphicsCommandPool = vk::CommandPool();
                std::cout << "Command pool destroyed" << std::endl;
            }
        }
    }

    void CreateRenderPass() {
        // NOTE: Setting loadOp to eClear requires later that we provide VkClearValue in VkRenderPassBeginInfo.
        vk::AttachmentDescription attachment_descriptions[] = {
            { 
                vk::AttachmentDescriptionFlags(),        // VkAttachmentDescriptionFlags   flags
                swapChainInfo.format,                    // VkFormat                       format
                vk::SampleCountFlagBits::e1,             // VkSampleCountFlagBits          samples
                vk::AttachmentLoadOp::eLoad,             // VkAttachmentLoadOp             loadOp
                vk::AttachmentStoreOp::eStore,           // VkAttachmentStoreOp            storeOp
                vk::AttachmentLoadOp::eDontCare,         // VkAttachmentLoadOp             stencilLoadOp
                vk::AttachmentStoreOp::eDontCare,        // VkAttachmentStoreOp            stencilStoreOp
                vk::ImageLayout::ePresentSrcKHR,         // VkImageLayout                  initialLayout
                vk::ImageLayout::ePresentSrcKHR          // VkImageLayout                  finalLayout
            }
        };

        vk::AttachmentReference color_attachment_references[] = {
            {
                0,                                        // uint32_t                       attachment
                vk::ImageLayout::eColorAttachmentOptimal, // VkImageLayout                  layout
            }
        };

        vk::SubpassDescription subpass_descriptions[] = {
            {
                vk::SubpassDescriptionFlags(),            // VkSubpassDescriptionFlags      flags
                vk::PipelineBindPoint::eGraphics,         // VkPipelineBindPoint            pipelineBindPoint
                0,                                        // uint32_t                       inputAttachmentCount
                nullptr,                                  // const VkAttachmentReference    *pInputAttachments
                1,                                        // uint32_t                       colorAttachmentCount
                color_attachment_references,              // const VkAttachmentReference    *pColorAttachments
                nullptr,                                  // const VkAttachmentReference    *pResolveAttachments
                nullptr,                                  // const VkAttachmentReference    *pDepthStencilAttachment
                0,                                        // uint32_t                       preserveAttachmentCount
                nullptr                                   // const uint32_t*                pPreserveAttachments
            }
        };

        vk::RenderPassCreateInfo render_pass_create_info = {
            vk::RenderPassCreateFlags(),                  // VkRenderPassCreateFlags        flags
            1,                                            // uint32_t                       attachmentCount
            attachment_descriptions,                      // const VkAttachmentDescription  *pAttachments
            1,                                            // uint32_t                       subpassCount
            subpass_descriptions,                         // const VkSubpassDescription     *pSubpasses
            0,                                            // uint32_t                       dependencyCount
            nullptr                                       // const VkSubpassDependency      *pDependencies
        };

        if( device.createRenderPass( &render_pass_create_info, nullptr, &renderPass ) != vk::Result::eSuccess ) {
            std::cout << "Could not create render pass!" << std::endl;
            Error();
        }
    }

    void CreateFramebuffers() {
        framebuffers.resize(swapChainInfo.images.size());

        for (size_t i = 0; i < swapChainInfo.images.size(); ++i) {
            vk::FramebufferCreateInfo framebuffer_create_info(
                vk::FramebufferCreateFlags(),                                // VkFramebufferCreateFlags       flags
                renderPass,                                                  // VkRenderPass                   renderPass
                1, // same as number of attachments from CreateRenderPass    // uint32_t                       attachmentCount
                &swapChainInfo.imageViews[i],                                // const VkImageView              *pAttachments
                swapChainInfo.extent.width,                                  // uint32_t                       width
                swapChainInfo.extent.height,                                 // uint32_t                       height
                1);                                                          // uint32_t                       layers

            if( device.createFramebuffer( &framebuffer_create_info, nullptr, &framebuffers[i] ) != vk::Result::eSuccess ) {
                std::cout << "Could not create a framebuffer!" << std::endl;
                Error();
            }
        }
    }

    vk::ShaderModule CreateShaderModule( const char* filename ) {
        const std::vector<char> code = GetBinaryFileContents( filename );
        if( code.size() == 0 ) {
            Error();
        }

        vk::ShaderModuleCreateInfo shader_module_create_info = {
            vk::ShaderModuleCreateFlags(),                  // VkShaderModuleCreateFlags      flags
            code.size(),                                    // size_t                         codeSize
            reinterpret_cast<const uint32_t*>(&code[0])     // const uint32_t                *pCode
        };

        vk::ShaderModule shader_module;
        if( device.createShaderModule( &shader_module_create_info, nullptr, &shader_module ) != vk::Result::eSuccess ) {
            std::cout << "Could not create shader module from a \"" << filename << "\" file!" << std::endl;
            Error();
        }
        return shader_module;
    }

    void CreatePipelineLayout() {
      vk::PushConstantRange linePushConstantRange = {
        vk::ShaderStageFlagBits::eVertex | 
        vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof(LinePushConstants)
      };

      vk::PushConstantRange pointPushConstantRange = {
        vk::ShaderStageFlagBits::eVertex | 
        vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof(PointPushConstants)
      };
      
      vk::PipelineLayoutCreateInfo lineLayoutCreateInfo = {
        vk::PipelineLayoutCreateFlags(),                // VkPipelineLayoutCreateFlags    flags
        0,                                              // uint32_t                       setLayoutCount
        nullptr,                                        // const VkDescriptorSetLayout    *pSetLayouts
        1/*0*/,                                         // uint32_t                       pushConstantRangeCount
        &linePushConstantRange                          // const VkPushConstantRange      *pPushConstantRanges
      };

      vk::PipelineLayoutCreateInfo pointLayoutCreateInfo = {
        vk::PipelineLayoutCreateFlags(),                // VkPipelineLayoutCreateFlags    flags
        0,                                              // uint32_t                       setLayoutCount
        nullptr,                                        // const VkDescriptorSetLayout    *pSetLayouts
        1/*0*/,                                         // uint32_t                       pushConstantRangeCount
        &pointPushConstantRange                         // const VkPushConstantRange      *pPushConstantRanges
      };

      if( device.createPipelineLayout( &lineLayoutCreateInfo, nullptr, &pipelineLayout ) != vk::Result::eSuccess ) {
        std::cout << "Could not create pipeline layout!" << std::endl;
        Error();
      }

      if( device.createPipelineLayout( &lineLayoutCreateInfo, nullptr, &linePipelineLayout ) != vk::Result::eSuccess ) {
        std::cout << "Could not create pipeline layout!" << std::endl;
        Error();
      }

      if( device.createPipelineLayout( &pointLayoutCreateInfo, nullptr, &pointPipelineLayout ) != vk::Result::eSuccess ) {
        std::cout << "Could not create pipeline layout!" << std::endl;
        Error();
      }
    }

    void CreatePipeline() {
        Ls::shaders["line.vert"] = Ls::CreateShaderModule("shaders/line.vert.spv");
        Ls::shaders["line.frag"] = Ls::CreateShaderModule("shaders/line.frag.spv");
        Ls::shaders["point.vert"] = Ls::CreateShaderModule("shaders/point.vert.spv");
        Ls::shaders["point.frag"] = Ls::CreateShaderModule("shaders/point.frag.spv");

        std::vector<vk::PipelineShaderStageCreateInfo> line_shader_stage_create_infos = {
          // Vertex shader
          {
            vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
            vk::ShaderStageFlagBits::eVertex,                         // VkShaderStageFlagBits                          stage
            Ls::shaders["line.vert"],                                 // VkShaderModule                                 module
            "main",                                                   // const char                                    *pName
            nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
          },
          // Fragment shader
          {
            vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
            vk::ShaderStageFlagBits::eFragment,                       // VkShaderStageFlagBits                          stage
            Ls::shaders["line.frag"],                                 // VkShaderModule                                 module
            "main",                                                   // const char                                    *pName
            nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
          }
        };

        std::vector<vk::PipelineShaderStageCreateInfo> point_shader_stage_create_infos = {
          // Vertex shader
          {
            vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
            vk::ShaderStageFlagBits::eVertex,                         // VkShaderStageFlagBits                          stage
            Ls::shaders["point.vert"],                                // VkShaderModule                                 module
            "main",                                                   // const char                                    *pName
            nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
          },
          // Fragment shader
          {
            vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
            vk::ShaderStageFlagBits::eFragment,                       // VkShaderStageFlagBits                          stage
            Ls::shaders["point.frag"],                                // VkShaderModule                                 module
            "main",                                                   // const char                                    *pName
            nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
          }
        };

        vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info(
          vk::PipelineVertexInputStateCreateFlags(),       // VkPipelineVertexInputStateCreateFlags          flags;
          0,                                               // uint32_t                                       vertexBindingDescriptionCount
          nullptr,                                         // const VkVertexInputBindingDescription         *pVertexBindingDescriptions
          0,                                               // uint32_t                                       vertexAttributeDescriptionCount
          nullptr                                          // const VkVertexInputAttributeDescription       *pVertexAttributeDescriptions
        );

        vk::PipelineInputAssemblyStateCreateInfo line_input_assembly_state_create_info = {
          vk::PipelineInputAssemblyStateCreateFlags(),                  // VkPipelineInputAssemblyStateCreateFlags        flags
          vk::PrimitiveTopology::eLineList,                             // VkPrimitiveTopology                            topology
          VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
        };

        vk::PipelineInputAssemblyStateCreateInfo point_input_assembly_state_create_info = {
          vk::PipelineInputAssemblyStateCreateFlags(),                  // VkPipelineInputAssemblyStateCreateFlags        flags
          vk::PrimitiveTopology::ePointList,                            // VkPrimitiveTopology                            topology
          VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
        };

        vk::Viewport viewport = {
          0.0f,                                                         // float                                          x
          0.0f,                                                         // float                                          y
          static_cast<float>(swapChainInfo.extent.width),               // float                                          width
          static_cast<float>(swapChainInfo.extent.height),              // float                                          height
          0.0f,                                                         // float                                          minDepth
          1.0f                                                          // float                                          maxDepth
        };

        vk::Rect2D scissor = {
          {                                                             // VkOffset2D                                     offset
            0,                                                          // int32_t                                        x
            0                                                           // int32_t                                        y
          },
          {                                                             // VkExtent2D                                     extent
            static_cast<uint32_t>(swapChainInfo.extent.width),          // uint32_t                                       width
            static_cast<uint32_t>(swapChainInfo.extent.height)          // uint32_t                                       height
          }
        };

        vk::PipelineViewportStateCreateInfo viewport_state_create_info = {
          vk::PipelineViewportStateCreateFlags(),                       // VkPipelineViewportStateCreateFlags             flags
          1,                                                            // uint32_t                                       viewportCount
          &viewport,                                                    // const VkViewport                              *pViewports
          1,                                                            // uint32_t                                       scissorCount
          &scissor                                                      // const VkRect2D                                *pScissors
        };

        vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info = {
          vk::PipelineRasterizationStateCreateFlags(),                  // VkPipelineRasterizationStateCreateFlags        flags
          VK_FALSE,                                                     // VkBool32                                       depthClampEnable
          VK_FALSE,                                                     // VkBool32                                       rasterizerDiscardEnable
          vk::PolygonMode::eFill,                                       // VkPolygonMode                                  polygonMode
          vk::CullModeFlagBits::eBack,                                  // VkCullModeFlags                                cullMode
          vk::FrontFace::eCounterClockwise,                             // VkFrontFace                                    frontFace
          VK_FALSE,                                                     // VkBool32                                       depthBiasEnable
          0.0f,                                                         // float                                          depthBiasConstantFactor
          0.0f,                                                         // float                                          depthBiasClamp
          0.0f,                                                         // float                                          depthBiasSlopeFactor
          1.0f                                                          // float                                          lineWidth
        };

        vk::PipelineMultisampleStateCreateInfo multisample_state_create_info = {
          vk::PipelineMultisampleStateCreateFlags(),                    // VkPipelineMultisampleStateCreateFlags          flags
          vk::SampleCountFlagBits::e1,                                  // VkSampleCountFlagBits                          rasterizationSamples
          VK_FALSE,                                                     // VkBool32                                       sampleShadingEnable
          1.0f,                                                         // float                                          minSampleShading
          nullptr,                                                      // const VkSampleMask                            *pSampleMask
          VK_FALSE,                                                     // VkBool32                                       alphaToCoverageEnable
          VK_FALSE                                                      // VkBool32                                       alphaToOneEnable
        };

        vk::PipelineColorBlendAttachmentState color_blend_attachment_state = {
          VK_FALSE,                                                     // VkBool32                                       blendEnable
          vk::BlendFactor::eOne,                                        // VkBlendFactor                                  srcColorBlendFactor
          vk::BlendFactor::eZero,                                       // VkBlendFactor                                  dstColorBlendFactor
          vk::BlendOp::eAdd,                                            // VkBlendOp                                      colorBlendOp
          vk::BlendFactor::eOne,                                        // VkBlendFactor                                  srcAlphaBlendFactor
          vk::BlendFactor::eZero,                                       // VkBlendFactor                                  dstAlphaBlendFactor
          vk::BlendOp::eAdd,                                            // VkBlendOp                                      alphaBlendOp
          vk::ColorComponentFlagBits::eR |                              // VkColorComponentFlags                          colorWriteMask
          vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB |
          vk::ColorComponentFlagBits::eA
        };

        vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info = {
          vk::PipelineColorBlendStateCreateFlags(),                     // VkPipelineColorBlendStateCreateFlags           flags
          VK_FALSE,                                                     // VkBool32                                       logicOpEnable
          vk::LogicOp::eCopy,                                           // VkLogicOp                                      logicOp
          1,                                                            // uint32_t                                       attachmentCount
          &color_blend_attachment_state,                                // const VkPipelineColorBlendAttachmentState     *pAttachments
          { 0.0f, 0.0f, 0.0f, 0.0f }                                    // float                                          blendConstants[4]
        };

        std::vector<vk::DynamicState> line_dynamic_states = {
          vk::DynamicState::eLineWidth
          //vk::DynamicState::eViewport,
          //vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo line_dynamic_state_create_info = {
          vk::PipelineDynamicStateCreateFlags(),                        // VkPipelineDynamicStateCreateFlags              flags
          static_cast<uint32_t>(line_dynamic_states.size()),            // uint32_t                                       dynamicStateCount
          &line_dynamic_states[0]                                       // const VkDynamicState                          *pDynamicStates
        };

        vk::GraphicsPipelineCreateInfo line_pipeline_create_info(
          vk::PipelineCreateFlags(),                                    // VkPipelineCreateFlags                          flags
          static_cast<uint32_t>(line_shader_stage_create_infos.size()), // uint32_t                                       stageCount
          &line_shader_stage_create_infos[0],                           // const VkPipelineShaderStageCreateInfo         *pStages
          &vertex_input_state_create_info,                              // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
          &line_input_assembly_state_create_info,                       // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
          nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
          &viewport_state_create_info,                                  // const VkPipelineViewportStateCreateInfo       *pViewportState
          &rasterization_state_create_info,                             // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
          &multisample_state_create_info,                               // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
          nullptr,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
          &color_blend_state_create_info,                               // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
          &line_dynamic_state_create_info,                              // const VkPipelineDynamicStateCreateInfo        *pDynamicState
          linePipelineLayout,                                           // VkPipelineLayout                               layout
          renderPass,                                                   // VkRenderPass                                   renderPass
          0,                                                            // uint32_t                                       subpass
          vk::Pipeline(),                                               // VkPipeline                                     basePipelineHandle
          -1                                                            // int32_t                                        basePipelineIndex
        );

        vk::GraphicsPipelineCreateInfo point_pipeline_create_info(
          vk::PipelineCreateFlags(),                                    // VkPipelineCreateFlags                          flags
          static_cast<uint32_t>(point_shader_stage_create_infos.size()), // uint32_t                                       stageCount
          &point_shader_stage_create_infos[0],                           // const VkPipelineShaderStageCreateInfo         *pStages
          &vertex_input_state_create_info,                              // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
          &point_input_assembly_state_create_info,                       // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
          nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
          &viewport_state_create_info,                                  // const VkPipelineViewportStateCreateInfo       *pViewportState
          &rasterization_state_create_info,                             // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
          &multisample_state_create_info,                               // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
          nullptr,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
          &color_blend_state_create_info,                               // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
          nullptr,                                                      // const VkPipelineDynamicStateCreateInfo        *pDynamicState
          pointPipelineLayout,                                               // VkPipelineLayout                               layout
          renderPass,                                                   // VkRenderPass                                   renderPass
          0,                                                            // uint32_t                                       subpass
          vk::Pipeline(),                                               // VkPipeline                                     basePipelineHandle
          -1                                                            // int32_t                                        basePipelineIndex
        );

        if( device.createGraphicsPipelines( vk::PipelineCache(), 1, &line_pipeline_create_info, nullptr, &graphicsPipeline ) != vk::Result::eSuccess ) {
          std::cout << "Could not create graphics pipeline!" << std::endl;
          Error();
        }

        if( device.createGraphicsPipelines( vk::PipelineCache(), 1, &line_pipeline_create_info, nullptr, &linePipeline ) != vk::Result::eSuccess ) {
          std::cout << "Could not create graphics pipeline!" << std::endl;
          Error();
        }

        if( device.createGraphicsPipelines( vk::PipelineCache(), 1, &point_pipeline_create_info, nullptr, &pointPipeline ) != vk::Result::eSuccess ) {
          std::cout << "Could not create graphics pipeline!" << std::endl;
          Error();
        }
    }

    void CreateFence() {
      // Fence used to protect command buffer while submitting
      vk::FenceCreateInfo fence_create_info = {
        vk::FenceCreateFlagBits::eSignaled // VkFenceCreateFlags flags
      };

      if( device.createFence( &fence_create_info, nullptr, &submitCompleteFence ) != vk::Result::eSuccess ) {
        std::cout << "Could not create a fence!" << std::endl;
        Error();
      }
    }

    void DestroyRenderpass() {
      if ( renderPass )
      {
        device.destroyRenderPass(renderPass, nullptr);
      }
    }

    void DestroyFramebuffers() {
      for ( const vk::Framebuffer& framebuffer : framebuffers ) {
        device.destroyFramebuffer(framebuffer, nullptr);
      }
    }

    void DestroyPipeline() {
      if ( graphicsPipeline )
      {
        device.destroyPipeline(graphicsPipeline, nullptr);
      }
    }

    void RefreshSwapChain() {
      if ( device ) {
        DestroyRenderpass();
        DestroyFramebuffers();
        DestroyPipeline();
        FreeCommandBuffers();

        CreateSwapChain(); // deletes old swap chain, because we need to specify what swap chain it replaces
        CreateRenderPass();
        CreateFramebuffers();
        CreatePipeline();
        CreateCommandBuffers();
      }
    }

    void BeginDrawing() {
      assert(!drawingContext.drawing);

      // If queue present and graphics queue families are not the same
      // transfer ownership to graphics queue
      vk::ImageSubresourceRange image_subresource_range(
        vk::ImageAspectFlagBits::eColor, // VkImageAspectFlags                     aspectMask
        0,                               // uint32_t                               baseMipLevel
        1,                               // uint32_t                               levelCount
        0,                               // uint32_t                               baseArrayLayer
        1                                // uint32_t                               layerCount
      );

      // wait for writes from clears and draws, block draws
      vk::ImageMemoryBarrier barrier_from_present_to_draw(
        vk::AccessFlagBits(),                                   // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
        vk::AccessFlagBits::eColorAttachmentWrite,              // VkAccessFlags            dstAccessMask
        vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            oldLayout
        vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            newLayout
        presentQueue.familyIndex,                               // uint32_t                 srcQueueFamilyIndex
        graphicsQueue.familyIndex,                              // uint32_t                 dstQueueFamilyIndex
        swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
        image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
      );

      commandBuffer.pipelineBarrier( 
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::DependencyFlagBits(),
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier_from_present_to_draw);

      // Begin rendering pass
      vk::RenderPassBeginInfo render_pass_begin_info = {
        renderPass,                                     // VkRenderPass                   renderPass
        framebuffers[swapChainInfo.acquiredImageIndex], // VkFramebuffer                  framebuffer
        {                                               // VkRect2D                       renderArea
          {                                             // VkOffset2D                     offset
            0,                                          // int32_t                        x
            0                                           // int32_t                        y
          },
          {                                             // VkExtent2D                     extent
            swapChainInfo.extent.width,                 // int32_t                        width
            swapChainInfo.extent.height,                // int32_t                        height
          }
        },
        0,                                              // uint32_t                       clearValueCount
        nullptr                                         // const VkClearValue            *pClearValues
      };
      commandBuffer.beginRenderPass( &render_pass_begin_info, vk::SubpassContents::eInline );

      // Tell drawing functions that command buffer is already prepared for drawing
      drawingContext.drawing = true;
    }

    void EndDrawing() {
      commandBuffer.endRenderPass();

      vk::ImageSubresourceRange image_subresource_range(
        vk::ImageAspectFlagBits::eColor, // VkImageAspectFlags                     aspectMask
        0,                               // uint32_t                               baseMipLevel
        1,                               // uint32_t                               levelCount
        0,                               // uint32_t                               baseArrayLayer
        1                                // uint32_t                               layerCount
      );

      vk::ImageMemoryBarrier barrier_from_present_to_draw(
        vk::AccessFlagBits::eColorAttachmentWrite,              // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
        vk::AccessFlagBits::eMemoryRead,                        // VkAccessFlags            dstAccessMask
        vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            oldLayout
        vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            newLayout
        graphicsQueue.familyIndex,                              // uint32_t                 srcQueueFamilyIndex
        presentQueue.familyIndex,                               // uint32_t                 dstQueueFamilyIndex
        swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
        image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
      );

      commandBuffer.pipelineBarrier( 
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer, 
        vk::DependencyFlagBits(),
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier_from_present_to_draw);

      drawingContext.drawing = false;
    }

    void BeginFrame() {
      if( device.waitForFences( 1, &submitCompleteFence, VK_FALSE, 1000000000 ) != vk::Result::eSuccess ) {
        std::cout << "Waiting for fence takes too long!" << std::endl;
        Error();
      }

      device.resetFences( 1, &submitCompleteFence );

      vk::Result result = device.acquireNextImageKHR( swapChainInfo.swapChain, 
                                                      UINT64_MAX,
                                                      semaphores.imageAvailable,
                                                      vk::Fence(),
                                                      &swapChainInfo.acquiredImageIndex );
      switch( result ) {
          case vk::Result::eSuccess:
              break;
          case vk::Result::eSuboptimalKHR:
              break;
          case vk::Result::eErrorOutOfDateKHR:
              std::cout << "Swap chain out of date" << std::endl;
              RefreshSwapChain();
              return;
          default:
              std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
              Error();
      }

      vk::CommandBufferBeginInfo cmd_buffer_begin_info(
          vk::CommandBufferUsageFlagBits::eSimultaneousUse, // VkCommandBufferUsageFlags              flags
          nullptr                                           // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
      );

      commandBuffer.begin( &cmd_buffer_begin_info );

      vk::ImageSubresourceRange image_subresource_range(
        vk::ImageAspectFlagBits::eColor,                // VkImageAspectFlags                     aspectMask
        0,                                              // uint32_t                               baseMipLevel
        1,                                              // uint32_t                               levelCount
        0,                                              // uint32_t                               baseArrayLayer
        1                                               // uint32_t                               layerCount
      );

      // Transition to presentation layout and tell vulkan that we are discarding previous contents of the image
      // block reads from present(atachment output), blocks draws and clears
      vk::ImageMemoryBarrier barrier_from_present_to_draw(
        vk::AccessFlagBits(),                                   // VkAccessFlags            srcAccessMask
        vk::AccessFlagBits::eMemoryRead,                        // VkAccessFlags            dstAccessMask
        vk::ImageLayout::eUndefined,                            // VkImageLayout            oldLayout // TODO: DO AN INITIAL FILL
        vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            newLayout
        VK_QUEUE_FAMILY_IGNORED,                                // uint32_t                 srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                                // uint32_t                 dstQueueFamilyIndex
        swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
        image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
      );

      commandBuffer.pipelineBarrier( 
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlagBits(),
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier_from_present_to_draw);
    }

    void EndFrame() {
      if( drawingContext.drawing ) {
        EndDrawing();
      }

      if( commandBuffer.end() != vk::Result::eSuccess ) {
        std::cout << "Could not record command buffers!" << std::endl;
        Error();
      }

      vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                                   vk::PipelineStageFlagBits::eTransfer;
      vk::SubmitInfo submit_info(
        1,                                     // uint32_t                     waitSemaphoreCount
        &semaphores.imageAvailable,            // const VkSemaphore           *pWaitSemaphores
        &wait_dst_stage_mask,                  // const VkPipelineStageFlags  *pWaitDstStageMask;
        1,                                     // uint32_t                     commandBufferCount
        &commandBuffer,                        // const VkCommandBuffer       *pCommandBuffers
        1,                                     // uint32_t                     signalSemaphoreCount
        &semaphores.renderingFinished          // const VkSemaphore           *pSignalSemaphores
      );

      if( presentQueue.handle.submit( 1, &submit_info, submitCompleteFence ) != vk::Result::eSuccess ) {
        std::cout << "Submit to queue failed!" << std::endl;
        Error();
      }

      vk::PresentInfoKHR present_info(
        1,                                    // uint32_t                     waitSemaphoreCount
        &semaphores.renderingFinished,        // const VkSemaphore           *pWaitSemaphores
        1,                                    // uint32_t                     swapchainCount
        &swapChainInfo.swapChain,             // const VkSwapchainKHR        *pSwapchains
        &swapChainInfo.acquiredImageIndex,    // const uint32_t              *pImageIndices
        nullptr                               // VkResult                    *pResults
      );
      
      vk::Result result = presentQueue.handle.presentKHR( &present_info );

      switch( result ) {
        case vk::Result::eSuccess:
          break;
        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR:
          return RefreshSwapChain();
        default:
          std::cout << "Problem occurred during image presentation!" << std::endl;
          Error();
      }
    }

    void Clear(float r, float g, float b) {
      if( drawingContext.drawing ) {
        EndDrawing();
      }

      vk::ImageSubresourceRange image_subresource_range(
        vk::ImageAspectFlagBits::eColor,                // VkImageAspectFlags                     aspectMask
        0,                                              // uint32_t                               baseMipLevel
        1,                                              // uint32_t                               levelCount
        0,                                              // uint32_t                               baseArrayLayer
        1                                               // uint32_t                               layerCount
      );

      vk::ImageMemoryBarrier barrier_from_present_to_clear(
        vk::AccessFlagBits(),                       // VkAccessFlags                          srcAccessMask, eMemoryRead fails validation
        vk::AccessFlagBits::eTransferWrite,         // VkAccessFlags                          dstAccessMask
        vk::ImageLayout::ePresentSrcKHR,            // VkImageLayout                          oldLayout
        vk::ImageLayout::eTransferDstOptimal,       // VkImageLayout                          newLayout
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
        swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                                image
        image_subresource_range                     // VkImageSubresourceRange                subresourceRange
      );

      commandBuffer.pipelineBarrier( vk::PipelineStageFlagBits::eTransfer, 
                                     vk::PipelineStageFlagBits::eTransfer,
                                     vk::DependencyFlagBits(),
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &barrier_from_present_to_clear );

      const std::array<float, 4> color = { r, g, b, 1.0f };
      vk::ClearColorValue clear_color(
        color
      );

      commandBuffer.clearColorImage( swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                                     vk::ImageLayout::eTransferDstOptimal,
                                     &clear_color,
                                     1,
                                     &image_subresource_range );

      vk::ImageMemoryBarrier barrier_from_clear_to_present(
        vk::AccessFlagBits::eTransferWrite ,        // VkAccessFlags                          srcAccessMask, eMemoryRead fails validation
        vk::AccessFlagBits::eMemoryRead,            // VkAccessFlags                          dstAccessMask
        vk::ImageLayout::eTransferDstOptimal,       // VkImageLayout                          oldLayout
        vk::ImageLayout::ePresentSrcKHR,            // VkImageLayout                          newLayout
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
        swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                                image
        image_subresource_range                     // VkImageSubresourceRange                subresourceRange
      );

      commandBuffer.pipelineBarrier( vk::PipelineStageFlagBits::eTransfer, 
                                     vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
                                     vk::DependencyFlagBits(),
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &barrier_from_clear_to_present );

    }

    void DrawLine(float x1, float y1, float x2, float y2) {
      if( !drawingContext.drawing ) {
        BeginDrawing();
      }

      // Bind line graphics pipeline
      commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, linePipeline );

      // Transition image layout from generic read/present
      LinePushConstants pushConstants;
      pushConstants.positions[0] = x1;
      pushConstants.positions[1] = y1;
      pushConstants.positions[2] = x2;
      pushConstants.positions[3] = y2;
      std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));

      commandBuffer.pushConstants( linePipelineLayout,
                                   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                   0, // offset
                                   sizeof(LinePushConstants),
                                   &pushConstants );
      commandBuffer.setLineWidth( drawingContext.lineWidth );
      commandBuffer.draw( 2, 1, 0, 0 );
    }

    void DrawPoint(float x, float y) {
      if( !drawingContext.drawing ) {
        BeginDrawing();
      }

      // Bind line graphics pipeline
      commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, pointPipeline );

      // Transition image layout from generic read/present
      PointPushConstants pushConstants;
      pushConstants.positions[0] = x;
      pushConstants.positions[1] = y;
      //pushConstants.data[0] = x;
      //pushConstants.data[1] = y;
      //pushConstants.size = 1.0f;//drawingContext.pointSize;
      std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));
      // pushConstants.data[2] = drawingContext.color[0];
      // pushConstants.data[3] = drawingContext.color[1];
      // pushConstants.data[4] = drawingContext.color[2];
      // pushConstants.data[5] = drawingContext.color[3];

      //pushConstants.data[6] = drawingContext.pointSize;
      //std::cout << sizeof(PointPushConstants) << std::endl;

      commandBuffer.pushConstants( pointPipelineLayout,
                                   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                   0, // offset
                                   sizeof(PointPushConstants),
                                   &pushConstants );
      commandBuffer.draw( 1, 1, 0, 0 );
    }

    void SetColor(float r, float g, float b) {
      drawingContext.color[0] = r;
      drawingContext.color[1] = g;
      drawingContext.color[2] = b;
    }

    void SetLineWidth(float width) {
      drawingContext.lineWidth = width;
    }

    void SetPointSize(float size) {
      drawingContext.pointSize = size;
    }

    bool Update() {
      static float t;
      t += 0.01;
      bool running = true;
      PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);
      if (msg.message == WM_QUIT) {
        running = false;
      }
      else {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      //RedrawWindow(windowHandle, NULL, NULL, RDW_INTERNALPAINT);
      
      // POINT cursor;
      // GetCursorPos(&cursor);
      // ScreenToClient(windowHandle, &cursor);
      // float fcursor[2] = { 2.0f*((float)cursor.x)/swapChainInfo.extent.width - 1.0f, 
      //                      2.0f*((float)cursor.y)/swapChainInfo.extent.height - 1.0f };
      
      // std::cout << fcursor[0] << std::endl;

      // BeginFrame();
      // Clear(0.0f, 0.0f, 0.0f);
      // SetColor(0.0f, 1.0f, 0.0f);
      // DrawLine(fcursor[0], fcursor[1]-0.05f, fcursor[0], fcursor[1]+0.05f);
      // DrawLine(fcursor[0]-0.05f, fcursor[1], fcursor[0]+0.05f, fcursor[1]);
      // EndFrame();
      return running;
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
      float fcursor[2];

      // Don't process any messages if modal dialog is showing
      if (dialogShowing) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
      }

      // Hide mouse inside client area
      if (uMsg == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT)
      {
        SetCursor(NULL);
        return TRUE;
      }

      switch (uMsg) {
        case WM_MOUSEMOVE:
        POINT cursor;
        GetCursorPos(&cursor);
        ScreenToClient(windowHandle, &cursor);
        fcursor[0] = 2.0f*((float)cursor.x)/swapChainInfo.extent.width - 1.0f;
        fcursor[1] = 2.0f*((float)cursor.y)/swapChainInfo.extent.height - 1.0f;
        BeginFrame();
        Clear(0.1f, 0.1f, 0.1f);
        SetColor(0.8f, 0.8f, 0.8f);
        SetLineWidth(1.5f);
        //DrawLine(fcursor[0], fcursor[1]-0.05f, fcursor[0], fcursor[1]+0.05f);
        //Clear(0.1f, 0.1f, 0.1f);
        //DrawLine(fcursor[0]-0.05f, fcursor[1], fcursor[0]+0.05f, fcursor[1]);
        DrawPoint(fcursor[0], fcursor[1]);
        //Clear(0.1f, 0.1f, 0.1f);
        //Clear(fcursor[0], 0.0f, 0.0f);
        EndFrame();
        break;
      case WM_CLOSE:
        PostQuitMessage(0);
        break;
      case WM_PAINT:
        return FALSE;
        break;
      case WM_SIZE:
        RefreshSwapChain();
        return FALSE;
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
        windowClass.hCursor = NULL;
        RegisterClassEx(&windowClass);

        windowHandle = CreateWindowEx(NULL,
            "LsMainWindow",
            "Lustrious Paint",
            WS_OVERLAPPEDWINDOW, //| WS_VISIBLE,
            100,
            100,
            800,
            800,
            NULL,
            NULL,
            hInstance,
            NULL);
    }
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Ls::hInstance = hInstance;

    Ls::AttachConsole();
    Ls::CreateMainWindow();

    vk::LoadVulkanLibrary();
    vk::LoadExportedEntryPoints();
    vk::LoadGlobalLevelEntryPoints();
    Ls::CheckValidationAvailability();

    Ls::CreateInstance();
    vk::LoadInstanceLevelEntryPoints(Ls::instance, Ls::INSTANCE_EXTENSIONS);

    Ls::CreateDebugReportCallback(); // needs an instance level function
    Ls::CreatePresentationSurface(); // need this for device creation

    Ls::CreateDevice();
    vk::LoadDeviceLevelEntryPoints(Ls::device, Ls::DEVICE_EXTENSIONS);

    Ls::CreateSemaphores();
    Ls::CreateFence();
    Ls::GetQueues();
    Ls::CreateSwapChain();

    Ls::CreateRenderPass();
    Ls::CreateFramebuffers();
    Ls::CreatePipelineLayout();
    Ls::CreatePipeline();

    Ls::CreateCommandBuffers();

    ShowWindow(Ls::windowHandle, SW_SHOW);
    //ShowCursor(false);

    while (Ls::Update()) {};
    
    Ls::FreeDevice();
    Ls::FreeInstance();
    vk::UnloadVulkanLibrary();
    return Ls::msg.wParam;
}
