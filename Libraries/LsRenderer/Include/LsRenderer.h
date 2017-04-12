#pragma once

#include <windows.h>
#include <vector>
#include <memory>
#include <vulkan_dynamic.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <LsFWin32MessageHandler.h>
#include <chrono>

typedef void (*LsFrameCaptureCallback)();

class LsRenderer {
  friend DWORD WINAPI WriteFrameThread(LPVOID lpParam);
public:
  LsRenderer(HINSTANCE hInstance, HWND window);
  ~LsRenderer();
  //static LsRenderer* Get();
#ifdef GIF_RECORDING
  void StartGIFRecording(std::string filename);
  void StopGIFRecording();
#endif
  void BeginFrame();
  void EndFrame();
  void Clear(float r, float g, float b);
  void DrawLine(float x1, float y1, float x2, float y2);
  void DrawLine(glm::vec2 p1, glm::vec2 p2);
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
#ifdef GIF_RECORDING
  void CreateCapturedFrameImage();
  void RecordFrameCaptureCmds();
  void OpenBufferFile();
  void BeginSavingCapturedFrame();
  bool CheckFrameSavingFinished();
#endif
  
  enum class PipelineBinding {
    eNone,
    eLine,
    ePoint
  };
  
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

  struct {
    VkFence captureCompleteFence;          // used to wait by host for copy to complete
    VkSemaphore captureFinishedSemaphore;  // used to block presentation while copying
    VkCommandBuffer captureFrameCmds;      // commands used to capture a frame
    struct {
      VkImage image;
      VkDeviceMemory memory;
      VkDeviceSize size;
      VkExtent2D extent;
      uint32_t rowPitch;
    } capturedFrameImage;                  // captured frame image buffer and information about it

    HANDLE hCaptureBufferFile;             // temporary file to store frames before processing
    HANDLE hSavingThread = NULL;           // handle to thread that is currently saving a frame to temporary file
    uint32_t captureFrameCount;            // running count of captured frames in a recording

    BOOL capturing = false;                // flag indicating whether we are currently capturing
    std::chrono::high_resolution_clock::time_point startTime;
    std::string filename;
  } captureInfo;                           // time when capture was started(used to estimate framerate)

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
