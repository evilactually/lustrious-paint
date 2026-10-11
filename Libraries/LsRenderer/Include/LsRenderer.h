#pragma once

#include <vector>
#include <memory>
#include <vulkan_dynamic.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <LsImage.h>
#include <chrono>
#include <string>
#include <cstdint> // uint_32

typedef void (*LsFrameCaptureCallback)();

class LsRenderer {
#ifdef FRAME_RECORDING
  friend int SDLCALL WriteFrameThread(void* data);
#endif
  friend class LsImage;
  friend class LsSceneView;
  friend class LsCanvas;
public:
  LsRenderer(SDL_Window* window);
  ~LsRenderer();
  //static LsRenderer* Get();
#ifdef FRAME_RECORDING
  // Writes every presented frame to <basePath>-00001.bmp, <basePath>-00002.bmp and so on,
  // skipping frames while the previous one is still being written
  void StartFrameRecording(std::string basePath);
  void StopFrameRecording();
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
  // Places the canvas in the window: window pixel = canvas pixel*zoom + offset
  void SetCanvasView(glm::vec2 offset, float zoom);
  glm::vec2 GetCanvasOffset() { return canvasView.offset; }
  float GetCanvasZoom() { return canvasView.zoom; }
  glm::vec2 GetCanvasSize(); // in canvas pixels
  // The canvas image for drawing with Dear ImGui, such as on a plane in the 3D view
  ImTextureID GetCanvasTexture();
  // SetTransformation(glm::max4x4& transformation)
  // CreateVertexBuffer (separate class)
  // SetVertexBuffer(&VertexBuffer)
  // DrawLines(void)
  // DrawPoints(void)
  // Dear ImGui: a UI frame is started by BeginFrame and drawn on top of everything by EndFrame,
  // so ImGui:: calls can be made anywhere in between
  bool ProcessUiEvent(const SDL_Event& event); // returns true when the UI captures the event
  bool IsUiCapturingMouse();
  // Writes the frame being recorded to a 32-bit BMP file once it has been rendered,
  // call between BeginFrame and EndFrame
  void SaveScreenshot(const std::string& path);
private:
  void RecordScreenshotCopy();
  void WriteScreenshot();

  // TODO: Decouple canvas code from rendering, it doesn't belong here, but it needs low level Vulkan access
  void InitializeCanvas(uint32_t width, uint32_t height);
  
  void InitializeUi();
  void ShutdownUi();
  void BeginUiFrame();
  void RenderUi();

  void getComputeQueue();
  void BeginDrawing();
  void EndDrawing();
#ifdef FRAME_RECORDING
  void CreateCapturedFrameImage();
  void RecordFrameCaptureCmds();
  void BeginSavingCapturedFrame();
  bool CheckFrameSavingFinished();
#endif
  
  enum class PipelineBinding {
    eNone,
    eLine,
    ePoint,
    eImage
  };
  
  SDL_Window* window;
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

#ifdef FRAME_RECORDING
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

    SDL_Thread* savingThread = nullptr;    // thread that is currently writing a frame out
    SDL_AtomicInt savingFrame = {};        // set while savingThread is writing
    uint32_t captureFrameCount;            // running count of captured frames in a recording

    bool capturing = false;                // flag indicating whether we are currently capturing
    std::string basePath;                  // frames are written to <basePath>-<frame number>.bmp
  } captureInfo;
#endif

  bool canRender = false;
  bool uiInitialized = false;

  struct {
    std::string path; // pending when not empty
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkExtent2D extent = {};
    bool recorded = false;
  } screenshot;

  struct {
    VkSemaphore imageAvailable;     // signals when swap chain image is acquired
    VkSemaphore renderingFinished;  // used to block presentation until rendering is finished
  } semaphores;

  VkFence submitCompleteFence;      // protects command buffer from being reset too soon
  // Counts BeginFrame calls. Work recorded in one frame has finished on the GPU
  // once a later frame has begun, there is only one frame in flight.
  uint64_t frameNumber = 0;
  
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
    uint32_t queueFamilyIndex;
  } canvasState;

  struct {
    glm::vec2 offset = glm::vec2(0.0f);
    float zoom = 1.0f;
  } canvasView;

  VkDescriptorSet canvasUiTexture = VK_NULL_HANDLE; // ImGui's descriptor for the canvas
  VkDescriptorPool descriptorPool;
  VkDescriptorSet imageDescriptorSet; // make member?
};
