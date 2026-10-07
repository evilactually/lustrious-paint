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
#include <LsCanvas.h>
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
  std::shared_ptr<LsCanvas> canvas;
  std::shared_ptr<LsCanvasPainter> canvasPainter;
  // Scene revision the canvas last got its primitives from
  uint64_t canvasSceneRevision = UINT64_MAX;
  std::shared_ptr<LsStuffingDemo> stuffingDemo;
  std::shared_ptr<LsRenderer> renderer;
  std::shared_ptr<LsScene> scene;
  std::unique_ptr<LsSceneHistory> history;
  std::unique_ptr<LsSceneEditor> sceneEditor;
  std::unique_ptr<LsSceneViewport> sceneViewport;
  bool dockLayoutBuilt = false;
  // The canvas starts in the middle of its area, once the layout has given the area a size
  bool canvasCentered = false;

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
    canvas.reset();
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
	  canvas = std::make_shared<LsCanvas>(renderer);
	  canvasPainter = std::make_shared<LsCanvasPainter>(renderer, canvasNavigator, canvas);
    //stuffingDemo = std::make_shared<LsStuffingDemo>(renderer);

    const glm::vec3 canvasBackground(0.1f, 0.0f, 0.1f);
    pointGrid->SetBackgroundColor(canvasBackground.r, canvasBackground.g, canvasBackground.b);

    scene = std::make_shared<LsScene>();
    CreateStarterScene();
    // Undo starts from the starter scene, not from nothing
    history = std::make_unique<LsSceneHistory>(scene);
    sceneEditor = std::make_unique<LsSceneEditor>(scene);
    sceneViewport = std::make_unique<LsSceneViewport>(renderer, scene, sceneEditor.get());
    // The 3D view fades in from what surrounds the canvas
    sceneViewport->SetCanvasBackground(canvasBackground);

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

      DrawDockedPanels();
      DrawToolbar();
      HandleViewToggle(overUi);

      // Strokes painted since the last frame, before anything draws the canvas
      UpdateCanvas();

      // After laying out the UI, which decides whether the canvas or the 3D view
      // shows, and before the UI is rendered over everything at the end of the frame
      pointGrid->Render();
      if (!sceneViewport->IsShowingScene()) {
        renderer->DrawCanvas();
        if (!overUi && !navigating) {
          brushRig->Render();
        }
        canvasPainter->Render();
      }
      //stuffingDemo->Render();

      if (ImGui::IsKeyPressed(ImGuiKey_F9, false)) {
        char path[64];
        snprintf(path, sizeof(path), "Screenshot %llu.bmp", static_cast<unsigned long long>(SDL_GetTicks()));
        renderer->SaveScreenshot(path);
      }

      // Whatever the UI changed this frame becomes an undo step once the edit is over
      history->Update(ImGui::IsAnyItemActive() || sceneViewport->IsEditing());

      if (ImGui::Begin("Brush", nullptr, ImGuiWindowFlags_NoCollapse)) {
        float brushRadius = canvasPainter->GetBrushRadius();
        if (ImGui::SliderFloat("Radius", &brushRadius, 1.0f, 100.0f, "%.0f px")) {
          canvasPainter->SetBrushRadius(brushRadius);
        }
        // What the canvas shows of the painted positions
        int display = static_cast<int>(canvas->GetDisplay());
        if (ImGui::Combo("Display", &display, "Depth\0Position\0")) {
          canvas->SetDisplay(static_cast<LsCanvasDisplay>(display));
        }
        bool isolines = canvas->GetIsolines();
        if (ImGui::Checkbox("Isolines", &isolines)) {
          canvas->SetIsolines(isolines);
        }
        ImGui::SetItemTooltip("Contour lines of the depth, every tenth one brighter");
        if (isolines) {
          float spacing = canvas->GetIsolineSpacing();
          if (ImGui::DragFloat("Spacing", &spacing, 0.005f, 0.001f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
            canvas->SetIsolineSpacing(spacing);
          }
          ImGui::SetItemTooltip("Depth between isolines, in scene units");
        }
      }
      ImGui::End();

      renderer->EndFrame();
    }


  }

  // Strokes transfer positions from the scene as it is now, through the canvas camera
  void UpdateCanvas() {
    if (scene->GetRevision() != canvasSceneRevision) {
      canvas->SetPrimitives(scene->Flatten());
      canvasSceneRevision = scene->GetRevision();
    }
    LsNodeId camera = scene->GetActiveCamera();
    if (camera != kLsInvalidNode) {
      glm::vec2 size = renderer->GetCanvasSize();
      canvas->SetCamera(scene->GetCamera(camera, size.x/size.y));
    }
    else {
      canvas->ClearCamera();
    }
    canvas->Render();
  }

  // Tab over the canvas shows or hides the scene over it, in the 3D view it goes
  // back to the canvas. A focused panel claims Tab for keyboard navigation, the
  // mouse decides here.
  void HandleViewToggle(bool overUi) {
    bool overCanvasArea = sceneViewport->IsShowingScene() ? sceneViewport->IsHovered() : !overUi;
    bool tabPressed = ImGui::IsKeyPressed(ImGuiKey_Tab, ImGuiInputFlags_None, ImGuiKeyOwner_Any);
    if (ImGui::GetIO().WantTextInput || !overCanvasArea || !tabPressed) {
      return;
    }
    if (sceneViewport->IsShowingScene()) {
      sceneViewport->ToggleSceneView();
    }
    else {
      sceneViewport->SetSceneOverCanvas(!sceneViewport->IsSceneOverCanvas());
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

  // A Cornell box: a 2 by 2 by 2 room open at the front, lit by a panel under the
  // ceiling, with a tall and a short box inside, so there is something to start from
  void CreateStarterScene() {
    const LsSceneRoles solid = eRoleConstruction | eRoleShadowCaster;
    // Walls are thin boxes around the room, which spans x and z from -1 to 1 and y from 0 to 2
    const float halfRoom = 1.0f;
    const float halfThickness = 0.05f;
    const float outer = halfRoom + halfThickness;

    LsNodeId room = scene->AddGroup("Room");
    auto addBox = [&](const char* name, glm::vec3 halfExtents, glm::vec3 position, float yawDegrees,
                      LsSceneRoles roles, LsNodeId parent) {
      LsNodeId id = scene->AddShape(name, LsShape::Box(halfExtents), roles, parent);
      LsPlacement placement;
      placement.translation = position;
      placement.rotation.y = yawDegrees;
      scene->SetPlacement(id, placement);
      return id;
    };
    addBox("Floor", glm::vec3(outer, halfThickness, outer), glm::vec3(0.0f, -halfThickness, 0.0f), 0.0f, solid, room);
    addBox("Ceiling", glm::vec3(outer, halfThickness, outer), glm::vec3(0.0f, 2.0f*halfRoom + halfThickness, 0.0f), 0.0f, solid, room);
    addBox("Back Wall", glm::vec3(outer, halfRoom, halfThickness), glm::vec3(0.0f, halfRoom, -outer), 0.0f, solid, room);
    addBox("Left Wall", glm::vec3(halfThickness, halfRoom, outer), glm::vec3(-outer, halfRoom, 0.0f), 0.0f, solid, room);
    addBox("Right Wall", glm::vec3(halfThickness, halfRoom, outer), glm::vec3(outer, halfRoom, 0.0f), 0.0f, solid, room);

    // Just below the ceiling, so the ceiling doesn't shadow it
    LsNodeId light = addBox("Light", glm::vec3(0.25f, 0.01f, 0.2f), glm::vec3(0.0f, 2.0f*halfRoom - 0.02f, 0.0f), 0.0f,
                            eRoleLightEmitter, room);
    LsSceneNodeProperties properties = scene->Find(light)->GetProperties();
    properties.emissionColor = glm::vec3(1.0f, 0.85f, 0.7f);
    // Lights the floor to about mid grey at the default exposure
    properties.emissionIntensity = 60.0f;
    scene->SetProperties(light, properties);

    // The classic pair, turned towards each other
    addBox("Tall Box", glm::vec3(0.3f, 0.6f, 0.3f), glm::vec3(-0.35f, 0.6f, -0.3f), 18.0f, solid, kLsInvalidNode);
    addBox("Short Box", glm::vec3(0.3f, 0.3f, 0.3f), glm::vec3(0.35f, 0.3f, 0.35f), -18.0f, solid, kLsInvalidNode);

    // Looking into the room through the open front, which just fills the frame
    LsNodeId camera = scene->AddCamera("Canvas Camera");
    LsCamera view = LsCamera::LookAt(glm::vec3(0.0f, halfRoom, 3.9f), glm::vec3(0.0f, halfRoom, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 40.0f, 1.0f);
    scene->SetPlacement(camera, LsPlacement::FromMatrix(view.GetCameraToWorld()));
  }

  // Vertical tool strip floating over the canvas, buttons only, no window background.
  // The 3D view has its own.
  void DrawToolbar() {
    if (sceneViewport->IsShowingScene()) {
      return;
    }
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
    if (ImGui::Button("3D View", buttonSize)) {
      sceneViewport->ToggleSceneView();
    }
    ImGui::SetItemTooltip("Fly out of the canvas into the scene");

    // The scene through the canvas camera, to see what is being painted over
    bool sceneShown = sceneViewport->IsSceneOverCanvas();
    if (ImGui::Checkbox("Scene", &sceneShown)) {
      sceneViewport->SetSceneOverCanvas(sceneShown);
    }
    ImGui::SetItemTooltip("Show the scene over the canvas, as the canvas camera sees it (Tab)");
    if (sceneShown) {
      ImGui::SetNextItemWidth(buttonSize.x);
      ImGui::SliderFloat("##SceneOpacity", &sceneViewport->SceneOverCanvasOpacity(), 0.0f, 1.0f, "%.2f");
      ImGui::SetItemTooltip("Opacity of the scene over the canvas");
    }
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

  // Fills the window with a dock space whose centre lets input through to the canvas
  // or holds the 3D view, and keeps the Scene, Properties and Brush panels docked to its right side
  void DrawDockedPanels() {
    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    // Rebuild the layout on every launch, so each session starts from it. Panels can be
    // undocked and rearranged while it runs.
    if (!dockLayoutBuilt) {
      ImGui::DockBuilderRemoveNode(dockspaceId);
      ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
      ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

      ImGuiID rightId = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.25f, nullptr, nullptr);
      ImGuiID propertiesId;
      ImGuiID sceneId = ImGui::DockBuilderSplitNode(rightId, ImGuiDir_Up, 0.4f, nullptr, &propertiesId);
      ImGuiID brushId = ImGui::DockBuilderSplitNode(propertiesId, ImGuiDir_Down, 0.25f, nullptr, &propertiesId);
      ImGui::DockBuilderDockWindow("Scene", sceneId);
      ImGui::DockBuilderDockWindow("Properties", propertiesId);
      ImGui::DockBuilderDockWindow("Brush", brushId);
      ImGui::DockBuilderFinish(dockspaceId);
      dockLayoutBuilt = true;
    }

    sceneEditor->DrawOutliner();
    // The canvas and the 3D view take turns in the central node
    if (ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockspaceId)) {
      glm::vec2 areaMin(central->Pos.x, central->Pos.y);
      glm::vec2 areaSize(central->Size.x, central->Size.y);
      if (!canvasCentered && areaSize.x > 0.0f && areaSize.y > 0.0f) {
        // The canvas is placed in window pixels, the layout is in UI points
        glm::vec2 pixelsPerPoint(ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplayFramebufferScale.y);
        canvasNavigator->CenterIn(areaMin*pixelsPerPoint, areaSize*pixelsPerPoint);
        canvasCentered = true;
      }
      sceneViewport->Draw(areaMin, areaSize);
    }

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
