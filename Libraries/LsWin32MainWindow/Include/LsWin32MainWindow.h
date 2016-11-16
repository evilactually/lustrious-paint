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
  void Destroy();
  void Hide();
  void Show();
  void Minimize();
  void Restore();
  void Maximize();
protected:
  static LsWin32MainWindow window;
  HWND windowHandle;
  MSG msg;
  LsWin32MainWindow();
  ~LsWin32MainWindow();
  LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};