#pragma once

#include <windows.h>

class LsWin32MainWindow
{
friend LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
  void Create(HINSTANCE hInstance, std::string title, int x, int y, int width, int height);
  static LsWin32MainWindow* Get();
  HWND GetWindowHandle();
  void WaitForMessages();
  bool ProcessMessages();
  bool ProcessNextMessage();
  void Destroy();
  void Hide();
  void Show();
  void Minimize();
  void Restore();
  void Maximize();
  void ShowMouse();
  void HideMouse();
  void GetClientArea(int* x, int* y, int* width, int* height);
protected:
  static LsWin32MainWindow window;
  HWND windowHandle;
  MSG msg;
  HCURSOR arrowCursor;
  bool mouseVisible = true;
  LsWin32MainWindow();
  ~LsWin32MainWindow();
  LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};