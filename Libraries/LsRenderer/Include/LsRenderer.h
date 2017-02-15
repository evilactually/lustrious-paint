#pragma once

#include <windows.h>
#include <vector>
#include <memory>
#include <vulkan_dynamic.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <LsFWin32MessageHandler.h>

class LsRenderer {
public:
  LsRenderer(HINSTANCE hInstance, HWND window);
  ~LsRenderer();
  //static LsRenderer* Get();
  void BeginFrame();
  void EndFrame();
  void Clear(float r, float g, float b);
  void DrawLine(float x1, float y1, float x2, float y2);
  void DrawPoint(float x, float y);
  void SetColor(float r, float g, float b);
  void SetLineWidth(float width);
  void SetPointSize(float size);
  void RefreshSwapChain();
  int GetSurfaceWidth();
  int GetSurfaceHeight();
private:
  void BeginDrawing();
  void EndDrawing();
  enum class PipelineBinding {
    eNone,
    eLine,
    ePoint
  };

  // Singleton instance of the LsRenderer
  static LsRenderer renderer;
  
  HWND window;
  VkInstance instance;             // Vulkan instance
  VkPhysicalDevice physicalDevice; // Chosen physical device
  VkDevice device;                 // Logical device created from physicalDevice
  
#ifdef VULKAN_VALIDATION
  VkDebugReportCallbackEXT debugReportCallback; // Handle representing Vulkan debug callback
#endif

  struct {
    uint32_t  familyIndex;
    VkQueue handle;
  } graphicsQueue;

  struct {
    uint32_t  familyIndex;
    VkQueue handle;
  } presentQueue;

  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;

  struct {
    VkSurfaceKHR   presentationSurface;
    VkSwapchainKHR swapChain;
    VkFormat       format;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkExtent2D extent;
    uint32_t acquiredImageIndex;
  } swapChainInfo = {};

  bool canRender = false;

  struct {
    VkSemaphore imageAvailable;     // signals when swap chain image is acquired
    VkSemaphore renderingFinished;  // used to block presentation until rendering is finished
  } semaphores;

  VkFence submitCompleteFence;      // protects command buffer from being reset too soon
  
  VkRenderPass renderPass;

  std::vector<VkFramebuffer> framebuffers;

  struct {
    VkShaderModule lineVertexShader;
    VkShaderModule lineFragmentShader;
    VkShaderModule pointVertexShader;
    VkShaderModule pointFragmentShader;
  } shaderModules;

  VkPipelineLayout linePipelineLayout;  // pipeline layout for line push constants
  VkPipelineLayout pointPipelineLayout; // pipeleine layout for point push constants
  VkPipeline linePipeline;              // pipeline for drawing lines
  VkPipeline pointPipeline;             // pipeline for drawing points

  struct {
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float lineWidth = 1.0f;
    float pointSize = 1.0f;
    bool drawing = false; // indicates that command buffer is ready to draw
    PipelineBinding pipelineBinding = PipelineBinding::eNone;
  } drawingContext;

  glm::tmat3x3<float> windowToVulkanTransformation = { 
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
  };
};
