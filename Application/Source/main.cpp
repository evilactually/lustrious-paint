#include <windows.h>
#include <string>
#include <iostream>
#include <memory>
#include <destructor.h>
#include <LsWin32MainWindow.h>
#include <LsRenderer.h>
#include <LsVulkanLoader.h>
#include <DWINTAB.h>
#include <LsWintabLoader.h>
#include <LsBrushRig.h>
#include <LsPointGrid.h>
#include <LsUselessBox.h>
#include <LsStuffingDemo.h>
#include <LsConsole.h>
#include <LsError.h>
#include <LsImage.h>

#define _USE_MATH_DEFINES
#include <math.h>
// #include <Test.h>

#include <LsIsosphere.h>
#include <LsTetrahedronMesh.h>
#include <LsIsosurfaceStuffer.h>

#define PACKETDATA (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE 0
#include "wintab/PKTDEF.h"

using namespace lslib;

HCTX OpenWintabContext(HINSTANCE hInstance, HWND hWindow)
{
  AXIS            dvcX, dvcY;         // The maximum tablet size
  LOGCONTEXT      lcMine = {};        // The context of the tablet
  WTInfoA(WTI_DEFSYSCTX, 0, &lcMine); // get system logical context
  // Modify the digitizing region
  wsprintf(lcMine.lcName, "Lustrious Paint Digitizing %x", hInstance);
  lcMine.lcOptions |= CXO_MESSAGES;   // recieve WM_PACKET
  lcMine.lcOptions |= CXO_SYSTEM;     // receive WM_MOUSEMOVE
  lcMine.lcPktData = PACKETDATA;
  lcMine.lcPktMode = PACKETMODE;
  lcMine.lcMoveMask = PACKETDATA;
  lcMine.lcBtnUpMask = lcMine.lcBtnDnMask;
  // Set the entire tablet as active
  WTInfoA(WTI_DEVICES,DVC_X, &dvcX);
  WTInfoA(WTI_DEVICES,DVC_Y, &dvcY);
  lcMine.lcInOrgX = 0;
  lcMine.lcInOrgY = 0;
  lcMine.lcInExtX = dvcX.axMax;
  lcMine.lcInExtY = dvcY.axMax;
  return WTOpenA(hWindow, &lcMine, TRUE);
}

class Application: LsFWin32MessageHandler {
  HINSTANCE hInstance;
  LsWin32MainWindow* window;
  std::shared_ptr<LsBrushRig> brushRig;
  std::shared_ptr<LsPointGrid> pointGrid;
  std::shared_ptr<LsUselessBox> uselessBox;
  std::shared_ptr<LsStuffingDemo> stuffingDemo;
  std::shared_ptr<LsRenderer> renderer;
  HCTX tabletContext;
  POINT cursor;

  float radar_x = 600.0f;
  float radar_y = 210.0f;

  float point_x, point_z;

public:
  Application() {
    LsCloseConsole();
    LsUnloadWintabLibrary();
  }

  ~Application() {}

