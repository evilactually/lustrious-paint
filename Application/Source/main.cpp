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
   
    window->Create(hInstance, "Lustrious Paint", 100, 100, 640, 800);
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

  void Application::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
      case WM_SIZE:
	    renderer->RefreshSwapChain();
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

  system("pause");
}