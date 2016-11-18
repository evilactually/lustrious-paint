#include <vulkan_dynamic.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#include <memory>
#include <destructor.h>

#include <LsRenderer.h>
#include <LsVulkanLoader.h>
#include <LsError.h>

using namespace lslib;

struct LinePushConstants {
  float positions[4];
  float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct PointPushConstants {
  float positions[2]; 
  float pad[2];
  float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  float size = 1.0f;
};

// Needs device, swapchain, graphics queue

LsRenderer::LsRenderer() {

}

LsRenderer::~LsRenderer() {

}

bool CheckExtensionAvailability( const char *extension_name, 
                                 std::vector<vk::ExtensionProperties> const& available_instance_extensions ) {
  for( size_t i = 0; i < available_instance_extensions.size(); ++i ) {
    if( std::string(available_instance_extensions[i].extensionName) == extension_name ) {
      return true;
    }
  }
  return false;
}

//-------------------------------------------------------------------------------
// @ CreateInstance()
//-------------------------------------------------------------------------------
// Create Vulkan instance with all required instance extensions
//-------------------------------------------------------------------------------
void CreateInstance( std::vector<const char*> const& extensions, 
                     std::vector<const char*> const& layers,
                     vk::Instance* instance ) {
  // Get available extensions
  uint32_t instance_extensions_count = 0;
  vk::Result result = vk::enumerateInstanceExtensionProperties(
    nullptr, 
    &instance_extensions_count,
    nullptr );
  if ( result != vk::Result::eSuccess || instance_extensions_count == 0 ) {
    throw std::string("Error occurred during instance extension enumeration!");
  }
  std::vector<vk::ExtensionProperties> available_instance_extensions( instance_extensions_count );
  result = vk::enumerateInstanceExtensionProperties(
    nullptr, 
    &instance_extensions_count,
    &available_instance_extensions[0] );
  if( result != vk::Result::eSuccess ) {
    throw std::string("Error occurred during instance extension enumeration!");
  }

  // Check that all required extensions are present
  for( size_t i = 0; i < extensions.size(); ++i ) {
    if( !CheckExtensionAvailability( extensions[i], available_instance_extensions ) ) {
      throw (std::string("Could not find instance extension named \"") + 
             std::string(extensions[i]) + 
             std::string("\"!"));
    }
  }

  vk::ApplicationInfo appliactionInfo(
    NULL,//LS_PRODUCT_NAME,
    NULL,//LS_VERSION,
    NULL,
    NULL,
    VK_API_VERSION_1_0 );

    vk::InstanceCreateInfo instanceCreateInfo(
      vk::InstanceCreateFlags(),
      &appliactionInfo,
      static_cast<uint32_t>(layers.size()),
      layers.data(),
      static_cast<uint32_t>(extensions.size()),
      extensions.data() );

    if ( createInstance( &instanceCreateInfo, nullptr, instance) != vk::Result::eSuccess ) {
      throw std::string( "Failed to create Vulkan instance!" );
    }
}

//-------------------------------------------------------------------------------
// @ requiredInstanceExtensions
//-------------------------------------------------------------------------------
// List of instance-level extension that will be enabled when creating an instance
//-------------------------------------------------------------------------------
std::vector<const char*> requiredInstanceExtensions = {
  VK_KHR_SURFACE_EXTENSION_NAME,
  VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef VULKAN_VALIDATION
  VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
};

//-------------------------------------------------------------------------------
// @ requiredDeviceExtensions
//-------------------------------------------------------------------------------
// List of device-level extension that will be enabled when creating a logical device
//-------------------------------------------------------------------------------
std::vector<const char*> requiredDeviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//-------------------------------------------------------------------------------
// @ requiredInstanceValidationLayers
//-------------------------------------------------------------------------------
// List of instance-level validation levels that will be enabled when creating an instance
//-------------------------------------------------------------------------------
std::vector<const char*> requiredInstanceValidationLayers = {
#ifdef VULKAN_VALIDATION
  "VK_LAYER_LUNARG_standard_validation"
#endif
};

void CheckValidationLayersAvailability(std::vector<const char*> const& layers) {
  uint32_t availableLayerCount;
  vk::enumerateInstanceLayerProperties(&availableLayerCount, nullptr);
  std::vector<vk::LayerProperties> availableLayers(availableLayerCount);
  vk::enumerateInstanceLayerProperties(&availableLayerCount, &availableLayers[0]);
  availableLayers.resize(availableLayerCount);

  for( size_t i = 0; i < layers.size(); ++i ) {
    bool found = false;
    for (size_t j = 0; j < availableLayers.size(); ++j)
    {
      if ( strcmp( layers[i], availableLayers[j].layerName ) == 0 ) {
        found = true;
        break;
      }
    }
    if ( !found )
    {
      throw std::string("Instance layer ") + std::string(layers[i]) + std::string(" not found!");
    }
  }
}

void CreateDebugReportCallback(vk::Instance instance, vk::DebugReportCallbackEXT* debugReportCallback) {
  vk::DebugReportCallbackCreateInfoEXT callbackCreateInfo(
    vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning,
    NULL,
    nullptr);
  instance.createDebugReportCallbackEXT(&callbackCreateInfo, nullptr, debugReportCallback);
}

void CreatePresentationSurface(vk::Instance instance, HINSTANCE hInstance, HWND windowHandle, vk::SurfaceKHR* presentationSurface) {
  vk::Win32SurfaceCreateInfoKHR surface_create_info(
    vk::Win32SurfaceCreateFlagsKHR(), // vk::Win32SurfaceCreateFlagsKHR   flags
    hInstance,                        // HINSTANCE                        hinstance
    windowHandle                      // HWND                             hwnd
  );

  if( instance.createWin32SurfaceKHR( &surface_create_info, nullptr, presentationSurface ) != vk::Result::eSuccess ) {
    throw std::string("Could not create presentation surface!");
  }
}

LsRenderer LsRenderer::renderer;

LsRenderer* LsRenderer::Get() {
  return &renderer;
}

//-------------------------------------------------------------------------------
// @ RatePhysicalDevice()
//-------------------------------------------------------------------------------
// Give a rating to vk::PhysicalDevice. Device with a higher rating is more favorable.
// Rating of zero or less means device is unsuitable.
//-------------------------------------------------------------------------------
int RatePhysicalDevice(vk::PhysicalDevice physicalDevice, void const* userData) {
  return 1;
}

typedef int (RatePhysicalDeviceFn)(vk::PhysicalDevice, void const*);

vk::PhysicalDevice FindPhysicalDevice(vk::Instance instance, RatePhysicalDeviceFn* ratePhysicalDeviceFn, void const* withUserData) {
  // Get physical device count
  uint32_t physicalDeviceCount = 0;
  vk::Result result = instance.enumeratePhysicalDevices( &physicalDeviceCount, NULL );
  if( result != vk::Result::eSuccess || physicalDeviceCount == 0 ) {
    throw std::string("Error occurred during physical devices enumeration!");
  }

  // Get a list of physical devices
  std::vector<vk::PhysicalDevice> physicalDevices( physicalDeviceCount );
  result = instance.enumeratePhysicalDevices( &physicalDeviceCount, physicalDevices.data() );
  if( result != vk::Result::eSuccess || physicalDeviceCount == 0 ) {
    throw std::string("Error occurred during physical devices enumeration!");
  }

  // Rate physical devices
  std::vector<std::tuple<vk::PhysicalDevice, int>> ratedPhysicalDevices;
  for( auto physicalDevice:physicalDevices ) {
    int rating = ratePhysicalDeviceFn(physicalDevice, withUserData);
    ratedPhysicalDevices.push_back(std::make_tuple(physicalDevice, rating));
  }

  // Sort in descending order
  std::sort( ratedPhysicalDevices.begin(), ratedPhysicalDevices.end(), [](auto x, auto y) {
    return std::get<1>(x) >= std::get<1>(y);
  });

  vk::PhysicalDevice topDevice = std::get<0>(ratedPhysicalDevices[0]);
  int topRating = std::get<1>(ratedPhysicalDevices[0]);
  
  // Check if at least one device has a positive non-zero rating
  if ( topRating <= 0 )
  {
    throw std::string("No suitable Vulkan device found!");
  }

  return topDevice;
}

void LsRenderer::Initialize(HINSTANCE hInstance, HWND window) {
  LsRenderer* renderer = LsRenderer::Get();
  
  LsLoadVulkanLibrary();
  renderer->vulkanDestructor = std::make_unique<destructor>(destructor([](){
    LsMessageBox( "~vulkan", "...");
    LsUnloadVulkanLibrary();
  }));

  LsLoadExportedEntryPoints();
  LsLoadGlobalLevelEntryPoints();

  CreateInstance(requiredInstanceExtensions, requiredInstanceValidationLayers, &renderer->instance);
  destructor& instanceDestructor = destructor([](){
    LsMessageBox( "~instance", "...");
    LsRenderer::Get()->instance.destroy(nullptr);
  }).attach_to(*renderer->vulkanDestructor);

  LsLoadInstanceLevelEntryPoints(renderer->instance, requiredInstanceExtensions);
  CheckValidationLayersAvailability(requiredInstanceValidationLayers);
#ifdef VULKAN_VALIDATION
  CreateDebugReportCallback(renderer->instance, &renderer->debugReportCallback);
  destructor([](){
    LsMessageBox( "~debug callback", "...");
    LsRenderer* renderer = LsRenderer::Get();
    renderer->instance.destroyDebugReportCallbackEXT(renderer->debugReportCallback, nullptr);
  }).attach_to(instanceDestructor);
#endif
  CreatePresentationSurface(renderer->instance, hInstance, window, &renderer->swapChainInfo.presentationSurface);
  destructor& presentationDestructor = destructor([](){
    LsMessageBox( "~presentation", "...");
    LsRenderer* renderer = LsRenderer::Get();
    renderer->instance.destroySurfaceKHR(renderer->swapChainInfo.presentationSurface, nullptr);
  }).attach_to(instanceDestructor);

  renderer->physicalDevice = FindPhysicalDevice(renderer->instance, RatePhysicalDevice, &renderer);
  // LsRenderer::renderer.device = device;
  // LsRenderer::renderer.swapChainInfo.swapChain = swapChain;
  // TODO: Prepare Vulkan as usual
}

void LsRenderer::BeginFrame() {
  // if( device.waitForFences( 1, &submitCompleteFence, VK_FALSE, 1000000000 ) != vk::Result::eSuccess ) {
  //   throw std::string("Waiting for fence takes too long!");
  // }

  // device.resetFences( 1, &submitCompleteFence );

  // vk::Result result = device.acquireNextImageKHR( swapChainInfo.swapChain, 
  //   UINT64_MAX,
  //   semaphores.imageAvailable,
  //   vk::Fence(),
  //   &swapChainInfo.acquiredImageIndex );

  // switch( result ) {
  //   case vk::Result::eSuccess:
  //   break;
  //   case vk::Result::eSuboptimalKHR:
  //   // It's still OK to use.
  //   break;
  //   case vk::Result::eErrorOutOfDateKHR:
  //   //RefreshSwapChain();
  //   //UpdatePixelDimensions();
  //   return;
  //   default:
  //   throw std::string("Problem occurred during swap chain image acquisition!");
  // }

  // vk::CommandBufferBeginInfo cmd_buffer_begin_info(
  //   vk::CommandBufferUsageFlagBits::eSimultaneousUse, // VkCommandBufferUsageFlags              flags
  //   nullptr                                           // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
  //   );

  // commandBuffer.begin( &cmd_buffer_begin_info );

  // vk::ImageSubresourceRange image_subresource_range(
  //   vk::ImageAspectFlagBits::eColor,                // VkImageAspectFlags                     aspectMask
  //   0,                                              // uint32_t                               baseMipLevel
  //   1,                                              // uint32_t                               levelCount
  //   0,                                              // uint32_t                               baseArrayLayer
  //   1                                               // uint32_t                               layerCount
  //   );

  // // Transition to presentation layout and tell vulkan that we are discarding previous contents of the image
  // // block reads from present(atachment output), blocks draws and clears
  // vk::ImageMemoryBarrier barrier_from_present_to_draw(
  //   vk::AccessFlagBits(),                                   // VkAccessFlags            srcAccessMask
  //   vk::AccessFlagBits::eMemoryRead,                        // VkAccessFlags            dstAccessMask
  //   vk::ImageLayout::eUndefined,                            // VkImageLayout            oldLayout // TODO: DO AN INITIAL FILL
  //   vk::ImageLayout::ePresentSrcKHR,                        // VkImageLayout            newLayout
  //   VK_QUEUE_FAMILY_IGNORED,                                // uint32_t                 srcQueueFamilyIndex
  //   VK_QUEUE_FAMILY_IGNORED,                                // uint32_t                 dstQueueFamilyIndex
  //   swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
  //   image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
  //   );

  // commandBuffer.pipelineBarrier( 
  //   vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
  //   vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer,
  //   vk::DependencyFlagBits(),
  //   0,
  //   nullptr,
  //   0,
  //   nullptr,
  //   1,
  //   &barrier_from_present_to_draw);
}