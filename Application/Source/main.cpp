#include <destructor.h>

#include <windows.h>
#include <string>
#include <iostream>
#include <LsWin32MainWindow.h>
#include <LsRenderer.h>
#include <LsVulkanLoader.h>
#include <LsWintabLoader.h>
#include <LsBrushRig.h>
#include <LsConsole.h>
#include <LsError.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

  try {
    // Get a pointer to main window singleton
    LsWin32MainWindow* window = LsWin32MainWindow::Get();

    // Attach console for standard output
    LsOpenConsole();

    // Load Wintab
    LsLoadWintabLibrary();
    LsLoadWintabEntryPoints();

    LsBrushRig brushRig;

    window->Create(hInstance, "Lustrious Paint", 100, 100, 1024, 640);
    LsSetDialogParentWindow(window->GetWindowHandle());

    // Initialize the rendering system
    LsRenderer::Initialize(hInstance, window->GetWindowHandle());
    
    window->Show();
    while( window->ProcessMessages() ){
      window->WaitForMessages();
    };

  }
  catch (std::string m) {
    LsErrorMessage(m, "Error");
  }

  LsCloseConsole();
}