  void Initialize(HINSTANCE hInstance) {
    // Attach console
    LsOpenConsole();

    // Get a pointer to main window singleton
    window = LsWin32MainWindow::Get();

    // Load Wintab
    LsLoadWintabLibrary();
    LsLoadWintabEntryPoints();
   
    window->Create(hInstance, "Lustrious Paint", 100, 100, 800, 300);
    LsSetDialogParentWindow(window->GetWindowHandle());

    // Open Wintab Context
    OpenWintabContext(hInstance, window->GetWindowHandle());

    window->HideMouse();

    // Initialize the rendering system
    renderer = std::make_shared<LsRenderer>(hInstance, window->GetWindowHandle());
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

  void GetPosition(float* x, float* z) {
      (*x) = (float)(cursor.x - radar_x);
      (*z) = (float)(cursor.y - radar_y);
  }

  void DrawRect(float x, float y, float width, float height){
    renderer->DrawLine(x, y, x + width, y);
    renderer->DrawLine(x, y, x, y + height);
    renderer->DrawLine(x, y + height, x + width, y + height);
    renderer->DrawLine(x + width, y, x + width, y + height);
  }

  void DrawArc(float x, float y, float radius, float start, float length) {
      #define ARC_STEPS_PER_RADIAN 10
      const int steps = length * ARC_STEPS_PER_RADIAN;
      const float radians_per_step = 1.0 / (float)ARC_STEPS_PER_RADIAN;

      float current_angle = start;
      float x1 = x + radius * cos(start);
      float y1 = y - radius * sin(start);
      float x2, y2;
           
      for (size_t i = 0; i < steps; i++)
      {
          current_angle += radians_per_step;
          x2 = x + radius * cos(current_angle);
          y2 = y - radius * sin(current_angle);

          renderer->DrawLine(x1, y1, x2, y2);
          swap(x1, x2);
          swap(y1, y2);
      }

      x2 = x + radius * cos(start + length);
      y2 = y - radius * sin(start + length);
      renderer->DrawLine(x1, y1, x2, y2);
  }

  void DrawPolarGrid(float x, float y, float start_angle, float end_angle, float distance_start, float distance_step, int distance_grid_line_count) {
      for (size_t i = 0; i < distance_grid_line_count; i++)
      {
          //DrawArc(x, y, start_angle, end_angle - start_angle, distance_start + distance_step*i);
          DrawArc(x, y, distance_start + i*distance_step, start_angle, end_angle - start_angle);
      }
  }

  void DrawCross(float x, float y, float size) {
      renderer->DrawLine(x-size/2.0, y, x+size/2.0, y);
      renderer->DrawLine(x, y - size / 2.0, x, y + size / 2.0);
  }

  void Run() {
    window->Show();
    while ( window->ProcessMessages() ) {
      window->WaitForMessages();
      renderer->BeginFrame();
      
      //pointGrid->Render();
      //renderer->DrawCanvas();
      renderer->Clear(1.0f,1.0f,1.0f);
      //brushRig->Render();

      //stuffingDemo->Render();
      renderer->SetLineWidth(2.0f);
      renderer->SetColor(0,0,0);
      DrawRect(10,10,300,200);

      renderer->SetLineWidth(2.0f);
      DrawPolarGrid(radar_x, radar_y, 0.0f, M_PI, 10.0, 20.0f, 10);

      renderer->SetColor(215, 38, 56);
      DrawCross(cursor.x, cursor.y, 10.0f);

      //DrawArc(400, 200, 40.0, 0.0f, M_PI-0.5f);
      ////DrawArc(400, 200, 20.0, 0.0f, M_PI);
      //DrawArc(400, 200, 30.0, 0.0f, M_PI);
      ////DrawArc(400, 200, 40.0, 0.0f, M_PI);
      //DrawArc(400, 200, 50.0, 0.0f, M_PI);
      ////DrawArc(400, 200, 60.0, 0.0f, M_PI);
      //DrawArc(400, 200, 70.0, 0.0f, M_PI);
      ////DrawArc(400, 200, 80.0, 0.0f, M_PI);
      //DrawArc(400, 200, 90.0, 0.0f, M_PI);

      
      renderer->EndFrame();
    };


  }

  void Application::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    switch (uMsg) {
      case WM_SIZE:
	    renderer->RefreshSwapChain();
      break;
      case WM_MOUSEMOVE:
        cursor = LsWin32MainWindow::Get()->GetMousePosition();
        point_x = (float)(cursor.x - radar_x);
        point_z = -(float)(cursor.y - radar_y);
        std::cout << point_x << " " << point_z << std::endl;
      break;
      case WM_KEYDOWN:
#ifdef GIF_RECORDING
        if (wParam == VK_F12) {
          renderer->StartGIFRecording("C:\\Users\\Public\\Downloads\\out.gif");
          std::cout << "PRESSED" << std::endl;
        }
        else if (wParam == VK_F11) {
          renderer->StopGIFRecording();
        }
#endif
      break;
    }
  }
};

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  //LsIsosphere sphere({0.0f, 0.0f, 0.0f}, 1.0f);
  //LsTetrahedronMesh mesh;
  //LsIsosurfaceStuffer stuffer;
  //stuffer.Stuff(mesh, sphere);
  try {
    Application application;
    application.Initialize(hInstance);// test();
    application.Run();
  } catch (std::string m) {
    LsErrorMessage(m, "Error");
  }

  //system("pause");
}