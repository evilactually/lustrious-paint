#pragma once

#include <string>
#include <functional>
#include <SDL3/SDL.h>

class LsSdlMainWindow
{
public:
  void Create(std::string title, int x, int y, int width, int height); // unscaled, multiplied by display scale
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
  // The filter sees every event before it is converted to pixels and broadcast,
  // returning true consumes the event so LsFSdlEventHandlers never receive it
  void SetEventFilter(std::function<bool(const SDL_Event&)> filter);
protected:
  static LsSdlMainWindow window;
  SDL_Window* windowHandle = nullptr;
  bool mouseVisible = true;
  std::function<bool(const SDL_Event&)> eventFilter;
  LsSdlMainWindow();
  ~LsSdlMainWindow();
  bool HandleEvent(SDL_Event& event);
  void ConvertEventToPixels(SDL_Event& event);
};
