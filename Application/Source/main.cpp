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
#include <LsLightingPanel.h>
#include <LsCanvasTool.h>
#include <LsDocument.h>
#include <mutex>
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
  std::unique_ptr<LsLightingPanel> lightingPanel;
  std::unique_ptr<LsCanvasTool> canvasTool;
  // Top left of the area the canvas and the 3D view share, for the toolbar
  ImVec2 canvasAreaMin = ImVec2(0.0f, 0.0f);
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

  // The file the painting was last saved to, empty before the first save
  std::string documentPath;
  // A save waiting for the canvas attributes to be read back from the GPU
  std::string pendingSavePath;
  // The open and save dialogs answer from another thread
  std::mutex dialogMutex;
  // Whether the dialog shown last opens a file rather than saves one
  bool dialogOpens = false;
  bool dialogAnswered = false;
  std::string dialogPath;

public:
  Application() {
    LsCloseConsole();
  }

  ~Application() {
    // Vulkan objects must be released before the window and SDL itself
    sceneViewport.reset();
    canvasTool.reset();
    stuffingDemo.reset();
    lightingPanel.reset();
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
    lightingPanel = std::make_unique<LsLightingPanel>(canvas, canvasPainter);
    //stuffingDemo = std::make_shared<LsStuffingDemo>(renderer);

    const glm::vec3 canvasBackground(0.1f, 0.0f, 0.1f);
    pointGrid->SetBackgroundColor(canvasBackground.r, canvasBackground.g, canvasBackground.b);

    scene = std::make_shared<LsScene>();
    CreateStarterScene();
    // Undo starts from the starter scene, not from nothing
    history = std::make_unique<LsSceneHistory>(scene);
    sceneEditor = std::make_unique<LsSceneEditor>(scene);
    sceneViewport = std::make_unique<LsSceneViewport>(renderer, scene, sceneEditor.get());
    canvasTool = std::make_unique<LsCanvasTool>(renderer, scene, canvas);
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
      // A save waits a frame for the canvas attributes
      if ( sceneViewport->IsAnimating() || canvas->IsRendering() || !pendingSavePath.empty() ) {
        idleFrames = 0;
      }
      if ( !window->ProcessMessages() ) {
        break;
      }

      renderer->BeginFrame();
      HandleUndoShortcuts();
      HandleFileShortcuts();

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

      // Before the dock space, which fills what the menu bar leaves
      DrawMenuBar();
      DrawDockedPanels();
      DrawToolbar();
      HandleToolShortcuts();
      HandleViewToggle(overUi);
      // Before the canvas renders, so a placement's depth pick goes into this frame
      if (!sceneViewport->IsShowingScene()) {
        if (!ImGui::GetIO().WantTextInput) {
          canvasTool->HandleShortcuts();
        }
        canvasTool->Update(!overUi && !navigating && !canvasPainter->IsPainting());
      }
      // Presses on the tool's gizmo, or placing it, don't paint
      canvasPainter->SetMouseClaimed(canvasTool->WantsMouse());

      // Strokes painted since the last frame, before anything draws the canvas
      UpdateCanvas();
      // After the canvas has rendered, so attributes a save asked for are back
      ProcessSaving();

      // After laying out the UI, which decides whether the canvas or the 3D view
      // shows, and before the UI is rendered over everything at the end of the frame
      pointGrid->Render();
      if (!sceneViewport->IsShowingScene()) {
        renderer->DrawCanvas();
        if (!overUi && !navigating && !canvasTool->WantsMouse()) {
          brushRig->Render();
          DrawBrushOutline();
        }
        canvasPainter->Render();
      }
      //stuffingDemo->Render();

      if (ImGui::IsKeyPressed(ImGuiKey_F9, false)) {
        char path[64];
        snprintf(path, sizeof(path), "Screenshot %llu.bmp", static_cast<unsigned long long>(SDL_GetTicks()));
        renderer->SaveScreenshot(path);
      }
#ifdef FRAME_RECORDING
      if (ImGui::IsKeyPressed(ImGuiKey_F12, false)) {
        char basePath[64];
        snprintf(basePath, sizeof(basePath), "Recording %llu", static_cast<unsigned long long>(SDL_GetTicks()));
        renderer->StartFrameRecording(basePath);
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_F11, false)) {
        renderer->StopFrameRecording();
      }
