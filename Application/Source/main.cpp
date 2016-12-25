#include <windows.h>
#include <string>
#include <iostream>

#include <destructor.h>
#include <LsWin32MainWindow.h>
#include <LsRenderer.h>
#include <LsVulkanLoader.h>
#include <DWINTAB.h>
#include <LsWintabLoader.h>
#include <LsBrushRig.h>
#include <LsPointGrid.h>
#include <LsUselessBox.h>
#include <LsConsole.h>
#include <LsError.h>
#include <Test.h>

#define PACKETDATA (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE 0
#include "wintab/PKTDEF.h"

using namespace lslib;

HCTX OpenWintabContext(HINSTANCE hInstance, HWND hWindow)
{
  AXIS            dvcX, dvcY;         // The maximum tablet size
  LOGCONTEXT      lcMine;             // The context of the tablet
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

class Application {
  HINSTANCE hInstance;
  LsWin32MainWindow* window;
  LsBrushRig brushRig;
  LsPointGrid pointGrid;
  LsUselessBox uselessBox;
  LsRenderer* renderer;
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
   
    window->Create(hInstance, "Lustrious Paint", 100, 100, 1024, 640);
    LsSetDialogParentWindow(window->GetWindowHandle());

    // Open Wintab Context
    OpenWintabContext(hInstance, window->GetWindowHandle());

    window->HideMouse();

    // Initialize the rendering system
    LsRenderer::Initialize(hInstance, window->GetWindowHandle());

    renderer = LsRenderer::Get();

    pointGrid.SetBackgroundColor(0.1f, 0.0f, 0.1f);
  }

  void Run() {
    window->Show();
    while( window->ProcessNextMessage() ){
      window->WaitForMessages();
      renderer->BeginFrame();
      pointGrid.Render();
      brushRig.Render();
      uselessBox.Render();
      renderer->EndFrame();
    };
  }
};

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  try {
    Application application;
    application.Initialize(hInstance); test();
    application.Run();
  }
  catch (std::string m) {
    LsErrorMessage(m, "Error");
  }
}