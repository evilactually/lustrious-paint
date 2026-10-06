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
#include <LsCanvasPainter.h>
#include <LsStuffingDemo.h>
#include <LsScene.h>
#include <LsSceneHistory.h>
#include <LsSceneEditor.h>
#include <LsSceneViewport.h>
#include <LsCanvasNavigator.h>
#include <LsConsole.h>
#include <LsError.h>
#include <LsImage.h>
#include <imgui.h>
#include <imgui_internal.h> // DockBuilder API
// #include <Test.h>

#include <LsIsosphere.h>
#include <LsTetrahedronMesh.h>
#include <LsIsosurfaceStuffer.h>

using namespace lslib;

class Application: LsFSdlEventHandler {
  LsSdlMainWindow* window = nullptr;
  std::shared_ptr<LsBrushRig> brushRig;
  std::shared_ptr<LsPointGrid> pointGrid;
  std::shared_ptr<LsCanvasNavigator> canvasNavigator;
  std::shared_ptr<LsCanvasPainter> canvasPainter;
  std::shared_ptr<LsStuffingDemo> stuffingDemo;
  std::shared_ptr<LsRenderer> renderer;
  std::shared_ptr<LsScene> scene;
  std::unique_ptr<LsSceneHistory> history;
  std::unique_ptr<LsSceneEditor> sceneEditor;
  std::unique_ptr<LsSceneViewport> sceneViewport;
  bool dockLayoutBuilt = false;

  // Placeholder tools until the brush modes exist
  enum class Tool { Brush, Smooth, Inflate };
  Tool currentTool = Tool::Brush;
public:
  Application() {
    LsCloseConsole();
  }

  ~Application() {
    // Vulkan objects must be released before the window and SDL itself
    sceneViewport.reset();
    stuffingDemo.reset();
    canvasPainter.reset();
    canvasNavigator.reset();
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
    window->Create("Lustrious Paint", 100, 100, 1280, 800);
    LsSetDialogParentWindow(window->GetWindowHandle());

    window->HideMouse();

    // Initialize the rendering system
    renderer = std::make_shared<LsRenderer>(window->GetWindowHandle());
	  renderer->RefreshSwapChain();

    // The UI sees events first, anything it captures doesn't reach the painting components
    window->SetEventFilter([this](const SDL_Event& event) {
      return renderer->ProcessUiEvent(event);
    });

	  // Initialize components
	  brushRig = std::make_shared<LsBrushRig>(renderer);
	  pointGrid = std::make_shared<LsPointGrid>(renderer);
	  canvasNavigator = std::make_shared<LsCanvasNavigator>(renderer);
	  canvasPainter = std::make_shared<LsCanvasPainter>(renderer, canvasNavigator);
    //stuffingDemo = std::make_shared<LsStuffingDemo>(renderer);

    pointGrid->SetBackgroundColor(0.1f, 0.0f, 0.1f);

    scene = std::make_shared<LsScene>();
    CreateStarterScene();
    // Undo starts from the starter scene, not from nothing
    history = std::make_unique<LsSceneHistory>(scene);
    sceneEditor = std::make_unique<LsSceneEditor>(scene);
    sceneViewport = std::make_unique<LsSceneViewport>(renderer, scene, sceneEditor.get());

    LsImage image = renderer->CreateImage(640, 800);
    renderer->CreateImage(8000, 8000);
    renderer->CreateImage(8000, 8000);
    renderer->CreateImage(8000, 8000);
    // image = renderer->CreateImage(8000, 8000);
    // image = renderer->CreateImage(8000, 8000);
  }