#endif

      // Whatever the UI changed this frame becomes an undo step once the edit is over
      history->Update(ImGui::IsAnyItemActive() || sceneViewport->IsEditing());

      if (ImGui::Begin("Brush", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // What a stroke paints: depth and normals from the construction geometry,
        // the BRDF panel's material, the Lighting panel's light, or erasing all
        // of them. Two rows, so they fit the panel.
        struct { const char* label; LsCanvasDabMode mode; const char* tooltip; } modes[] = {
          { "Depth", LsCanvasDabMode::Depth, "Paint positions from the construction geometry (B)" },
          { "Normals", LsCanvasDabMode::Normal,
            "Paint normals from the construction geometry, or the Lighting panel's normal" },
          { "Material", LsCanvasDabMode::Material, "Paint the BRDF panel's material (M)" },
          { "Light", LsCanvasDabMode::Light, "Paint the light selected in the Lighting panel (L)" },
          { "Erase", LsCanvasDabMode::Erase,
            "Remove painted positions, normals and lights, back to the default material (N)" },
          { "Sculpt", LsCanvasDabMode::Sculpt,
            "Pull painted positions towards the camera or push them away, dabs build up (S)" },
        };
        for (const auto& mode : modes) {
          if (&mode != modes && &mode != modes + 3) {
            ImGui::SameLine();
          }
          if (ImGui::RadioButton(mode.label, canvasPainter->GetMode() == mode.mode)) {
            SetBrushMode(mode.mode);
          }
          ImGui::SetItemTooltip("%s", mode.tooltip);
        }
        float brushRadius = canvasPainter->GetBrushRadius();
        if (ImGui::SliderFloat("Radius", &brushRadius, 1.0f, 100.0f, "%.0f px", ImGuiSliderFlags_AlwaysClamp)) {
          canvasPainter->SetBrushRadius(brushRadius);
        }
        float brushOpacity = canvasPainter->GetBrushOpacity();
        if (ImGui::SliderFloat("Opacity", &brushOpacity, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
          canvasPainter->SetBrushOpacity(brushOpacity);
        }
        ImGui::SetItemTooltip("How far a stroke blends towards what it paints, overlapping dabs don't add up past it");
        float brushFalloff = canvasPainter->GetBrushFalloff();
        if (ImGui::SliderFloat("Falloff", &brushFalloff, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
          canvasPainter->SetBrushFalloff(brushFalloff);
        }
        ImGui::SetItemTooltip("How much of the radius the brush fades out over, 0 for a hard edge");
        if (canvasPainter->GetMode() == LsCanvasDabMode::Sculpt) {
          bool push = canvasPainter->GetSculptPush();
          if (ImGui::RadioButton("Pull", !push)) {
            canvasPainter->SetSculptPush(false);
          }
          ImGui::SetItemTooltip("Raise the surface towards the camera (X switches)");
          ImGui::SameLine();
          if (ImGui::RadioButton("Push", push)) {
            canvasPainter->SetSculptPush(true);
          }
          ImGui::SetItemTooltip("Sink the surface away from the camera (X switches)");
          ImGui::SameLine();
          bool alongNormal = canvasPainter->GetSculptAlongNormal();
          if (ImGui::Checkbox("Along normal", &alongNormal)) {
            canvasPainter->SetSculptAlongNormal(alongNormal);
          }
          ImGui::SetItemTooltip("Move the surface along its normal, as at the stroke's start,\n"
                                "otherwise along the camera rays. Shift smooths.");
        }
        else if (canvasPainter->GetMode() == LsCanvasDabMode::Light) {
          ImGui::TextDisabled("Hold Shift to paint no rig");
          ImGui::SetItemTooltip("Leaves only the scene's lights on the pixels");
        }
        else {
          ImGui::TextDisabled("Hold Shift to blend");
          ImGui::SetItemTooltip("Smooths what the brush paints instead, or everything while erasing.\n"
                                "Opacity sets how strongly each dab smooths.");
        }
        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - ImGui::CalcTextSize("Fill").x -
                        ImGui::GetStyle().FramePadding.x*2.0f);
        if (ImGui::Button("Fill")) {
          canvasPainter->Fill(ImGui::GetIO().KeyShift);
        }
        ImGui::SetItemTooltip("Paint the whole canvas with the brush, as if painted over everywhere (F).\n"
                              "In Light mode Shift fills with no rig.");
        // Light strokes don't blend
        if (canvasPainter->GetMode() != LsCanvasDabMode::Light) {
          DrawKeepEdges();
        }
      }
      ImGui::End();


      if (ImGui::Begin("View", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // What the canvas shows of the painted attributes
        int display = static_cast<int>(canvas->GetDisplay());
        if (ImGui::Combo("Display", &display, "Depth\0Position\0Normal\0Rendering\0Material\0Rigs\0Matcap\0")) {
          canvas->SetDisplay(static_cast<LsCanvasDisplay>(display));
        }
        ImGui::SetItemTooltip("Rendering lights the painted points with the scene's emitters, shaded with the BRDF panel");
        bool rendering = canvas->GetDisplay() == LsCanvasDisplay::Rendering;
        if (rendering) {
          DrawRenderSettings();
        }
        // The rendering and the matcap are shown without isolines
        bool shaded = rendering || canvas->GetDisplay() == LsCanvasDisplay::Matcap;
        bool isolines = canvas->GetIsolines();
        if (!shaded && ImGui::Checkbox("Isolines", &isolines)) {
          canvas->SetIsolines(isolines);
        }
        if (!shaded) {
          ImGui::SetItemTooltip("Contour lines of the depth, every tenth one brighter");
        }
        if (!shaded && isolines) {
          float spacing = canvas->GetIsolineSpacing();
          if (ImGui::DragFloat("Spacing", &spacing, 0.005f, 0.001f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
            canvas->SetIsolineSpacing(spacing);
          }
          ImGui::SetItemTooltip("Depth between isolines, in scene units");
        }
      }
      ImGui::End();

      if (ImGui::Begin("Lighting", nullptr, ImGuiWindowFlags_NoCollapse)) {
        LsNodeId cameraNode = scene->GetActiveCamera();
        if (cameraNode != kLsInvalidNode) {
          glm::vec2 size = renderer->GetCanvasSize();
          LsCamera camera = scene->GetCamera(cameraNode, size.x/size.y);
          lightingPanel->Draw(&camera);
        }
        else {
          lightingPanel->Draw(nullptr);
        }
      }
      ImGui::End();

      if (ImGui::Begin("BRDF", nullptr, ImGuiWindowFlags_NoCollapse)) {
        DrawBrdf();
      }
      ImGui::End();

      if (ImGui::Begin("Tool", nullptr, ImGuiWindowFlags_NoCollapse)) {
        canvasTool->DrawPanel();
      }
      ImGui::End();

      renderer->EndFrame();
    }


  }

  // Whether blending leaves out neighbours on other surfaces, a checkbox and the
  // threshold on one row, the slider only matters while it is checked
  void DrawKeepEdges() {
    bool keepEdges = canvasPainter->GetBlendKeepsEdges();
    if (ImGui::Checkbox("##KeepEdges", &keepEdges)) {
      canvasPainter->SetBlendKeepsEdges(keepEdges);
    }
    ImGui::SetItemTooltip("Blend only with neighbours on the same surface");
    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemInnerSpacing.x);
    ImGui::BeginDisabled(!keepEdges);
    float edgePercent = canvasPainter->GetBlendEdgeThreshold()*100.0f;
    if (ImGui::SliderFloat("Keep edges", &edgePercent, 1.0f, 50.0f, "%.0f%% depth", ImGuiSliderFlags_AlwaysClamp)) {
      canvasPainter->SetBlendEdgeThreshold(edgePercent/100.0f);
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Neighbours further in front or behind than this share of the depth\n"
                          "are on another surface and aren't blended in");
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
    changed |= ImGui::SliderFloat("Metallic", &brdf.metallic, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Blends from a dielectric coat over a diffuse base to a bare metal");
    changed |= ImGui::SliderFloat("Roughness", &brdf.roughness, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Perceptual roughness of the microfacets, alpha is its square");
    changed |= ImGui::SliderFloat("IOR", &brdf.ior, 1.0f, 3.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Index of refraction of the dielectric coat, sets how strongly it reflects");
    if (changed) {
      canvas->SetBrushBrdf(brdf);
    }
    if (canvasPainter->GetMode() != LsCanvasDabMode::Material && ImGui::Button("Paint material")) {
      SetBrushMode(LsCanvasDabMode::Material);
    }

    // Every BRDF painted so far, to pick it again
    const std::vector<LsBrdfParameters>& materials = canvas->GetPaintedBrdfs();
    ImGui::SeparatorText("Painted");
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
      ImGui::SetItemTooltip("Metallic %.2f, roughness %.2f, IOR %.2f", material.metallic, material.roughness, material.ior);
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
    LsScenePrimitive tool;
    canvas->SetTool(canvasTool->GetPrimitive(tool) ? &tool : nullptr);
    LsNodeId camera = scene->GetActiveCamera();
    if (camera != kLsInvalidNode) {
      glm::vec2 size = renderer->GetCanvasSize();
      canvas->SetCamera(scene->GetCamera(camera, size.x/size.y));
    }
    else {
      canvas->ClearCamera();
    }
    canvas->Render();
    // A normal picked from the canvas becomes the brush normal, and the rig
    // under it the one light strokes paint
    glm::vec3 picked;
    if (canvas->TakePickedNormal(picked)) {
      canvasPainter->SetNormal(picked);
    }
    uint32_t pickedRig;
    if (canvas->TakePickedRig(pickedRig)) {
      lightingPanel->SelectRig(pickedRig);
    }
  }

  // The display stays as chosen in the View panel, whatever the brush paints
  void SetBrushMode(LsCanvasDabMode mode) {
    canvasPainter->SetMode(mode);
  }

  // B paints depth, M materials, L lights and N erases. Normals have no key,
  // they are painted seldom, and W, E and R belong to the tool's gizmo, see
  // LsCanvasTool. F fills the canvas with the brush.
  void HandleToolShortcuts() {
    if (ImGui::GetIO().WantTextInput || sceneViewport->IsShowingScene()) {
      return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_B, false)) {
      SetBrushMode(LsCanvasDabMode::Depth);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_M, false)) {
      SetBrushMode(LsCanvasDabMode::Material);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_L, false)) {
      SetBrushMode(LsCanvasDabMode::Light);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
      canvasPainter->Fill(ImGui::GetIO().KeyShift);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_N, false)) {
      SetBrushMode(LsCanvasDabMode::Erase);
    }
    // Not with Ctrl, which saves
    else if (ImGui::IsKeyPressed(ImGuiKey_S, false) && !ImGui::GetIO().KeyCtrl) {
      SetBrushMode(LsCanvasDabMode::Sculpt);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_X, false) && canvasPainter->GetMode() == LsCanvasDabMode::Sculpt) {
      canvasPainter->SetSculptPush(!canvasPainter->GetSculptPush());
    }

    // As in Photoshop: [ and ] change the size, with Shift the falloff, the
    // digits set the opacity in tens with 0 for full
    bool shift = ImGui::GetIO().KeyShift;
    float step = 0;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, true)) {
      step = -1.0f;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, true)) {
      step = 1.0f;
    }
    if (step != 0.0f && shift) {
      // Shift+[ softer, Shift+] harder
      canvasPainter->SetBrushFalloff(glm::clamp(canvasPainter->GetBrushFalloff() - step*0.1f, 0.0f, 1.0f));
    }
    else if (step != 0.0f) {
      // In proportion, so small brushes change finely and large ones quickly
      float radius = canvasPainter->GetBrushRadius()*(step > 0.0f ? 1.15f : 1.0f/1.15f);
      canvasPainter->SetBrushRadius(glm::clamp(radius, 1.0f, 100.0f));
    }
    for (int digit = 0; digit <= 9; digit++) {
      if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_0 + digit), false)) {
        canvasPainter->SetBrushOpacity(digit == 0 ? 1.0f : 0.1f*static_cast<float>(digit));
      }
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

  // The circle the brush covers around the mouse, and a faint one where its
  // falloff starts. Dark under light, so it shows over any part of the canvas.
  void DrawBrushOutline() {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImVec2 center = ImGui::GetIO().MousePos;
    // The brush radius is in window pixels, the UI draws in points
    float radius = canvasPainter->GetBrushRadius()/ImGui::GetIO().DisplayFramebufferScale.x;
    drawList->AddCircle(center, radius, IM_COL32(0, 0, 0, 140), 0, 3.0f);
    drawList->AddCircle(center, radius, IM_COL32(255, 255, 255, 220), 0, 1.0f);
    float falloff = canvasPainter->GetBrushFalloff();
    if (falloff > 0.02f && falloff < 0.98f) {
      drawList->AddCircle(center, radius*(1.0f - falloff), IM_COL32(255, 255, 255, 70), 0, 1.0f);
    }
  }

  void DrawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open...", "Ctrl+O")) {
          Open();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
          Save();
        }
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
          SaveAs();
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
  }

  void HandleFileShortcuts() {
    if (ImGui::GetIO().WantTextInput) {
      return;
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)) {
      SaveAs();
    }
    else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) {
      Save();
    }
    else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_O)) {
      Open();
    }
  }

  void Open() {
    static const SDL_DialogFileFilter filter = { "Lustrious Paint", kLsDocumentExtension };
    dialogOpens = true;
    SDL_ShowOpenFileDialog(&Application::OnFileDialog, this, window->GetWindowHandle(), &filter, 1,
                           documentPath.empty() ? nullptr : documentPath.c_str(), false);
  }

  // Replaces the scene, the canvas and its lighting with the document's. The
  // undo history starts over from it.
  void OpenDocument(const std::string& path) {
    LsDocument document;
    std::string error;
    if (!LsLoadDocument(path, document, error)) {
      LsErrorMessage(error, "Could not open");
      return;
    }
    glm::uvec2 size = canvas->GetSize();
    if (document.attributes.width != size.x || document.attributes.height != size.y) {
      LsErrorMessage(path + " has a " + std::to_string(document.attributes.width) + " by " +
                     std::to_string(document.attributes.height) + " canvas, this one is " +
                     std::to_string(size.x) + " by " + std::to_string(size.y) + ".", "Could not open");
      return;
    }
    scene->Restore(document.scene);
    history = std::make_unique<LsSceneHistory>(scene);
    // Ids of the old scene can name other nodes now
    sceneEditor->SetSelected(kLsInvalidNode);
    canvas->ClearLighting();
    for (const auto& light : document.lights) {
      canvas->SetLocalLightAt(light.first, light.second);
    }
    for (const auto& occluder : document.occluders) {
      canvas->SetOccluderAt(occluder.first, occluder.second);
    }
    for (const auto& rig : document.rigs) {
      canvas->SetRigAt(rig.first, rig.second);
    }
    canvas->SetBrushBrdf(document.brushBrdf);
    canvas->SetPaintedBrdfs(document.paintedBrdfs);
    canvas->SetRenderSettings(document.renderSettings);
    canvas->SetAttributes(document.attributes);
    lightingPanel->SelectRig(0);
    SetDocumentPath(path);
  }

  // The file name in the title, without its folders
  void SetDocumentPath(const std::string& path) {
    documentPath = path;
    size_t slash = documentPath.find_last_of("/\\");
    std::string title = "Lustrious Paint - " + (slash == std::string::npos ? documentPath : documentPath.substr(slash + 1));
    SDL_SetWindowTitle(window->GetWindowHandle(), title.c_str());
  }

  // To the file saved to before, or a new one
  void Save() {
    if (documentPath.empty()) {
      SaveAs();
    }
    else {
      StartSave(documentPath);
    }
  }

  void SaveAs() {
    static const SDL_DialogFileFilter filter = { "Lustrious Paint", kLsDocumentExtension };
    dialogOpens = false;
    SDL_ShowSaveFileDialog(&Application::OnFileDialog, this, window->GetWindowHandle(), &filter, 1,
                           documentPath.empty() ? nullptr : documentPath.c_str());
  }

  // Called by SDL, possibly on another thread, with no file when cancelled
  static void SDLCALL OnFileDialog(void* userdata, const char* const* files, int) {
    Application* application = static_cast<Application*>(userdata);
    {
      std::lock_guard<std::mutex> lock(application->dialogMutex);
      application->dialogAnswered = true;
      application->dialogPath = files && files[0] ? files[0] : "";
    }
    // Wakes the main loop, which may be waiting for events
    SDL_Event event = {};
    event.type = SDL_EVENT_USER;
    SDL_PushEvent(&event);
  }

  // The canvas attributes are read back from the GPU first, see ProcessSaving
  void StartSave(const std::string& path) {
    pendingSavePath = path;
    canvas->RequestAttributes();
  }

  // Opens the file the open dialog chose, or starts the save the save dialog
  // chose and writes the file once the canvas attributes are back
  void ProcessSaving() {
    std::string chosen;
    {
      std::lock_guard<std::mutex> lock(dialogMutex);
      if (dialogAnswered) {
        chosen = dialogPath;
        dialogAnswered = false;
      }
    }
    if (!chosen.empty() && dialogOpens) {
      OpenDocument(chosen);
    }
    else if (!chosen.empty()) {
      std::string extension = std::string(".") + kLsDocumentExtension;
      if (chosen.size() < extension.size() || chosen.compare(chosen.size() - extension.size(), extension.size(), extension) != 0) {
        chosen += extension;
      }
      StartSave(chosen);
    }

    LsCanvasAttributes attributes;
    if (pendingSavePath.empty() || !canvas->TakeAttributes(attributes)) {
      return;
    }
    std::string error;
    if (LsSaveDocument(pendingSavePath, *scene, *canvas, attributes, error)) {
      SetDocumentPath(pendingSavePath);
    }
    else {
      LsErrorMessage(error, "Could not save");
    }
    pendingSavePath.clear();
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

    // In the corner of the canvas area, clear of the docked panels on either side
    ImGui::SetNextWindowPos(ImVec2(canvasAreaMin.x + 12.0f, canvasAreaMin.y + 12.0f), ImGuiCond_Always);

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
      // Lighting on the left, so the sphere stays in view while painting, with the Tool panel under it
      ImGuiID lightingId = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, nullptr, nullptr);
      ImGuiID toolId = ImGui::DockBuilderSplitNode(lightingId, ImGuiDir_Down, 0.3f, nullptr, &lightingId);
      ImGui::DockBuilderDockWindow("Lighting", lightingId);
      ImGui::DockBuilderDockWindow("Tool", toolId);
      ImGuiID propertiesId;
      ImGuiID sceneId = ImGui::DockBuilderSplitNode(rightId, ImGuiDir_Up, 0.3f, nullptr, &propertiesId);
      ImGuiID brushId = ImGui::DockBuilderSplitNode(propertiesId, ImGuiDir_Down, 0.62f, nullptr, &propertiesId);
      ImGuiID viewId = ImGui::DockBuilderSplitNode(brushId, ImGuiDir_Down, 0.42f, nullptr, &brushId);
      ImGui::DockBuilderDockWindow("Scene", sceneId);
      // Tabs, the scene's properties while modelling and the BRDF while painting.
      // The first docked is the one shown.
      ImGui::DockBuilderDockWindow("BRDF", propertiesId);
      ImGui::DockBuilderDockWindow("Properties", propertiesId);
      ImGui::DockBuilderDockWindow("Brush", brushId);
      ImGui::DockBuilderDockWindow("View", viewId);
      ImGui::DockBuilderFinish(dockspaceId);
      dockLayoutBuilt = true;
    }

    sceneEditor->DrawOutliner();
    // The canvas and the 3D view take turns in the central node
    if (ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockspaceId)) {
      glm::vec2 areaMin(central->Pos.x, central->Pos.y);
      canvasAreaMin = central->Pos;
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

#ifdef _WIN32
  system("pause");
#endif
  return 0;
}
