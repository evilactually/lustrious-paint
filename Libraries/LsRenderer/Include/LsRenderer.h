#pragma once

#include <windows.h>
#include <vulkan_dynamic.hpp>
#include <vector>
#include <memory>
#include <destructor.h>

#define VULKAN_VALIDATION

class LsRenderer {
public:
  static void Initialize(HINSTANCE hInstance, HWND window);
  static LsRenderer* Get();
  void BeginFrame();
  void EndFrame();
  void Clear(float r, float g, float b);
  void DrawLine(float x1, float y1, float x2, float y2);
  void DrawPoint(float x, float y);
  void SetColor(float r, float g, float b);
  void SetLineWidth(float width);
  void SetPointSize(float size);
private:
  LsRenderer();
  ~LsRenderer();
  void BeginDrawing();
  void EndDrawing();
    
  // Singleton instance of the LsRenderer
  static LsRenderer renderer;

  vk::Instance instance;             // Vulkan instance
  vk::PhysicalDevice physicalDevice; // Chosen physical device
  vk::Device device;                 // Logical device created from physicalDevice
  
#ifdef VULKAN_VALIDATION
  vk::DebugReportCallbackEXT debugReportCallback; // Handle representing Vulkan debug callback
#endif

  struct {
    vk::SurfaceKHR   presentationSurface;
    vk::SwapchainKHR swapChain;
    vk::Format       format;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;
    vk::Extent2D extent;
    uint32_t acquiredImageIndex;
  } swapChainInfo;

  std::unique_ptr<lslib::destructor> vulkanDestructor;

};