  void Run() {
    window->Show();

    // Dear ImGui needs a few frames after input to settle (new windows stay hidden on their
    // first frame while they are measured), so keep drawing briefly before blocking for events
    const int settleFrames = 3;
    int idleFrames = 0;
    while ( true ) {
      if ( idleFrames >= settleFrames ) {
        window->WaitForMessages();
      }
      SDL_PumpEvents();
      idleFrames = SDL_HasEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST) ? 0 : idleFrames + 1;
      // Flying keeps moving while keys are held, even without new events
      if ( sceneViewport->IsAnimating() ) {
        idleFrames = 0;
      }
      if ( !window->ProcessMessages() ) {
        break;
      }

      renderer->BeginFrame();
      HandleUndoShortcuts();

      // The system cursor shows only over the UI, the brush rig is the cursor over the canvas
      bool overUi = renderer->IsUiCapturingMouse();
      // While space or ctrl is held the canvas is being navigated, show a system cursor for that instead
      bool navigating = !overUi && canvasNavigator->IsActive();
      if (navigating) {
        canvasNavigator->UpdateCursor();
      }
      else if (!overUi) {
        window->HideMouse();
      }

      pointGrid->Render();
      renderer->DrawCanvas();
      if (!overUi && !navigating) {
        brushRig->Render();
      }

      canvasPainter->Render();
      //stuffingDemo->Render();

      DrawDockedPanels();
      DrawToolbar();

      if (ImGui::IsKeyPressed(ImGuiKey_F9, false)) {
        char path[64];
        snprintf(path, sizeof(path), "Screenshot %llu.bmp", static_cast<unsigned long long>(SDL_GetTicks()));
        renderer->SaveScreenshot(path);
      }

      // Whatever the UI changed this frame becomes an undo step once the edit is over
      history->Update(ImGui::IsAnyItemActive() || sceneViewport->IsEditing());

      ImGui::ShowDemoWindow();

      if (ImGui::Begin("Brush")) {
        float brushRadius = canvasPainter->GetBrushRadius();
        if (ImGui::SliderFloat("Radius", &brushRadius, 1.0f, 100.0f, "%.0f px")) {
          canvasPainter->SetBrushRadius(brushRadius);
        }
      }
      ImGui::End();

      renderer->EndFrame();
    }


  }

  // Text fields have their own undo while they are being typed in
  void HandleUndoShortcuts() {
    if (ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive() || sceneViewport->IsEditing()) {
      return;
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) {
      history->Undo();
    }
    else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y) || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)) {
      history->Redo();
    }
  }

  // Ground, an object to paint from, and a light, so there is something to start from
  void CreateStarterScene() {
    scene->AddShape("Ground", LsShape::Plane(10.0f), eRoleConstruction);

    LsNodeId sphere = scene->AddShape("Sphere", LsShape::Sphere(0.5f), eRoleConstruction | eRoleShadowCaster);
    LsPlacement placement;
    placement.translation = glm::vec3(0.0f, 0.5f, 0.0f);
    scene->SetPlacement(sphere, placement);

    LsNodeId light = scene->AddShape("Key Light", LsShape::Sphere(0.25f), eRoleLightEmitter);
    LsSceneNodeProperties properties = scene->Find(light)->GetProperties();
    properties.placement.translation = glm::vec3(2.0f, 3.0f, 2.0f);
    // Lights the sphere to about mid grey at the default exposure
    properties.emissionIntensity = 150.0f;
    scene->SetProperties(light, properties);

    // Looking at the sphere from the front, slightly above
    LsNodeId camera = scene->AddCamera("Canvas Camera");
    LsCamera view = LsCamera::LookAt(glm::vec3(0.0f, 1.2f, 4.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 40.0f, 1.0f);
    scene->SetPlacement(camera, LsPlacement::FromMatrix(view.GetCameraToWorld()));
  }

  // Vertical tool strip floating over the canvas, buttons only, no window background
  void DrawToolbar() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoSavedSettings;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 12.0f, viewport->WorkPos.y + 12.0f), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##Toolbar", nullptr, flags);

    struct { const char* label; Tool tool; } buttons[] = {
      { "Brush", Tool::Brush },
      { "Smooth", Tool::Smooth },
      { "Inflate", Tool::Inflate },
    };

    const ImVec2 buttonSize(ImGui::GetFontSize()*5.0f, 0.0f);
    // for (const auto& button : buttons) {
    //   // The selected tool is drawn in the accent colour
    //   bool selected = currentTool == button.tool;
    //   if (selected) {
    //     ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    //     ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive));
    //   }
    //   if (ImGui::Button(button.label, buttonSize)) {
    //     currentTool = button.tool;
    //   }
    //   if (selected) {
    //     ImGui::PopStyleColor(2);
    //   }
    // }

    ImGui::End();
    ImGui::PopStyleVar();
  }

  // Fills the window with a dock space whose centre lets input through to the canvas,
  // and keeps the Properties panel docked to its right side
  void DrawDockedPanels() {
    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    // Rebuild the layout on every launch, so a layout saved in imgui.ini can't move the panel
    if (!dockLayoutBuilt) {
      ImGui::DockBuilderRemoveNode(dockspaceId);
      ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
      ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

      ImGuiID centerId;
      ImGuiID rightId = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.25f, nullptr, &centerId);
      // The 3D view shares the rest with the canvas, which shows through the central node
      ImGuiID viewId = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, 0.5f, nullptr, nullptr);
      ImGuiID propertiesId;
      ImGuiID sceneId = ImGui::DockBuilderSplitNode(rightId, ImGuiDir_Up, 0.4f, nullptr, &propertiesId);
      ImGui::DockBuilderGetNode(sceneId)->LocalFlags |= ImGuiDockNodeFlags_NoUndocking;
      ImGui::DockBuilderGetNode(propertiesId)->LocalFlags |= ImGuiDockNodeFlags_NoUndocking;
      ImGui::DockBuilderDockWindow("Scene", sceneId);
      ImGui::DockBuilderDockWindow("Properties", propertiesId);
      ImGui::DockBuilderDockWindow("3D View", viewId);
      ImGui::DockBuilderFinish(dockspaceId);
      dockLayoutBuilt = true;
    }

    sceneEditor->DrawOutliner();
    sceneViewport->Draw();

    if (ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoCollapse)) {
      sceneEditor->DrawProperties();
    }
    ImGui::End();
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
