#include <windows.h>
#include "vk.hpp"
#include "version.h"
#include "LsUtility/LsOptional.h"
#include "win32_console.hpp"
#include "utility.h"
#include <map>
#include <cmath>
#include "assert.h"
#include "wt.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <FreeImage.h>

#include <chrono>
#include <thread>
using namespace std::chrono_literals;

#define PACKETDATA (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE 0
#include "wintab/PKTDEF.h"

#define GIF_RECORDING

// int32x2_t test_1() {
//   //int32x2_t xs;
//   // xs[0] = 0;
//   // xs[1] = 0;
//   // xs[2] = 0;
//   return xs;
// }

//void assert(bool flag, char *msg = "") {
//    if (!flag) {
//        OutputDebugStringA("ASSERT: ");
//        OutputDebugStringA(msg);
//        OutputDebugStringA("\n");
//        int *base = 0;
//        *base = 1;
//    }
//}

/* converts FIX32 to double */
#define FIX_DOUBLE(x)   ((double)(INT(x))+((double)FRAC(x)/65536))

#define M_PI_F static_cast<float>(M_PI)

std::ostream& operator<<(std::ostream& os, const AXIS& ax)
{
  os << "AXIS { axMin:" << ax.axMin <<
             ", axMax:" << ax.axMax << 
             ", axUnits: " << ax.axUnits << 
             ", axResolution: " << FIX_DOUBLE(ax.axResolution) << " }";
  return os;
}

namespace ls {
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

    vk::Fence submitCompleteFence; // protects command buffer from being reset too soon

    vk::CommandPool graphicsCommandPool; 
    vk::CommandBuffer commandBuffer;
    uint32_t commandBufferIndex;
    
    vk::RenderPass renderPass;
    std::vector<vk::Framebuffer> framebuffers;
    vk::Pipeline linePipeline;
    vk::Pipeline pointPipeline;
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

    HCTX tabletContext;

    struct {
      float position[2];
      float orientation[2] = { M_PI_F, M_PI_F/2.0f };
      float pressure;
    } penStatus;

    #define PRESSURE_SENSITIVITY 20.0f

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

    vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

    struct {
      vk::Fence captureCompleteFence;          // used to wait by host for copy to complete
      vk::Semaphore captureFinishedSemaphore;  // used to block presentation while copying

      vk::CommandBuffer captureFrameCmds;      // commands used to capture a frame

      struct {
        vk::Image image;
        vk::DeviceMemory memory;
        vk::DeviceSize size;
        vk::Extent2D extent;
        uint32_t rowPitch;
      } capturedFrameImage;                    // captured frame image buffer and information about it
            
      HANDLE hCaptureBufferFile;               // temporary file to store frames before processing
      HANDLE hSavingThread = NULL;             // handle to thread that is currently saving a frame to temporary file
      uint32_t captureFrameCount;              // running count of captured frames in a recording
      
      BOOL capturing = false;                  // flag indicating whether we are currently capturing
      std::chrono::high_resolution_clock::time_point startTime; // time when capture was started(used to estimate framerate)
    } captureInfo;

    BOOL ShowSaveAsDialog(LPTSTR fileName, DWORD fileNameLength, LPTSTR filter, LPTSTR defaultExtension = NULL) {
      fileName[0] = '\0';
      OPENFILENAME dialogInfo = {};
      dialogInfo.lStructSize = sizeof(OPENFILENAME);
      dialogInfo.hwndOwner = windowHandle;
      dialogInfo.hInstance = hInstance;
      dialogInfo.lpstrFilter = filter;
      dialogInfo.lpstrFile = fileName; 
      dialogInfo.nMaxFile = fileNameLength;
      dialogInfo.lpstrDefExt = defaultExtension;
      dialogInfo.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;
      return GetSaveFileName(&dialogInfo);
    }

