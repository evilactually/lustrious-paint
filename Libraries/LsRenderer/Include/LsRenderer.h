#pragma once

#include <windows.h>
#pragma once

#include <vector>
#include <memory>
#include <vulkan_dynamic.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <LsFWin32MessageHandler.h>
#include <LsImage.h>
#include <chrono>
#include <string>
#include <cstdint> // uint_32

typedef void (*LsFrameCaptureCallback)();

class LsRenderer {
  friend DWORD WINAPI WriteFrameThread(LPVOID lpParam);
  friend class LsImage;
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
  LsImage CreateImage(uint32_t width, uint32_t height);
  void DrawImage(glm::vec2 p1, glm::vec2 p2, LsImage image);
  void DrawLine(float x1, float y1, float x2, float y2);
  void DrawLine(glm::vec2 p1, glm::vec2 p2);
  void DrawPoint(float x, float y);
  //void PaintIntoCanvas(float x, float y, float size);
  void SetColor(float r, float g, float b);
  void SetLineWidth(float width);
  void SetPointSize(float size);
  void RefreshSwapChain();
  int GetSurfaceWidth();
  int GetSurfaceHeight();
  void DrawCanvas();
  // SetTransformation(glm::max4x4& transformation)
  // CreateVertexBuffer (separate class)
  // SetVertexBuffer(&VertexBuffer)
  // DrawLines(void)
  // DrawPoints(void)
  void CanvasStroke(float x, float y, float radius);
private:
  // TODO: Decouple canvas code from rendering, it doesn't belong here, but it needs low level Vulkan access
  void InitializeCanvas(uint32_t width, uint32_t height);
  
  void getComputeQueue();
  //void CanvasStroke();
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
    ePoint,
    eImage
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
    VkShaderModule imageVertexShader;
    VkShaderModule imageFragmentShader;
  } shaderModules;

  VkPipelineLayout linePipelineLayout;  // pipeline layout for line push constants
  VkPipelineLayout pointPipelineLayout; // pipeleine layout for point push constants
  VkPipelineLayout imagePipelineLayout; // pipeleine layout for image push constants
  VkPipeline linePipeline;              // pipeline for drawing lines
  VkPipeline pointPipeline;             // pipeline for drawing points
  VkPipeline imagePipeline;             // pipeline for drawing textured quads

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

  struct {
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize size;
    uint32_t width, height;
    VkSampler sampler;
    VkFormat format;
    VkImageView view;
    VkImageLayout imageLayout;
    VkDescriptorImageInfo descriptor;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkPipelineLayout pipelineLayout;
    VkShaderModule paintComputeShader;
    VkPipeline pipeline;
    uint32_t queueFamilyIndex;
    VkQueue queue;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence fence;
  } canvasState;

  VkDescriptorPool descriptorPool;
  VkDescriptorSet imageDescriptorSet; // make member?
};
