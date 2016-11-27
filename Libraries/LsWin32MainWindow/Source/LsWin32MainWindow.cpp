#include <windows.h>
#include <assert.h>
#include <string>
#include <LsWin32MainWindow.h>
#include <LsFWin32MessageHandler.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return LsWin32MainWindow::Get()->HandleMessage(hwnd, uMsg, wParam, lParam);
}

void LsWin32MainWindow::Create(HINSTANCE hInstance, std::string title, int x, int y, int width, int height) {
  WNDCLASSEX windowClass = {};
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  windowClass.lpfnWndProc = WndProc;
  windowClass.hInstance = hInstance;
  windowClass.lpszClassName = "LsMainWindow";
  windowClass.hCursor = NULL;
  ::RegisterClassEx(&windowClass);

  // Calculate windows size with requested client area
  RECT windowRegion;
  windowRegion.left = x;
  windowRegion.right = x + width;
  windowRegion.top = y;
  windowRegion.bottom = y + height;

  ::AdjustWindowRect(&windowRegion, WS_OVERLAPPEDWINDOW | WS_VISIBLE, false);

  windowHandle = NULL;
  HWND handle = ::CreateWindowEx( WS_EX_APPWINDOW,
                                  "LsMainWindow",
                                  title.c_str(),
                                  WS_OVERLAPPEDWINDOW, // | WS_VISIBLE,
                                  windowRegion.left,
                                  windowRegion.top,
                                  windowRegion.right - windowRegion.left,
                                  windowRegion.bottom - windowRegion.top,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  NULL );

  // Sanity check, make sure windowHandle was set inside WndProc to correct value
  assert(handle == windowHandle);
}

LsWin32MainWindow* LsWin32MainWindow::Get() {
  return &window;
}

HWND LsWin32MainWindow::GetWindowHandle() {
  return windowHandle;
}

void LsWin32MainWindow::WaitForMessages() {
  ::WaitMessage();
}

bool LsWin32MainWindow::ProcessMessages() {
  while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
    if ( !::GetMessage( &msg, NULL, 0, 0 ) ) {
      return false;
    }
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  return true;
}

bool LsWin32MainWindow::ProcessNextMessage() {
  if ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
    if ( !::GetMessage( &msg, NULL, 0, 0 ) ) {
      return false;
    }
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  return true;
}

void LsWin32MainWindow::Destroy() {
  DestroyWindow(windowHandle);
}

void LsWin32MainWindow::Hide() {
  ShowWindow(windowHandle, SW_HIDE);
}

void LsWin32MainWindow::Show() {
  ShowWindow(windowHandle, SW_SHOW);
}

void LsWin32MainWindow::Minimize() {
  ShowWindow(windowHandle, SW_MINIMIZE);
}

void LsWin32MainWindow::Restore() {
  ShowWindow(windowHandle, SW_RESTORE);
}

void LsWin32MainWindow::Maximize() {
  ShowWindow(windowHandle, SW_MAXIMIZE);
}

void LsWin32MainWindow::GetClientArea(int* x, int* y, int* width, int* height) {
  RECT clientRect;
  ::GetClientRect(windowHandle, &clientRect);

  if (x) {
    *x = clientRect.left;
  }

  if (y) {
    *y = clientRect.top;
  }

  if (width) {
    *width = clientRect.right - clientRect.left;
  }

  if (height) {
    *height = clientRect.bottom - clientRect.top;
  }
}

LsWin32MainWindow LsWin32MainWindow::window;

LsWin32MainWindow::LsWin32MainWindow() {

}

LsWin32MainWindow::~LsWin32MainWindow() {
  
}

LRESULT LsWin32MainWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  // Set window handle as soon as possible
  if ( !windowHandle ) {
     LsWin32MainWindow::Get()->windowHandle = hwnd;
  }

  LsFWin32MessageHandler::BroadCastWin32Message(uMsg, wParam, lParam);

  switch( uMsg ) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
  }
 
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}