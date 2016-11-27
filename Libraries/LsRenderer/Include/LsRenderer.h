#pragma once

#include <windows.h>
#include <vulkan_dynamic.hpp>
#include <vector>
#include <memory>
#include <destructor.h>
#include <LsFWin32MessageHandler.h>

class LsRenderer: public LsFWin32MessageHandler {
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
  void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
private:
  LsRenderer();
  ~LsRenderer();
  void BeginDrawing();
  void EndDrawing();
  void RefreshSwapChain();
  
  enum class PipelineBinding {
    eNone,
    eLine,
    ePoint
  };

  // Singleton instance of the LsRenderer
  static LsRenderer renderer;
  
  HWND window;
  vk::Instance instance;             // Vulkan instance
  vk::PhysicalDevice physicalDevice; // Chosen physical device
  vk::Device device;                 // Logical device created from physicalDevice
  
#ifdef VULKAN_VALIDATION
  vk::DebugReportCallbackEXT debugReportCallback; // Handle representing Vulkan debug callback
#endif

  struct {
    uint32_t  familyIndex;
    vk::Queue handle;
  } graphicsQueue;

  struct {
    uint32_t  familyIndex;
    vk::Queue handle;
  } presentQueue;

  vk::CommandPool commandPool;
  vk::CommandBuffer commandBuffer;

  struct {
    vk::SurfaceKHR   presentationSurface;
    vk::SwapchainKHR swapChain;
    vk::Format       format;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;
    vk::Extent2D extent;
    uint32_t acquiredImageIndex;
  } swapChainInfo;

  bool canRender = false;

  struct {
    vk::Semaphore imageAvailable;     // signals when swap chain image is acquired
    vk::Semaphore renderingFinished;  // used to block presentation until rendering is finished
  } semaphores;

  vk::Fence submitCompleteFence;      // protects command buffer from being reset too soon
  
  vk::RenderPass renderPass;

  std::vector<vk::Framebuffer> framebuffers;

  struct {
    vk::ShaderModule lineVertexShader;
    vk::ShaderModule lineFragmentShader;
    vk::ShaderModule pointVertexShader;
    vk::ShaderModule pointFragmentShader;
  } shaderModules;

  vk::PipelineLayout linePipelineLayout;  // pipeline layout for line push constants
  vk::PipelineLayout pointPipelineLayout; // pipeleine layout for point push constants
  vk::Pipeline linePipeline;              // pipeline for drawing lines
  vk::Pipeline pointPipeline;             // pipeline for drawing points

  struct {
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float lineWidth = 1.0f;
    float pointSize = 1.0f;
    bool drawing = false; // indicates that command buffer is ready to draw
    PipelineBinding pipelineBinding = PipelineBinding::eNone;
  } drawingContext;

};