    // Find a memory type in "memoryTypeBits" that includes all of "properties"
    int32_t FindProperties(uint32_t memoryTypeBits, vk::MemoryPropertyFlags properties)
    {
      for (int32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
      {
        if ((memoryTypeBits & (1 << i)) &&
          ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
          return i;
      }
      return -1;
    }

    void CmdImageBarrier( vk::CommandBuffer cmdBuffer,
                          vk::AccessFlags srcAccessMask,
                          vk::AccessFlags dstAccessMask,
                          vk::ImageLayout oldLayout,
                          vk::ImageLayout newLayout,
                          uint32_t srcQueueFamilyIndex,
                          uint32_t dstQueueFamilyIndex,
                          vk::Image image,
                          vk::ImageSubresourceRange subresourceRange,
                          vk::PipelineStageFlags srcStageMask,
                          vk::PipelineStageFlags dstStageMask,
                          vk::DependencyFlags dependencyFlags) {
      vk::ImageMemoryBarrier barrier(
        srcAccessMask,              // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
        dstAccessMask,              // VkAccessFlags            dstAccessMask
        oldLayout,                  // VkImageLayout            oldLayout
        newLayout,                  // VkImageLayout            newLayout
        srcQueueFamilyIndex,        // uint32_t                 srcQueueFamilyIndex
        dstQueueFamilyIndex,        // uint32_t                 dstQueueFamilyIndex
        image,                      // VkImage                  image
        subresourceRange            // VkImageSubresourceRange  subresourceRange
      );

      cmdBuffer.pipelineBarrier( 
        srcStageMask,
        dstStageMask,
        dependencyFlags,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    }

    void CreateCapturedFrameImage() {
      vk::ImageCreateInfo imageCreateInfo( vk::ImageCreateFlags(), 
                                           vk::ImageType::e2D,
                                           vk::Format::eR8G8B8A8Srgb, // can convert to any same-sized format
                                           vk::Extent3D(swapChainInfo.extent.width, swapChainInfo.extent.height, 1),
                                           1, // mipLevels
                                           1, // arrayLayers
                                           vk::SampleCountFlagBits::e1,
                                           vk::ImageTiling::eLinear,     // have to have linear for host access
                                           vk::ImageUsageFlagBits::eTransferDst,
                                           vk::SharingMode::eExclusive,  // not using from other queues
                                           0, 
                                           nullptr, 
                                           vk::ImageLayout::eUndefined );
      vk::MemoryRequirements memoryRequirements;
      if ( device.createImage( &imageCreateInfo, nullptr, &captureInfo.capturedFrameImage.image ) != vk::Result::eSuccess ) {
        std::cout << "Could not create image!" << std::endl;
        Error();
      }
      device.getImageMemoryRequirements(captureInfo.capturedFrameImage.image, &memoryRequirements);
      // memory requirements give required flags only, not exact type index, we have to find that
      int32_t memoryType = FindProperties( memoryRequirements.memoryTypeBits, 
                                           vk::MemoryPropertyFlagBits::eHostVisible |
                                           vk::MemoryPropertyFlagBits::eHostCoherent );
      vk::MemoryAllocateInfo memoryAllocationInfo( memoryRequirements.size,
                                                   memoryType ); // heap index is known implicitly from type index
      device.allocateMemory( &memoryAllocationInfo, nullptr, &captureInfo.capturedFrameImage.memory );
      device.bindImageMemory( captureInfo.capturedFrameImage.image, captureInfo.capturedFrameImage.memory, 0 );

      // frame memory size used later for writing to file, and later
      // it doubles as frame pitch for reading
      captureInfo.capturedFrameImage.size = memoryRequirements.size;
    }

    void RecordFrameCaptureCmds() {
      
      vk::CommandBufferBeginInfo cmd_buffer_begin_info(
          vk::CommandBufferUsageFlagBits::eSimultaneousUse, // VkCommandBufferUsageFlags              flags
          nullptr                                           // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
      );

      captureInfo.captureFrameCmds.begin( &cmd_buffer_begin_info );

      vk::ImageSubresourceRange imageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,                // VkImageAspectFlags                     aspectMask
        0,                                              // uint32_t                               baseMipLevel
        1,                                              // uint32_t                               levelCount
        0,                                              // uint32_t                               baseArrayLayer
        1                                               // uint32_t                               layerCount
      );

      // transition swap chain image into transfer source layout
      CmdImageBarrier( captureInfo.captureFrameCmds,
                       vk::AccessFlagBits::eMemoryRead,
                       vk::AccessFlagBits::eTransferRead,
                       vk::ImageLayout::ePresentSrcKHR,
                       vk::ImageLayout::eTransferSrcOptimal,
                       VK_QUEUE_FAMILY_IGNORED,
                       VK_QUEUE_FAMILY_IGNORED,
                       swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                       imageSubresourceRange,
                       vk::PipelineStageFlags(),             // wait for nothing
                       vk::PipelineStageFlagBits::eTransfer, // block transfer
                       vk::DependencyFlags() );

      // transition captureInfo.capturedFrameImage.image to transfer destination layout
      CmdImageBarrier( captureInfo.captureFrameCmds,
                       vk::AccessFlags(),
                       vk::AccessFlagBits::eTransferWrite,
                       vk::ImageLayout::eUndefined,
                       vk::ImageLayout::eTransferDstOptimal,
                       VK_QUEUE_FAMILY_IGNORED,
                       VK_QUEUE_FAMILY_IGNORED,
                       captureInfo.capturedFrameImage.image,
                       imageSubresourceRange,
                       vk::PipelineStageFlags(),             // wait for nothing
                       vk::PipelineStageFlagBits::eTransfer, // block transfer
                       vk::DependencyFlags() );
      
      vk::ImageSubresourceLayers subresourceLayers(
        vk::ImageAspectFlagBits::eColor,                // VkImageAspectFlags                     aspectMask
        0,                                              // uint32_t                               mipLevel
        0,                                              // uint32_t                               baseArrayLayer
        1                                               // uint32_t                               layerCount
      );

      vk::ImageCopy region = {
        subresourceLayers,
        {0,0,0},
        subresourceLayers,
        {0,0,0},
        {swapChainInfo.extent.width, swapChainInfo.extent.height, 1}
      };

      captureInfo.captureFrameCmds.copyImage( swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                                  vk::ImageLayout::eTransferSrcOptimal,
                                  captureInfo.capturedFrameImage.image,
                                  vk::ImageLayout::eTransferDstOptimal,
                                  1,
                                  &region );

      // transition captureInfo.capturedFrameImage.image to host read layout
      CmdImageBarrier( captureInfo.captureFrameCmds,
                       vk::AccessFlagBits::eTransferWrite,
                       vk::AccessFlagBits::eHostRead,        // image memory will be mapped for reading later
                       vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eGeneral,            // layout that supports host access
                       VK_QUEUE_FAMILY_IGNORED,
                       VK_QUEUE_FAMILY_IGNORED,
                       captureInfo.capturedFrameImage.image,
                       imageSubresourceRange,
                       vk::PipelineStageFlagBits::eTransfer, // wait for transfer
                       vk::PipelineStageFlagBits::eHost,     // guarantee visibility of writes to host
                       vk::DependencyFlags() );

      // transition swap chain image back to presentation layout
      CmdImageBarrier( captureInfo.captureFrameCmds,
                       vk::AccessFlagBits::eTransferRead,
                       vk::AccessFlagBits::eMemoryRead,
                       vk::ImageLayout::eTransferSrcOptimal,
                       vk::ImageLayout::ePresentSrcKHR,
                       VK_QUEUE_FAMILY_IGNORED,
                       VK_QUEUE_FAMILY_IGNORED,
                       swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                       imageSubresourceRange,
                       vk::PipelineStageFlagBits::eTransfer, // wait for transfer
                       vk::PipelineStageFlags(),             // block nothing, because nothing comes next
                       vk::DependencyFlags() );

      captureInfo.captureFrameCmds.end();
    }

    void OpenBufferFile() {
      TCHAR tmpPath[MAX_PATH];
      TCHAR tmpFilePath[MAX_PATH];
      DWORD result = GetTempPath(MAX_PATH, tmpPath);
      if ( (result > MAX_PATH) || (result == 0) ) {
        std::cout << "GetTempPath failed!" << std::endl;
        Error();
      }
      std::cout << "Using temporary path:" << tmpPath << std::endl;
      result = GetTempFileName( tmpPath, TEXT("ls"), 0, tmpFilePath );
      std::cout << "Buffer temporary file" << tmpFilePath << std::endl;
      if ( !result ) {
        std::cout << "GetTempFileName failed!" << std::endl;
        Error();
      }
      
      captureInfo.hCaptureBufferFile = CreateFile( tmpFilePath, 
                                                   GENERIC_WRITE | GENERIC_READ,
                                                   0,
                                                   NULL,
                                                   CREATE_ALWAYS, 
                                                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                                                   NULL );
      if (captureInfo.hCaptureBufferFile == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to create a file!" << std::endl;
        Error(); 
      }
    }

    BOOL CheckFrameSavingFinished() {
      if (captureInfo.hSavingThread) {
        DWORD result = WaitForSingleObject( captureInfo.hSavingThread,
                                            0 ); // no wait
        return !result; // it's 0 when finished
      } else {
        return true; // no thread was started
      }
    }

    DWORD WINAPI WriteFrameThread( LPVOID lpParam ) {
      void* data;
      DWORD written;
      device.mapMemory( captureInfo.capturedFrameImage.memory, 0, captureInfo.capturedFrameImage.size, vk::MemoryMapFlags(), &data );
      DWORD result = WriteFile( captureInfo.hCaptureBufferFile, data, captureInfo.capturedFrameImage.size, &written, nullptr );
      device.unmapMemory ( captureInfo.capturedFrameImage.memory ); 
      //TODO: What if write fails?
      return 0;
    }

    void BeginSavingCapturedFrame() {
      captureInfo.captureFrameCount++;
      captureInfo.hSavingThread = CreateThread( NULL,
                                    0,
                                    WriteFrameThread,
                                    NULL,
                                    0,
                                    NULL );
    }

    void StartCapturing() {
      std::cout << "Recording started" << std::endl;
      OpenBufferFile();
      CreateCapturedFrameImage();
      RecordFrameCaptureCmds();

      // frame row pitch is used for decoding frame memory
      vk::ImageSubresource imageSubresource = {
        vk::ImageAspectFlagBits::eColor,
        0,
        0
      };
      vk::SubresourceLayout subresourceLayout;
      device.getImageSubresourceLayout( captureInfo.capturedFrameImage.image,
                                        &imageSubresource,
                                        &subresourceLayout );
      captureInfo.capturedFrameImage.rowPitch = subresourceLayout.rowPitch;

      // capture frame extent
      captureInfo.capturedFrameImage.extent = swapChainInfo.extent;

      RecordFrameCaptureCmds();
      captureInfo.capturing = true;
      captureInfo.captureFrameCount = 0;
      captureInfo.startTime = std::chrono::high_resolution_clock::now();
    }

    void StopCapturing() {
      std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
      BYTE* captureData = (BYTE*)malloc(captureInfo.capturedFrameImage.size);
      DWORD bytesRead;

      std::cout << "Recording stopped" << std::endl;
      SetFilePointer( captureInfo.hCaptureBufferFile, // hFile
                      0,                              // lDistanceToMove
                      0,                              // lpDistanceToMoveHigh
                      FILE_BEGIN );                   // dwMoveMethod

      char fileName[1024];
      BOOL dialogResult = ShowSaveAsDialog(fileName, sizeof(fileName), "*.gif", "gif");

      if (!dialogResult) {
        free(captureData);
        CloseHandle(captureInfo.hCaptureBufferFile);
        captureInfo.capturing = false;
        return;
      }

      std::cout << "Saving to " << fileName << std::endl;

      FIMULTIBITMAP* multibitmap = FreeImage_OpenMultiBitmap( FIF_GIF,
                                                              fileName,
                                                              TRUE,
                                                              FALSE );

      if ( !multibitmap )
      {
        std::cout << "FreeImage_OpenMultiBitmap failed to create gif file!" << std::endl;
        Error();
      }

      double captureDuration = std::chrono::duration<double, std::milli>(endTime - captureInfo.startTime).count()/1000.0;
      double fps = 30.0f;//((double)captureInfo.captureFrameCount)/captureDuration;
      DWORD frameTimeMs = (DWORD)((1000.0 / fps) + 0.5);
      //DWORD frameTimeMs = (DWORD)( ((double)captureInfo.captureFrameCount)/captureDuration + 0.5);
      //std::cout << ((double)captureInfo.captureFrameCount)/captureDuration << std::endl;
      
      FITAG* tag = FreeImage_CreateTag();

      FreeImage_SetTagKey(tag, "FrameTime");
      FreeImage_SetTagType(tag, FIDT_LONG);
      FreeImage_SetTagCount(tag, 1);
      FreeImage_SetTagLength(tag, 4);
      FreeImage_SetTagValue(tag, &frameTimeMs);

      while ( captureInfo.captureFrameCount ) {
        // read next captured frame
        BOOL readResult = ReadFile( captureInfo.hCaptureBufferFile,
                                  captureData,
                                  captureInfo.capturedFrameImage.size,
                                  &bytesRead,
                                  nullptr );

        if ( !readResult || (bytesRead != captureInfo.capturedFrameImage.size) ) {
          std::cout << "ReadFile failed:" << GetLastError() << std::endl;
          Error();
        }

        FIBITMAP* bitmap = FreeImage_Allocate( captureInfo.capturedFrameImage.extent.width, 
                                               captureInfo.capturedFrameImage.extent.height,
                                               24 );
        RGBQUAD color;

        for (int y = 0; y < captureInfo.capturedFrameImage.extent.height; ++y)
        {
          for (int x = 0; x < captureInfo.capturedFrameImage.extent.width; ++x)
          {
            BYTE* pixel = captureData + captureInfo.capturedFrameImage.rowPitch*y + 4*x;
            color.rgbRed = *(pixel+2);
            color.rgbGreen = *(pixel+1);
            color.rgbBlue = *pixel;
            FreeImage_SetPixelColor(bitmap, x, captureInfo.capturedFrameImage.extent.height-y, &color);
          }   
        }

        FIBITMAP* quantized = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);

        // Reset animation metadata
        FreeImage_SetMetadata(FIMD_ANIMATION, quantized, NULL, NULL);
        FreeImage_SetMetadata(FIMD_ANIMATION, quantized, FreeImage_GetTagKey(tag), tag);

        //FreeImage_Save(FIF_PNG, bitmap, (std::to_string(captureInfo.captureFrameCount)+std::string(".png")).c_str(), 0);
        FreeImage_AppendPage(multibitmap, quantized);

        if (quantized)  {
          FreeImage_Unload(quantized);
        }
        FreeImage_Unload(bitmap);

        captureInfo.captureFrameCount--;
        //captureData += captureInfo.capturedFrameImage.size;
      }

      FreeImage_CloseMultiBitmap(multibitmap);

      free(captureData);
      //UnmapViewOfFile(captureData);
      //CloseHandle(hMapping);
      CloseHandle(captureInfo.hCaptureBufferFile);
      captureInfo.capturing = false;
    }

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

        vk::ApplicationInfo appliactionInfo(ls::Info::PRODUCT_NAME,
                                            ls::Info::VERSION,
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
                (queue_family_properties[i].queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer)) ) {
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

    void CheckPushConstantsLimits() {
      vk::PhysicalDeviceProperties deviceProps;
      physicalDevice.getProperties( &deviceProps );
      std::cout << "Push constants limit " << deviceProps.limits.maxPushConstantsSize << std::endl;
      assert(sizeof(PointPushConstants) <= deviceProps.limits.maxPushConstantsSize);
      assert(sizeof(LinePushConstants) <= deviceProps.limits.maxPushConstantsSize);
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

        if( selected_physical_device->createDevice( &device_create_info, nullptr, &ls::device ) != vk::Result::eSuccess ) {
            std::cout << "Could not create Vulkan device!" << std::endl;
            Error();
        }

        ls::graphicsQueue.familyIndex = selected_graphics_queue_family_index;
        ls::presentQueue.familyIndex = selected_present_queue_family_index;
        ls::physicalDevice = *selected_physical_device;

        physicalDevice.getMemoryProperties(&physicalDeviceMemoryProperties);

        CheckPushConstantsLimits();
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

        #ifdef GIF_RECORDING
        if (device.createSemaphore( &semaphore_create_info, nullptr, &captureInfo.captureFinishedSemaphore ) != vk::Result::eSuccess) {
            std::cout << "Could not create semaphores!" << std::endl;
            Error();
        }
        #endif
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

    LsOptional<vk::Extent2D> GetSwapChainExtent( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
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
          LsOptional<vk::Extent2D>::None();
        }
        // Most of the cases we define size of the swap_chain images equal to current window's size
        return surface_capabilities.currentExtent;
    }

    LsOptional<vk::ImageUsageFlags> GetSwapChainUsageFlags( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
      // Color attachment flag must always be supported, don't have to check
      // We can define other usage flags but we always need to check if they are supported
      vk::ImageUsageFlags additionalImageUsageFlags = vk::ImageUsageFlagBits::eTransferDst;
      #ifdef GIF_RECORDING
      additionalImageUsageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
      #endif
      if( surface_capabilities.supportedUsageFlags & additionalImageUsageFlags ) {
        return vk::ImageUsageFlagBits::eColorAttachment | additionalImageUsageFlags;
      }
      std::cout << "Image usage flags are not supported by the swap chain:" << vk::to_string(additionalImageUsageFlags) << std::endl;
      return LsOptional<vk::ImageUsageFlags>::None();
    }

    vk::SurfaceTransformFlagBitsKHR GetSwapChainTransform( vk::SurfaceCapabilitiesKHR &surface_capabilities ) {
      if( surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ) {
        return vk::SurfaceTransformFlagBitsKHR::eIdentity;
      } else {
        return surface_capabilities.currentTransform;
      }
    }

    LsOptional<vk::PresentModeKHR> GetSwapChainPresentMode( std::vector<vk::PresentModeKHR> &present_modes ) {
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
      return LsOptional<vk::PresentModeKHR>();
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
        LsOptional<vk::Extent2D>          desired_extent = GetSwapChainExtent( surface_capabilities );
        LsOptional<vk::ImageUsageFlags>   desired_usage = GetSwapChainUsageFlags( surface_capabilities );
        vk::SurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform( surface_capabilities );
        LsOptional<vk::PresentModeKHR>    desired_present_mode = GetSwapChainPresentMode( present_modes );
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

        canRender = true;
    }

    void GetSwapChainImages() {
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

        vk::CommandBufferAllocateInfo cmd_buffer_allocate_info(
          graphicsCommandPool,                               // VkCommandPool                commandPool
          vk::CommandBufferLevel::ePrimary,                  // VkCommandBufferLevel         level
          1);                                                // uint32_t                     bufferCount

        if( device.allocateCommandBuffers( &cmd_buffer_allocate_info, &commandBuffer ) != vk::Result::eSuccess ) {
            std::cout << "Could not allocate command buffer!" << std::endl;
            Error();
        }

        #ifdef GIF_RECORDING
        if( device.allocateCommandBuffers( &cmd_buffer_allocate_info, &captureInfo.captureFrameCmds ) != vk::Result::eSuccess ) {
            std::cout << "Could not allocate command buffer!" << std::endl;
            Error();
        }
        #endif
    }
    
    void FreeCommandBuffers() {
        if( ls::device ) {
            if ( commandBuffer ) {
              ls::device.waitIdle();
              device.freeCommandBuffers( graphicsCommandPool, 1, &commandBuffer);
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
        1,                                              // uint32_t                       pushConstantRangeCount
        &linePushConstantRange                          // const VkPushConstantRange      *pPushConstantRanges
      };

      vk::PipelineLayoutCreateInfo pointLayoutCreateInfo = {
        vk::PipelineLayoutCreateFlags(),                // VkPipelineLayoutCreateFlags    flags
        0,                                              // uint32_t                       setLayoutCount
        nullptr,                                        // const VkDescriptorSetLayout    *pSetLayouts
        1,                                              // uint32_t                       pushConstantRangeCount
        &pointPushConstantRange                         // const VkPushConstantRange      *pPushConstantRanges
      };

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
        ls::shaders["line.vert"] = ls::CreateShaderModule("shaders/line.vert.spv");
        ls::shaders["line.frag"] = ls::CreateShaderModule("shaders/line.frag.spv");
        ls::shaders["point.vert"] = ls::CreateShaderModule("shaders/point.vert.spv");
        ls::shaders["point.frag"] = ls::CreateShaderModule("shaders/point.frag.spv");

        std::vector<vk::PipelineShaderStageCreateInfo> line_shader_stage_create_infos = {
          // Vertex shader
          {
            vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
            vk::ShaderStageFlagBits::eVertex,                         // VkShaderStageFlagBits                          stage
            ls::shaders["line.vert"],                                 // VkShaderModule                                 module
            "main",                                                   // const char                                    *pName
            nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
          },
          // Fragment shader
          {
            vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
            vk::ShaderStageFlagBits::eFragment,                       // VkShaderStageFlagBits                          stage
            ls::shaders["line.frag"],                                 // VkShaderModule                                 module
            "main",                                                   // const char                                    *pName
            nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
          }
        };

        std::vector<vk::PipelineShaderStageCreateInfo> point_shader_stage_create_infos = {
          // Vertex shader
          {
            vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
            vk::ShaderStageFlagBits::eVertex,                         // VkShaderStageFlagBits                          stage
            ls::shaders["point.vert"],                                // VkShaderModule                                 module
            "main",                                                   // const char                                    *pName
            nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
          },
          // Fragment shader
          {
            vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
            vk::ShaderStageFlagBits::eFragment,                       // VkShaderStageFlagBits                          stage
            ls::shaders["point.frag"],                                // VkShaderModule                                 module
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
          static_cast<uint32_t>(point_shader_stage_create_infos.size()),// uint32_t                                       stageCount
          &point_shader_stage_create_infos[0],                          // const VkPipelineShaderStageCreateInfo         *pStages
          &vertex_input_state_create_info,                              // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
          &point_input_assembly_state_create_info,                      // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
          nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
          &viewport_state_create_info,                                  // const VkPipelineViewportStateCreateInfo       *pViewportState
          &rasterization_state_create_info,                             // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
          &multisample_state_create_info,                               // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
          nullptr,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
          &color_blend_state_create_info,                               // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
          nullptr,                                                      // const VkPipelineDynamicStateCreateInfo        *pDynamicState
          pointPipelineLayout,                                          // VkPipelineLayout                               layout
          renderPass,                                                   // VkRenderPass                                   renderPass
          0,                                                            // uint32_t                                       subpass
          vk::Pipeline(),                                               // VkPipeline                                     basePipelineHandle
          -1                                                            // int32_t                                        basePipelineIndex
        );

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

      #ifdef GIF_RECORDING
      fence_create_info.flags = vk::FenceCreateFlags();
      if( device.createFence( &fence_create_info, nullptr, &captureInfo.captureCompleteFence ) != vk::Result::eSuccess ) {
        std::cout << "Could not create a fence!" << std::endl;
        Error();
      }      
      #endif
    }

    void DestroyFence() {
      if ( submitCompleteFence ) {
        device.destroyFence( submitCompleteFence, nullptr );
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

    void DestroyShaderModules() {
      for(auto &shader: shaders) {
        device.destroyShaderModule(shader.second, nullptr);
      }
    }

    void DestroyPipeline() {
      if ( linePipeline )
      {
        device.destroyPipeline(linePipeline, nullptr);
      }

      if ( pointPipeline )
      {
        device.destroyPipeline(pointPipeline, nullptr);
      }

      DestroyShaderModules();
    }

    void DestroyImageViews() {
      swapChainInfo.imageViews.resize(swapChainInfo.images.size());
      for( auto &imageView: swapChainInfo.imageViews) {
        device.destroyImageView(imageView, nullptr);
      }
    }

    void DestroyPipelineLayout() {
      if ( linePipelineLayout ) {
        device.destroyPipelineLayout(linePipelineLayout, nullptr);
      }

      if ( pointPipelineLayout ) {
        device.destroyPipelineLayout(pointPipelineLayout, nullptr);
      }
    }

    void DestroyPresentationSurface() {
      if ( swapChainInfo.presentationSurface ) {
        instance.destroySurfaceKHR(swapChainInfo.presentationSurface, nullptr);
      }
    }

    void DestroySwapchain() {
      if ( swapChainInfo.swapChain )
      {
        device.destroySwapchainKHR(swapChainInfo.swapChain, nullptr);
      }
    }

    void DestroySemaphores() {
      if ( semaphores.renderingFinished ) {
        device.destroySemaphore(semaphores.renderingFinished, nullptr);
      }

      if ( semaphores.imageAvailable ) {
        device.destroySemaphore(semaphores.imageAvailable, nullptr);
      }
    }

    void DestroyDebugReportCallback() {
      if ( debugReportCallback ) {
        instance.destroyDebugReportCallbackEXT(debugReportCallback, nullptr);
      }
    }

    void RefreshSwapChain() {
      if ( device ) {
        FreeCommandBuffers();
        DestroyPipeline();
        DestroyFramebuffers();
        DestroyRenderpass();
        DestroyImageViews();

        CreateSwapChain(); // deletes old swap chain, because we need to specify what swap chain it replaces
        GetSwapChainImages();
        CreateSwapChainImageViews();
        CreateRenderPass();
        CreateFramebuffers();
        CreatePipeline();
        CreateCommandBuffers();

        // stop recording, because we can't put differently sized images into gif
        if(captureInfo.capturing) {
          StopCapturing();
        }

        std::cout << "Swap chain extent " << swapChainInfo.extent.width << "x" << swapChainInfo.extent.height << std::endl;
      }
    }
    struct {
      float width;
      float height;
    } pixelDimensions; // size of a pixel in vulkan coordinates

    void UpdatePixelDimensions() {
      pixelDimensions.width = (2.0f/(float)swapChainInfo.extent.width);
      pixelDimensions.height = (2.0f/(float)swapChainInfo.extent.height);
    }

    #define PIPELINE_BINDING_NONE 0x0
    #define PIPELINE_BINDING_LINE  0x1
    #define PIPELINE_BINDING_POINT 0x2

    struct {
      float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
      float lineWidth = 1.0f;
      float pointSize = 1.0f;
      bool drawing = false; // indicates that command buffer is ready to draw
      int pipelineBinding = 0;;
    } drawingContext;

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

      drawingContext.pipelineBinding = PIPELINE_BINDING_NONE;
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
        vk::PipelineStageFlagBits::eBottomOfPipe, // need to block presentation, there's no actual stage for it, //vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTransfer, 
        vk::DependencyFlagBits(),                 // but bottom of pipe is what we need
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
              UpdatePixelDimensions();
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
      BOOL captureInitiated = false;
      if( drawingContext.drawing ) {
        EndDrawing();
      }

      if( commandBuffer.end() != vk::Result::eSuccess ) {
        std::cout << "Could not record command buffers!" << std::endl;
        Error();
      }

      // stall these stages until image is available
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

      if( graphicsQueue.handle.submit( 1, &submit_info, submitCompleteFence ) != vk::Result::eSuccess ) {
        std::cout << "Submit to queue failed!" << std::endl;
        Error();
      }

      vk::Semaphore presentBlockingSemaphore = semaphores.renderingFinished;

      if (captureInfo.capturing) {
        // capture next frame only if previous capture already finished
        if (CheckFrameSavingFinished()) // && recordingOn
        {
          // stall transfer stage until rendering is finished 
          wait_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
          submit_info = {
            1,                                     // uint32_t                     waitSemaphoreCount
            &semaphores.renderingFinished,         // const VkSemaphore           *pWaitSemaphores
            &wait_dst_stage_mask,                  // const VkPipelineStageFlags  *pWaitDstStageMask;
            1,                                     // uint32_t                     commandBufferCount
            &captureInfo.captureFrameCmds,         // const VkCommandBuffer       *pCommandBuffers
            1,                                     // uint32_t                     signalSemaphoreCount
            &captureInfo.captureFinishedSemaphore  // const VkSemaphore           *pSignalSemaphores
          };
          device.resetFences( 1, &captureInfo.captureCompleteFence );
          if( graphicsQueue.handle.submit( 1, &submit_info, captureInfo.captureCompleteFence ) != vk::Result::eSuccess ) {
            std::cout << "Submit to queue failed!" << std::endl;
            Error();
          }
          // make present wait for capture instead of just render
          presentBlockingSemaphore = captureInfo.captureFinishedSemaphore;
          // resume presentation as soon as possible, using this flag to resume capture
          captureInitiated = true;
        } else {
          //std::cout << "Frame skipped, still writing!" << std::endl;
        }
      }

      vk::PresentInfoKHR present_info(
        1,                                    // uint32_t                     waitSemaphoreCount
        &presentBlockingSemaphore,            // const VkSemaphore           *pWaitSemaphores
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
          RefreshSwapChain();
          UpdatePixelDimensions();
          return;
        default:
          std::cout << "Problem occurred during image presentation!" << std::endl;
          Error();
      }

      if (captureInitiated) {
        // wait for capture submission to finish
        if( device.waitForFences( 1, &captureInfo.captureCompleteFence, VK_FALSE, 1000000000 ) != vk::Result::eSuccess ) {
          std::cout << "Waiting for fence takes too long!" << std::endl;
          Error();
        }
        // async copy frame to file
        BeginSavingCapturedFrame();
      }
    }

    void RepeatLastFrame() {
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
              UpdatePixelDimensions();
              return;
          default:
              std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
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
      
      result = presentQueue.handle.presentKHR( &present_info );

      switch( result ) {
        case vk::Result::eSuccess:
          break;
        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR:
          RefreshSwapChain();
          UpdatePixelDimensions();
          return;
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

      if(drawingContext.pipelineBinding != PIPELINE_BINDING_LINE) {
        // Bind line graphics pipeline
        commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, linePipeline );
        drawingContext.pipelineBinding = PIPELINE_BINDING_LINE; 
      }

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

      if(drawingContext.pipelineBinding != PIPELINE_BINDING_POINT) {
        // Bind line graphics pipeline
        commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, pointPipeline );
        drawingContext.pipelineBinding = PIPELINE_BINDING_POINT;
      }

      // Transition image layout from generic read/present
      // TODO: Do I need to push it every time?
      PointPushConstants pushConstants;
      pushConstants.positions[0] = x;
      pushConstants.positions[1] = y;
      pushConstants.size = drawingContext.pointSize;
      std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));

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

    struct {
      int frameCounter = 0;
      double fpsTimer = 0.0f;
      int lastFPS = 0;
      const double fpsUpdatePeriod = 100.0f;
      std::chrono::high_resolution_clock::time_point tStart;
      std::chrono::high_resolution_clock::time_point tEnd;
      double tDiff;
    } profilingInfo;

    void BeginProfiling() {
      profilingInfo.tStart = std::chrono::high_resolution_clock::now();
    }

    bool EndProfiling() {
      profilingInfo.frameCounter++;
      profilingInfo.tEnd = std::chrono::high_resolution_clock::now();
      profilingInfo.tDiff = std::chrono::duration<double, std::milli>(profilingInfo.tEnd - profilingInfo.tStart).count();

      profilingInfo.fpsTimer += (double)profilingInfo.tDiff;
      if (profilingInfo.fpsTimer > profilingInfo.fpsUpdatePeriod)
      {
        profilingInfo.lastFPS = profilingInfo.frameCounter*(1000.0f/profilingInfo.fpsUpdatePeriod);
        profilingInfo.fpsTimer = 0.0f;
        profilingInfo.frameCounter = 0;
        // indicate to caller that FPS counter updated
        return true;
      }
      // no update
      return false;
    }

    void RenderPointGrid() {
      const float cell_size = 32.0f;
      const int horizontal_cell_count = static_cast<int>(ceil(((float)swapChainInfo.extent.width)/cell_size));
      const int vertical_cell_count = static_cast<int>(ceil(((float)swapChainInfo.extent.height)/cell_size));
      SetPointSize(2.0f);
      SetColor(0.2f, 0.2f, 0.2f);
      for( int y = 0; y < vertical_cell_count; ++y ) {
        float y_offset = y*cell_size;
        for( int x = 0; x < horizontal_cell_count; ++x ) {
          float x_offset = x*cell_size;
          DrawPoint(pixelDimensions.width*x_offset - 1.0f, pixelDimensions.height*y_offset - 1.0f);
        }
      }
    }

    void RenderPointerFrame() {
      const float half_width = 16.0f;
      const float half_height = 16.0f;
      POINT cursor;
      float fcursor[2];
      GetCursorPos(&cursor);
      ScreenToClient(windowHandle, &cursor);
      fcursor[0] = pixelDimensions.width*cursor.x - 1.0f;
      fcursor[1] = pixelDimensions.height*cursor.y - 1.0f;
      float fhalf_width = pixelDimensions.width*half_width;
      float fhalf_height = pixelDimensions.height*half_height;
      SetColor(1.0f, 0.0f, 0.0f);
      DrawLine(fcursor[0], fcursor[1], fcursor[0], fcursor[1] + fhalf_height);
      SetColor(0.0f, 1.0f, 0.0f);
      DrawLine(fcursor[0], fcursor[1], fcursor[0] + fhalf_width, fcursor[1]);
    }

    glm::tmat2x2<float> tmat2x2_rotation(float angle) {
      return { cos(angle), sin(angle), 
              -sin(angle), cos(angle) };
    }

    void RenderBrushFrame() {
      glm::tmat2x2<float> rot;
      glm::vec2 pos(0.0f, 1.0f);
      // glm::rotate(pos, 0.1f, glm::vec2(1.0f, 0.0f));
      glm::tmat2x2<float> to_pixels = { pixelDimensions.width, 0.0f, 
                                        0.0f, pixelDimensions.height };

      glm::tmat2x2<float> extend = { 32.0f, 0.0f, 
                                     0.0f, 32.0f };

      auto azimuth = tmat2x2_rotation(penStatus.orientation[0])*glm::vec2(0.0f, -32.0f*penStatus.pressure);
      azimuth = to_pixels*azimuth;
      
      const float half_width = 32.0f;
      const float half_height = 32.0f;
      float fcursor[2];
      fcursor[0] = pixelDimensions.width*penStatus.position[0] - 1.0f;
      fcursor[1] = pixelDimensions.height*penStatus.position[1] - 1.0f;
      float fhalf_width = pixelDimensions.width*half_width;
      float fhalf_height = pixelDimensions.height*half_height;
      // SetColor(1.0f, 0.0f, 0.0f);
      // DrawLine(fcursor[0], fcursor[1], fcursor[0], fcursor[1] + fhalf_height);
      // SetColor(0.0f, 1.0f, 0.0f);
      // DrawLine(fcursor[0], fcursor[1], fcursor[0] + fhalf_width, fcursor[1]);
      //SetColor(1.0f, 1.0f, 1.0f);
      //DrawLine(fcursor[0], fcursor[1], fcursor[0] + azimuth[0], fcursor[1] + azimuth[1]);

      float length = 64.0f;
      glm::vec4 axis_x(1.0f, 0.0f, 0.0f, 1.0f);
      glm::vec4 axis_y(0.0f, 1.0f, 0.0f, 1.0f);
      glm::vec4 axis_z(0.0f, 0.0f, 1.0f, 1.0f);

      // apply altitude by rotation around x
      auto rotX = glm::rotate(glm::tmat4x4<float>(1.0f), -(penStatus.orientation[1] - M_PI_F/2.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      axis_x = rotX*axis_x;
      axis_y = rotX*axis_y;
      axis_z = rotX*axis_z;
      // apply azimuth by rotation around z
      auto rotZ = glm::rotate(glm::tmat4x4<float>(1.0f), penStatus.orientation[0] + M_PI_F, glm::vec3(0.0f, 0.0f, 1.0f));
      axis_x = rotZ*axis_x;
      axis_y = rotZ*axis_y;
      axis_z = rotZ*axis_z;
      // extend to length
      axis_x *= length;
      axis_y *= length;
      axis_z *= length;
      // transform into screen coordinates
      auto normalize = glm::scale(glm::tmat4x4<float>(1.0f), glm::vec3(pixelDimensions.width, pixelDimensions.height, 1.0f));
      axis_x = normalize*axis_x;
      axis_y = normalize*axis_y;
      axis_z = normalize*axis_z;
      SetLineWidth(1.0f);
      SetColor(1.0f, 0.0f, 0.0f);
      DrawLine(fcursor[0], fcursor[1], fcursor[0] + axis_x[0], fcursor[1] + axis_x[1]);

      SetColor(0.0f, 1.0f, 0.0f);
      DrawLine(fcursor[0], fcursor[1], fcursor[0] + axis_y[0], fcursor[1] + axis_y[1]);

      SetColor(0.0f, 0.0f, 1.0f);
      DrawLine(fcursor[0], fcursor[1], fcursor[0] + axis_z[0], fcursor[1] + axis_z[1]);

      axis_z *= penStatus.pressure;
      SetColor(1.0f, 1.0f, 1.0f);
      DrawLine(fcursor[0], fcursor[1], fcursor[0] + axis_z[0], fcursor[1] + axis_z[1]);
    }

    void Render() {
      BeginFrame();
      Clear(0.1f, 0.1f, 0.1f);
      RenderPointGrid();
      //RenderPointerFrame();
      RenderBrushFrame();
      EndFrame();
    }

    bool needRender = false;
    bool canSleep = true;
    
    // 1. Run update code
    // 2. Render if update code set needRender flag
    // 3. Sleep if update code set canSleep flag
    bool Run() {
      BeginProfiling();
      needRender = false;
      while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
        if ( !GetMessage( &msg, NULL, 0, 0 ) ) {
          return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      if (needRender) {
        Render();
        // If EndProfiling is not called for corresponding BeginProfiling, the frame is not counted
        if(EndProfiling()) {
          SetWindowText(windowHandle, (std::string("Lustrious Paint - ") + std::to_string(profilingInfo.lastFPS)).c_str());
        }
      }
      if (canSleep) {
        WaitMessage();
      }
      return true;
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
      PACKET pkt;
      POINT cursor;
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
      static int n;
      switch (uMsg) {
        case WM_KEYDOWN:
          if ( (lParam & 0x40000000) == 0 && wParam == VK_F12 ) {
            if (captureInfo.capturing) {
              StopCapturing();
            } else {
              StartCapturing();
            }
          }
          break;
        case WM_CREATE:
          needRender = true;
          break;
        case WM_MOUSEMOVE:
        needRender = true;
        GetCursorPos(&cursor);
        ScreenToClient(windowHandle, &cursor);
        penStatus.position[0] = cursor.x;
        penStatus.position[1] = cursor.y;
        break;
      case WM_CLOSE:
        PostQuitMessage(0);
        break;
      case WM_PAINT: // WM_PAINT is not removed by GetMessage from queue!
        needRender = true;
        //   return TRUE;
        break;
      case WT_PACKET:
        if (WTPacket((HCTX)lParam, wParam, &pkt)) 
        {
          needRender = true;
          AXIS pressureNormal;
          WTInfoA(WTI_DEVICES|0, DVC_NPRESSURE, &pressureNormal);
          penStatus.pressure = static_cast<float>(pkt.pkNormalPressure)/static_cast<float>(pressureNormal.axMax);
          penStatus.pressure *= PRESSURE_SENSITIVITY;
          penStatus.orientation[0] = (pkt.pkOrientation.orAzimuth/10)*(M_PI/180.0f);
          penStatus.orientation[1] = (pkt.pkOrientation.orAltitude/10)*(M_PI/180.0f);
          penStatus.orientation[1] = std::abs(penStatus.orientation[1]); // eraser on wacom gives negative altitude
          // GetCursorPos(&cursor);
          // ScreenToClient(windowHandle, &cursor);
          // penStatus.position[0] = cursor.x;
          // penStatus.position[1] = cursor.y;
          //std::cout << penStatus.position[0] << std::endl;
        }
        return FALSE;
        break;
      case WM_SIZE:
        // Message pump to loop while resizing, no rendering will be performed
        RefreshSwapChain();
        UpdatePixelDimensions();
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
                                    400,
                                    100,
                                    1024+16,
                                    640+38,
                                    NULL,
                                    NULL,
                                    hInstance,
                                    NULL);
  }

  void WintabReport() {
    TCHAR wintabid[32];
    WORD specVersion;
    WORD implVersion;
    UINT nDevices;
    UINT nCursors;
    UINT nContexts;
    UINT ctxOptions;
    UINT ctxSaveSize;
    UINT nExtensions;
    UINT nManagers;

    WTInfoA(WTI_INTERFACE , IFC_WINTABID, &wintabid);
    WTInfoA(WTI_INTERFACE , IFC_SPECVERSION, &specVersion);
    WTInfoA(WTI_INTERFACE , IFC_IMPLVERSION, &implVersion);
    WTInfoA(WTI_INTERFACE , IFC_NDEVICES, &nDevices);
    WTInfoA(WTI_INTERFACE , IFC_NCURSORS, &nCursors);
    WTInfoA(WTI_INTERFACE , IFC_NCONTEXTS, &nContexts);
    WTInfoA(WTI_INTERFACE , IFC_CTXOPTIONS, &ctxOptions);
    WTInfoA(WTI_INTERFACE , IFC_CTXSAVESIZE, &ctxSaveSize);
    WTInfoA(WTI_INTERFACE , IFC_NEXTENSIONS, &nExtensions);
    WTInfoA(WTI_INTERFACE , IFC_NMANAGERS, &nManagers);
    
    std::cout << " ------------------------------------ " << std::endl;
    std::cout << "             Wintab Report            " << std::endl;
    std::cout << " ------------------------------------ " << std::endl << std::endl;
    std::cout << "WTI_INTERFACE:" << std::endl;
    std::cout << "  IFC_WINTABID:" << wintabid << std::endl;
    std::cout << "  IFC_SPECVERSION:" << specVersion << std::endl;
    std::cout << "  IFC_IMPLVERSION:" << implVersion << std::endl;
    std::cout << "  IFC_NDEVICES:" << nDevices << std::endl;
    std::cout << "  IFC_NCURSORS:" << nCursors << std::endl;
    std::cout << "  IFC_NCONTEXTS:" << nContexts << std::endl;
    std::cout << "  IFC_CTXOPTIONS:" << ctxOptions << std::endl;
    std::cout << "  IFC_CTXSAVESIZE:" << ctxSaveSize << std::endl;
    std::cout << "  IFC_NEXTENSIONS:" << nExtensions << std::endl;
    std::cout << "  IFC_NMANAGERS:" << nManagers << std::endl;

    TCHAR wtDvcName[64];
    AXIS pressureNormal;
    AXIS pressureTangent;
    AXIS orientation[3];
    AXIS dvcX;
    AXIS dvcY;
    
    for(int i = 0; i < static_cast<int>(nDevices); ++i) {
      WTInfoA(WTI_DEVICES|i, DVC_NAME, &wtDvcName);
      WTInfoA(WTI_DEVICES|i, DVC_NPRESSURE, &pressureNormal);
      WTInfoA(WTI_DEVICES|i, DVC_TPRESSURE, &pressureTangent);
      WTInfoA(WTI_DEVICES|i, DVC_ORIENTATION, &orientation);
      WTInfoA(WTI_DEVICES|i, DVC_X, &dvcX);
      WTInfoA(WTI_DEVICES|i, DVC_Y, &dvcY);

      std::cout << "WTI_DEVICES[" << i << "]" << std::endl;
      std::cout << "  DVC_NAME:" << wtDvcName << std::endl;
      std::cout << "  DVC_NPRESSURE:" << pressureNormal << std::endl;
      std::cout << "  DVC_TPRESSURE:" << pressureTangent << std::endl;
      std::cout << "  DVC_ORIENTATION[0]:" << orientation[0] << std::endl;
      std::cout << "  DVC_ORIENTATION[1]:" << orientation[1] << std::endl;
      std::cout << "  DVC_ORIENTATION[2]:" << orientation[2] << std::endl;
      std::cout << "  DVC_X:" << dvcX << std::endl;
      std::cout << "  DVC_Y:" << dvcY << std::endl;
    }

    TCHAR extName[32];
    for(int i = 0; i < static_cast<int>(nExtensions); ++i) {
      WTInfoA(WTI_EXTENSIONS|i, EXT_NAME, &extName);

      std::cout << "WTI_EXTENSIONS[" << i << "]" << std::endl;
      std::cout << "  EXT_NAME:" << extName << std::endl;
    }

    TCHAR defCtxName[32];
    UINT defCtxOptions;
    UINT defCtxStatus;
    UINT defCtxLocks;
    UINT defCtxMsgBase;
    UINT defCtxDevice;
    UINT defCtxPktRate;
    WTPKT defCtxPktData;
    WTPKT defCtxPktMode;
    WTPKT defCtxMoveMask;
    DWORD defCtxBtnDnMask;
    DWORD defCtxBtnUpMask;
    LONG defCtxInOrgX;
    LONG defCtxInOrgY;
    LONG defCtxInOrgZ;
    LONG defCtxInExtX;
    LONG defCtxInExtY;
    LONG defCtxInExtZ;
    LONG defCtxOutOrgX;
    LONG defCtxOutOrgY;
    LONG defCtxOutOrgZ;
    LONG defCtxOutExtX;
    LONG defCtxOutExtY;
    LONG defCtxOutExtZ;
    FIX32 defCtxSensX;
    FIX32 defCtxSensY;
    FIX32 defCtxSensZ;
    BOOL defCtxSysMode;
    int defCtxSysOrgX;
    int defCtxSysOrgY;
    int defCtxSysExtX;
    int defCtxSysExtY;
    FIX32 defCtxSysSensX;
    FIX32 defCtxSysSensY;

    WTInfoA(WTI_DEFSYSCTX, CTX_NAME, &defCtxName);
    WTInfoA(WTI_DEFSYSCTX, CTX_OPTIONS, &defCtxOptions);
    WTInfoA(WTI_DEFSYSCTX, CTX_STATUS, &defCtxStatus);
    WTInfoA(WTI_DEFSYSCTX, CTX_LOCKS, &defCtxLocks);
    WTInfoA(WTI_DEFSYSCTX, CTX_MSGBASE, &defCtxMsgBase);
    WTInfoA(WTI_DEFSYSCTX, CTX_DEVICE, &defCtxDevice);
    WTInfoA(WTI_DEFSYSCTX, CTX_PKTRATE, &defCtxPktRate);
    WTInfoA(WTI_DEFSYSCTX, CTX_PKTDATA, &defCtxPktData);
    WTInfoA(WTI_DEFSYSCTX, CTX_PKTMODE, &defCtxPktMode);
    WTInfoA(WTI_DEFSYSCTX, CTX_MOVEMASK, &defCtxMoveMask);
    WTInfoA(WTI_DEFSYSCTX, CTX_BTNDNMASK, &defCtxBtnDnMask);
    WTInfoA(WTI_DEFSYSCTX, CTX_BTNUPMASK, &defCtxBtnUpMask);
    WTInfoA(WTI_DEFSYSCTX, CTX_INORGX, &defCtxInOrgX);
    WTInfoA(WTI_DEFSYSCTX, CTX_INORGY, &defCtxInOrgY);
    WTInfoA(WTI_DEFSYSCTX, CTX_INORGZ, &defCtxInOrgZ);
    WTInfoA(WTI_DEFSYSCTX, CTX_INEXTX, &defCtxInExtX);
    WTInfoA(WTI_DEFSYSCTX, CTX_INEXTY, &defCtxInExtY);
    WTInfoA(WTI_DEFSYSCTX, CTX_INEXTZ, &defCtxInExtZ);
    WTInfoA(WTI_DEFSYSCTX, CTX_OUTORGX, &defCtxOutOrgX);
    WTInfoA(WTI_DEFSYSCTX, CTX_OUTORGY, &defCtxOutOrgY);
    WTInfoA(WTI_DEFSYSCTX, CTX_OUTORGZ, &defCtxOutOrgZ);
    WTInfoA(WTI_DEFSYSCTX, CTX_OUTEXTX, &defCtxOutExtX);
    WTInfoA(WTI_DEFSYSCTX, CTX_OUTEXTY, &defCtxOutExtY);
    WTInfoA(WTI_DEFSYSCTX, CTX_OUTEXTZ, &defCtxOutExtZ);
    WTInfoA(WTI_DEFSYSCTX, CTX_SENSX, &defCtxSensX);
    WTInfoA(WTI_DEFSYSCTX, CTX_SENSY, &defCtxSensY);
    WTInfoA(WTI_DEFSYSCTX, CTX_SENSZ, &defCtxSensZ);
    WTInfoA(WTI_DEFSYSCTX, CTX_SYSMODE, &defCtxSysMode);
    WTInfoA(WTI_DEFSYSCTX, CTX_SYSORGX, &defCtxSysOrgX);
    WTInfoA(WTI_DEFSYSCTX, CTX_SYSORGY, &defCtxSysOrgY);
    WTInfoA(WTI_DEFSYSCTX, CTX_SYSEXTX, &defCtxSysExtX);
    WTInfoA(WTI_DEFSYSCTX, CTX_SYSEXTY, &defCtxSysExtY);
    WTInfoA(WTI_DEFSYSCTX, CTX_SYSSENSX, &defCtxSysSensX);
    WTInfoA(WTI_DEFSYSCTX, CTX_SYSSENSY, &defCtxSysSensY);

    std::cout << "WTI_DEFSYSCTX:" << std::endl;
    std::cout << "  CTX_NAME:" << defCtxName << std::endl;
    std::cout << "  CTX_OPTIONS:" << std::hex << "0x" << defCtxOptions << std::dec << std::endl;
    std::cout << "  CTX_STATUS:" << defCtxStatus << std::endl;
    std::cout << "  CTX_LOCKS:" << defCtxLocks << std::endl;
    std::cout << "  CTX_MSGBASE:" << defCtxMsgBase << std::endl;
    std::cout << "  CTX_DEVICE:" << (int)defCtxDevice << std::endl;
    std::cout << "  CTX_PKTRATE:" << defCtxPktRate << std::endl;
    std::cout << "  CTX_PKTDATA:" << std::hex << "0x" << defCtxPktData << std::dec << std::endl;
    std::cout << "  CTX_PKTMODE:" << std::hex << "0x" << defCtxPktMode << std::dec << std::endl;
    std::cout << "  CTX_MOVEMASK:" << std::hex << "0x" << defCtxMoveMask << std::dec << std::endl;
    std::cout << "  CTX_BTNDNMASK:" << std::hex << "0x" << defCtxBtnDnMask << std::dec << std::endl;
    std::cout << "  CTX_BTNUPMASK:" << std::hex << "0x" << defCtxBtnUpMask << std::dec << std::endl;
    std::cout << "  CTX_INORGX:" << defCtxInOrgX << std::endl;
    std::cout << "  CTX_INORGY:" << defCtxInOrgY << std::endl;
    std::cout << "  CTX_INORGZ:" << defCtxInOrgZ << std::endl;
    std::cout << "  CTX_INEXTX:" << defCtxInExtX << std::endl;
    std::cout << "  CTX_INEXTY:" << defCtxInExtY << std::endl;
    std::cout << "  CTX_INEXTZ:" << defCtxInExtZ << std::endl;
    std::cout << "  CTX_OUTORGX:" << defCtxOutOrgX << std::endl;
    std::cout << "  CTX_OUTORGY:" << defCtxOutOrgY << std::endl;
    std::cout << "  CTX_OUTORGZ:" << defCtxOutOrgZ << std::endl;
    std::cout << "  CTX_OUTEXTX:" << defCtxOutExtX << std::endl;
    std::cout << "  CTX_OUTEXTY:" << defCtxOutExtY << std::endl;
    std::cout << "  CTX_OUTEXTZ:" << defCtxOutExtZ << std::endl;
    std::cout << "  CTX_SENSX:" << FIX_DOUBLE(defCtxSensX) << std::endl;
    std::cout << "  CTX_SENSY:" << FIX_DOUBLE(defCtxSensY) << std::endl;
    std::cout << "  CTX_SENSZ:" << FIX_DOUBLE(defCtxSensZ) << std::endl;
    std::cout << "  CTX_SYSMODE:" << defCtxSysMode << std::endl;
    std::cout << "  CTX_SYSORGX:" << defCtxSysOrgX << std::endl;
    std::cout << "  CTX_SYSORGY:" << defCtxSysOrgY << std::endl;
    std::cout << "  CTX_SYSEXTX:" << defCtxSysExtX << std::endl;
    std::cout << "  CTX_SYSEXTY:" << defCtxSysExtY << std::endl;
    std::cout << "  CTX_SYSSENSX:" << FIX_DOUBLE(defCtxSysSensX) << std::endl;
    std::cout << "  CTX_SYSSENSY:" << FIX_DOUBLE(defCtxSysSensY) << std::endl;
    std::cout << std::endl << " ------------------------------------ " << std::endl;
  }

  /* ------------------------------------------------------------------------- */
  void TabletInit()
  {
    AXIS            dvcX, dvcY; // The maximum tablet size
    LOGCONTEXT      lcMine;     // The context of the tablet

    WTInfoA(WTI_DEFSYSCTX, 0, &lcMine); // get system logical context

    /* modify the digitizing region */
    wsprintf(lcMine.lcName, "Lustrious Paint Digitizing %x", hInstance);
    lcMine.lcOptions |= CXO_MESSAGES; // recieve WM_PACKET
    lcMine.lcOptions |= CXO_SYSTEM;   // receive WM_MOUSEMOVE
    lcMine.lcPktData = PACKETDATA;
    lcMine.lcPktMode = PACKETMODE;
    lcMine.lcMoveMask = PACKETDATA;
    lcMine.lcBtnUpMask = lcMine.lcBtnDnMask;

    /* Set the entire tablet as active */
    WTInfoA(WTI_DEVICES,DVC_X, &dvcX);
    WTInfoA(WTI_DEVICES,DVC_Y, &dvcY);
    lcMine.lcInOrgX = 0;
    lcMine.lcInOrgY = 0;
    lcMine.lcInExtX = dvcX.axMax;
    lcMine.lcInExtY = dvcY.axMax;

    tabletContext = WTOpenA(windowHandle, &lcMine, TRUE);
  }
}

#ifndef TESTING

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

  try {
    ls::hInstance = hInstance;
    
    ls::CreateMainWindow();
    AttachConsole();

    FreeImage_Initialise();

    wt::LoadWintabLibrary();
    wt::LoadEntryPoints();
    //ls::WintabReport();
    ls::TabletInit();

    vk::LoadVulkanLibrary();
    vk::LoadExportedEntryPoints();
    vk::LoadGlobalLevelEntryPoints();
    ls::CheckValidationAvailability();

    ls::CreateInstance();
    vk::LoadInstanceLevelEntryPoints(ls::instance, ls::INSTANCE_EXTENSIONS);

    ls::CreateDebugReportCallback(); // needs an instance level function
    ls::CreatePresentationSurface(); // need this for device creation

    ls::CreateDevice();
    vk::LoadDeviceLevelEntryPoints(ls::device, ls::DEVICE_EXTENSIONS);

    ls::CreateSemaphores();
    ls::CreateFence();
    ls::GetQueues();
    ls::CreateSwapChain();
    ls::GetSwapChainImages();
    ls::CreateSwapChainImageViews();
    ls::UpdatePixelDimensions();

    ls::CreateRenderPass();
    ls::CreateFramebuffers();
    ls::CreatePipelineLayout();
    ls::CreatePipeline();

    ls::CreateCommandBuffers();

    ShowWindow(ls::windowHandle, SW_SHOW);

    while (ls::Run()) {};
  }
  catch (...) {
    std::cout << "Exception thrown, cleaning-up." << std::endl;
  }; 

  ls::FreeCommandBuffers();
  ls::DestroyPipeline();
  ls::DestroyPipelineLayout();
  ls::DestroyFramebuffers();
  ls::DestroyRenderpass();

  ls::DestroyImageViews();
  ls::DestroySwapchain();
  ls::DestroyPresentationSurface();

  ls::DestroyFence();
  ls::DestroySemaphores();

  ls::FreeDevice();
  ls::DestroyDebugReportCallback();
  ls::FreeInstance();

  vk::UnloadVulkanLibrary();

  FreeImage_DeInitialise();
  return ls::msg.wParam;
}

#endif // #ifndef TESTING