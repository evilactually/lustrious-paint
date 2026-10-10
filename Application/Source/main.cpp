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
#include <cmath>
// #include <Test.h>

#include <LsIsosphere.h>
#include <LsTetrahedronMesh.h>
#include <LsIsosurfaceStuffer.h>

using namespace lslib;

// Colour pickers show sRGB values, the renderer works in linear ones
static float SrgbToLinear(float c) {
  return c <= 0.04045f ? c/12.92f : std::pow((c + 0.055f)/1.055f, 2.4f);
}

static float LinearToSrgb(float c) {
  return c <= 0.0031308f ? c*12.92f : 1.055f*std::pow(c, 1.0f/2.4f) - 0.055f;
}

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
      // Flying keeps moving while keys are held, even without new events, and the
      // rendering keeps adding samples until it is done
      if ( sceneViewport->IsAnimating() || canvas->IsRendering() ) {
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
      HandleToolShortcuts();
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
        // What a stroke paints: depth and normals from the construction geometry,
        // the BRDF panel's material, or erasing all of them
        struct { const char* label; LsCanvasDabMode mode; const char* tooltip; } modes[] = {
          { "Depth", LsCanvasDabMode::Depth, "Paint positions from the construction geometry (B)" },
          { "Normals", LsCanvasDabMode::Normal, "Paint normals from the construction geometry (N)" },
          { "Material", LsCanvasDabMode::Material, "Paint the BRDF panel's material (M)" },
          { "Erase", LsCanvasDabMode::Erase, "Remove painted positions, normals and materials (E)" },
        };
        for (const auto& mode : modes) {
          if (&mode != modes) {
            ImGui::SameLine();
          }
          if (ImGui::RadioButton(mode.label, canvasPainter->GetMode() == mode.mode)) {
            SetBrushMode(mode.mode);
          }
          ImGui::SetItemTooltip("%s", mode.tooltip);
        }
        float brushRadius = canvasPainter->GetBrushRadius();
        if (ImGui::SliderFloat("Radius", &brushRadius, 1.0f, 100.0f, "%.0f px")) {
          canvasPainter->SetBrushRadius(brushRadius);
        }
      }
      ImGui::End();

      if (ImGui::Begin("View", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // What the canvas shows of the painted attributes
        int display = static_cast<int>(canvas->GetDisplay());
        if (ImGui::Combo("Display", &display, "Depth\0Position\0Normal\0Rendering\0Material\0")) {
          canvas->SetDisplay(static_cast<LsCanvasDisplay>(display));
        }
        ImGui::SetItemTooltip("Rendering lights the painted points with the scene's emitters, shaded with the BRDF panel");
        bool rendering = canvas->GetDisplay() == LsCanvasDisplay::Rendering;
        if (rendering) {
          DrawRenderSettings();
        }
        bool isolines = canvas->GetIsolines();
        if (!rendering && ImGui::Checkbox("Isolines", &isolines)) {
          canvas->SetIsolines(isolines);
        }
        if (!rendering) {
          ImGui::SetItemTooltip("Contour lines of the depth, every tenth one brighter");
        }
        if (!rendering && isolines) {
          float spacing = canvas->GetIsolineSpacing();
          if (ImGui::DragFloat("Spacing", &spacing, 0.005f, 0.001f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
            canvas->SetIsolineSpacing(spacing);
          }
          ImGui::SetItemTooltip("Depth between isolines, in scene units");
        }
      }
      ImGui::End();

      if (ImGui::Begin("BRDF", nullptr, ImGuiWindowFlags_NoCollapse)) {
        DrawBrdf();
      }
      ImGui::End();

      renderer->EndFrame();
    }


  }

  // How the rendering is exposed and how fast it accumulates samples
  void DrawRenderSettings() {
    LsCanvasRenderSettings settings = canvas->GetRenderSettings();
    bool changed = ImGui::SliderFloat("Exposure", &settings.exposure, -10.0f, 10.0f, "%.1f stops");
    int samplesPerFrame = static_cast<int>(settings.samplesPerFrame);
    if (ImGui::SliderInt("Samples/frame", &samplesPerFrame, 1, 64, "%d", ImGuiSliderFlags_Logarithmic)) {
      settings.samplesPerFrame = static_cast<uint32_t>(samplesPerFrame);
      changed = true;
    }
    ImGui::SetItemTooltip("Samples added to each pixel every frame, fewer keep the application responsive");
    int maxSamples = static_cast<int>(settings.maxSamples);
    if (ImGui::SliderInt("Max samples", &maxSamples, 1, 16384, "%d", ImGuiSliderFlags_Logarithmic)) {
      settings.maxSamples = static_cast<uint32_t>(maxSamples);
      changed = true;
    }
    ImGui::SetItemTooltip("Samples per pixel after which the rendering stops");
    if (changed) {
      canvas->SetRenderSettings(settings);
    }
    float progress = static_cast<float>(canvas->GetRenderedSamples())/static_cast<float>(settings.maxSamples);
    char label[64];
    snprintf(label, sizeof(label), "%u / %u samples", canvas->GetRenderedSamples(), settings.maxSamples);
    ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0f), label);
  }

  // The material strokes paint, and every material painted so far to pick it from
  void DrawBrdf() {
    LsBrdfParameters brdf = canvas->GetBrushBrdf();
    bool changed = false;
    float color[3] = { LinearToSrgb(brdf.baseColor.r), LinearToSrgb(brdf.baseColor.g), LinearToSrgb(brdf.baseColor.b) };
    if (ImGui::ColorEdit3("Base color", color, ImGuiColorEditFlags_Float)) {
      brdf.baseColor = glm::vec3(SrgbToLinear(color[0]), SrgbToLinear(color[1]), SrgbToLinear(color[2]));
      changed = true;
    }
    ImGui::SetItemTooltip("Diffuse colour of dielectrics, reflectance at normal incidence of metals");
    changed |= ImGui::SliderFloat("Metallic", &brdf.metallic, 0.0f, 1.0f, "%.2f");
    ImGui::SetItemTooltip("Blends from a dielectric coat over a diffuse base to a bare metal");
    changed |= ImGui::SliderFloat("Roughness", &brdf.roughness, 0.0f, 1.0f, "%.2f");
    ImGui::SetItemTooltip("Perceptual roughness of the microfacets, alpha is its square");
    changed |= ImGui::SliderFloat("IOR", &brdf.ior, 1.0f, 3.0f, "%.2f");
    ImGui::SetItemTooltip("Index of refraction of the dielectric coat, sets how strongly it reflects");
    if (changed) {
      canvas->SetBrushBrdf(brdf);
    }
    if (canvasPainter->GetMode() != LsCanvasDabMode::Material && ImGui::Button("Paint material")) {
      SetBrushMode(LsCanvasDabMode::Material);
    }

    // Each swatch is one entry of the canvas material table, pixels refer to it by index
    const std::vector<LsBrdfParameters>& materials = canvas->GetMaterials();
    ImGui::SeparatorText("Materials");
    float swatchSize = ImGui::GetFrameHeight();
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    for (size_t i = 0; i < materials.size(); i++) {
      const LsBrdfParameters& material = materials[i];
      ImVec4 color(LinearToSrgb(material.baseColor.r), LinearToSrgb(material.baseColor.g), LinearToSrgb(material.baseColor.b), 1.0f);
      ImGui::PushID(static_cast<int>(i));
      bool current = material == canvas->GetBrushBrdf();
      if (current) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.55f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
      }
      if (ImGui::ColorButton("##material", color, ImGuiColorEditFlags_NoTooltip, ImVec2(swatchSize, swatchSize))) {
        canvas->SetBrushBrdf(material);
      }
      if (current) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
      }
      ImGui::SetItemTooltip("Material %zu%s\nMetallic %.2f, roughness %.2f, IOR %.2f", i,
                            i == 0 ? " (unpainted pixels)" : "", material.metallic, material.roughness, material.ior);
      ImGui::PopID();
      // Wrap the swatches to the panel width
      float next = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + swatchSize;
      if (i + 1 < materials.size() && next < right) {
        ImGui::SameLine();
      }
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

  // The canvas switches to showing what the brush now paints. Depth strokes are
  // seen as depth or as positions, so only the normal and material displays are
  // switched away from. Erasing removes everything and keeps whatever is shown,
  // and the rendering shows the result of every stroke, so it is kept too.
  void SetBrushMode(LsCanvasDabMode mode) {
    canvasPainter->SetMode(mode);
    LsCanvasDisplay display = canvas->GetDisplay();
    if (display == LsCanvasDisplay::Rendering) {
      return;
    }
    if (mode == LsCanvasDabMode::Depth && (display == LsCanvasDisplay::Normal || display == LsCanvasDisplay::Material)) {
      canvas->SetDisplay(LsCanvasDisplay::Depth);
    }
    else if (mode == LsCanvasDabMode::Material) {
      canvas->SetDisplay(LsCanvasDisplay::Material);
    }
    else if (mode == LsCanvasDabMode::Normal) {
      canvas->SetDisplay(LsCanvasDisplay::Normal);
    }
  }

  // B paints depth, N normals, M materials and E erases, B and E as the brush
  // and the eraser in most painting applications
  void HandleToolShortcuts() {
    if (ImGui::GetIO().WantTextInput || sceneViewport->IsShowingScene()) {
      return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_B, false)) {
      SetBrushMode(LsCanvasDabMode::Depth);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_N, false)) {
      SetBrushMode(LsCanvasDabMode::Normal);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_M, false)) {
      SetBrushMode(LsCanvasDabMode::Material);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
      SetBrushMode(LsCanvasDabMode::Erase);
    }
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
      int overlay = static_cast<int>(sceneViewport->SceneOverlay());
      ImGui::SetNextItemWidth(buttonSize.x);
      if (ImGui::Combo("##SceneOverlay", &overlay, "Shaded\0Outlines\0Both\0")) {
        sceneViewport->SceneOverlay() = static_cast<LsSceneOverlay>(overlay);
      }
      ImGui::SetItemTooltip("Show the scene shaded, as outlines of its shapes, or both");
      ImGui::SetNextItemWidth(buttonSize.x);
      ImGui::SliderFloat("##SceneOpacity", &sceneViewport->SceneOverCanvasOpacity(), 0.0f, 1.0f, "%.2f");
      ImGui::SetItemTooltip("Opacity of the scene over the canvas");
    }

    ImGui::End();
    ImGui::PopStyleVar();
  }

  // Fills the window with a dock space whose centre lets input through to the canvas
  // or holds the 3D view, and keeps the Scene, Properties, Brush, View and BRDF panels docked to its right side
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
      ImGuiID sceneId = ImGui::DockBuilderSplitNode(rightId, ImGuiDir_Up, 0.3f, nullptr, &propertiesId);
      ImGuiID brushId = ImGui::DockBuilderSplitNode(propertiesId, ImGuiDir_Down, 0.65f, nullptr, &propertiesId);
      ImGuiID viewId = ImGui::DockBuilderSplitNode(brushId, ImGuiDir_Down, 0.76f, nullptr, &brushId);
      ImGuiID brdfId = ImGui::DockBuilderSplitNode(viewId, ImGuiDir_Down, 0.5f, nullptr, &viewId);
      ImGui::DockBuilderDockWindow("Scene", sceneId);
      ImGui::DockBuilderDockWindow("Properties", propertiesId);
      ImGui::DockBuilderDockWindow("Brush", brushId);
      ImGui::DockBuilderDockWindow("View", viewId);
      ImGui::DockBuilderDockWindow("BRDF", brdfId);
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
