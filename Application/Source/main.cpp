#include <string>
#include <iostream>
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <destructor.h>
#include <LsSdlMainWindow.h>
#include <LsFSdlEventHandler.h>
#include <LsRenderer.h>
#include <LsVulkanLoader.h>
#include <LsBrushRig.h>
#include <LsPointGrid.h>
#include <LsUselessBox.h>
#include <LsStuffingDemo.h>
#include <LsConsole.h>
#include <LsError.h>
#include <LsImage.h>
// #include <Test.h>

#include <LsIsosphere.h>
#include <LsTetrahedronMesh.h>
#include <LsIsosurfaceStuffer.h>

using namespace lslib;

class Application: LsFSdlEventHandler {
  LsSdlMainWindow* window = nullptr;
  std::shared_ptr<LsBrushRig> brushRig;
  std::shared_ptr<LsPointGrid> pointGrid;
  std::shared_ptr<LsUselessBox> uselessBox;
  std::shared_ptr<LsStuffingDemo> stuffingDemo;
  std::shared_ptr<LsRenderer> renderer;
public:
  Application() {
    LsCloseConsole();
  }

  ~Application() {
    // Vulkan objects must be released before the window and SDL itself
    stuffingDemo.reset();
    uselessBox.reset();
    pointGrid.reset();
    brushRig.reset();
    renderer.reset();
    if (window) {
      window->Destroy();
    }
  }

  void Initialize() {
    // Attach console
    LsOpenConsole();

    // Get a pointer to main window singleton
    window = LsSdlMainWindow::Get();

    // Tablet input arrives as SDL pen events, no separate tablet context is needed
    window->Create("Lustrious Paint", 100, 100, 640, 800);
    LsSetDialogParentWindow(window->GetWindowHandle());

    window->HideMouse();

    // Initialize the rendering system
    renderer = std::make_shared<LsRenderer>(window->GetWindowHandle());
	  renderer->RefreshSwapChain();

	  // Initialize components
	  brushRig = std::make_shared<LsBrushRig>(renderer);
	  pointGrid = std::make_shared<LsPointGrid>(renderer);
	  uselessBox = std::make_shared<LsUselessBox>(renderer);
    //stuffingDemo = std::make_shared<LsStuffingDemo>(renderer);

    pointGrid->SetBackgroundColor(0.1f, 0.0f, 0.1f);

    LsImage image = renderer->CreateImage(640, 800);
    renderer->CreateImage(8000, 8000);
    renderer->CreateImage(8000, 8000);
    renderer->CreateImage(8000, 8000);
    // image = renderer->CreateImage(8000, 8000);
    // image = renderer->CreateImage(8000, 8000);
  }

  void Run() {
    window->Show();
    while ( window->ProcessMessages() ) {
      window->WaitForMessages();
      renderer->BeginFrame();

      pointGrid->Render();
      renderer->DrawCanvas();
      brushRig->Render();

      uselessBox->Render();
      //stuffingDemo->Render();
      renderer->EndFrame();
    };


  }

  void OnSdlEvent(const SDL_Event& event) override {
    switch (event.type) {
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        if (renderer) {
          renderer->RefreshSwapChain();
        }
      break;
      case SDL_EVENT_KEY_DOWN:
#ifdef GIF_RECORDING
        if (event.key.key == SDLK_F12) {
          renderer->StartGIFRecording("C:\\Users\\Public\\Downloads\\out.gif");
          std::cout << "PRESSED" << std::endl;
        }
        else if (event.key.key == SDLK_F11) {
          renderer->StopGIFRecording();
        }
#endif
      break;
    }
  }
};

int main(int argc, char* argv[]) {
  //LsIsosphere sphere({0.0f, 0.0f, 0.0f}, 1.0f);
  //LsTetrahedronMesh mesh;
  //LsIsosurfaceStuffer stuffer;
  //stuffer.Stuff(mesh, sphere);
  try {
    Application application;
    application.Initialize();// test();
    application.Run();
  } catch (std::string m) {
    LsErrorMessage(m, "Error");
  }

  system("pause");
  return 0;
}
