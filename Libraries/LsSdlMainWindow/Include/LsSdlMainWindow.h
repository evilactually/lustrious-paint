#pragma once

#include <string>
#include <SDL3/SDL.h>

class LsSdlMainWindow
{
public:
  void Create(std::string title, int x, int y, int width, int height);
  static LsSdlMainWindow* Get();
  SDL_Window* GetWindowHandle();
  void WaitForMessages();
  bool ProcessMessages();
  void Destroy();
  void Hide();
  void Show();
  void Minimize();
  void Restore();
  void Maximize();
  void ShowMouse();
  void HideMouse();
  void GetClientArea(int* x, int* y, int* width, int* height); // in pixels
  SDL_FPoint GetMousePosition();                               // in pixels, relative to client area
protected:
  static LsSdlMainWindow window;
  SDL_Window* windowHandle = nullptr;
  bool mouseVisible = true;
  LsSdlMainWindow();
  ~LsSdlMainWindow();
  bool HandleEvent(SDL_Event& event);
  void ConvertEventToPixels(SDL_Event& event);
};
