#include <string>
#include <SDL3/SDL.h>
#include <LsSdlMainWindow.h>
#include <LsFSdlEventHandler.h>

void LsSdlMainWindow::Create(std::string title, int x, int y, int width, int height) {
  // Have the pen also generate mouse events, so mouse driven components work with a tablet
  SDL_SetHint(SDL_HINT_PEN_MOUSE_EVENTS, "1");

  if ( !SDL_Init(SDL_INIT_VIDEO) ) {
    throw std::string("Could not initialize SDL: ") + SDL_GetError();
  }

  windowHandle = SDL_CreateWindow(title.c_str(),
                                  width,
                                  height,
                                  SDL_WINDOW_VULKAN |
                                  SDL_WINDOW_RESIZABLE |
                                  SDL_WINDOW_HIGH_PIXEL_DENSITY |
                                  SDL_WINDOW_HIDDEN);
  if ( !windowHandle ) {
    throw std::string("Could not create window: ") + SDL_GetError();
  }

  SDL_SetWindowPosition(windowHandle, x, y);
}

LsSdlMainWindow* LsSdlMainWindow::Get() {
  return &window;
}

SDL_Window* LsSdlMainWindow::GetWindowHandle() {
  return windowHandle;
}

void LsSdlMainWindow::WaitForMessages() {
  // Passing null blocks until an event is available without removing it from the queue
  SDL_WaitEvent(nullptr);
}

bool LsSdlMainWindow::ProcessMessages() {
  SDL_Event event;
  while ( SDL_PollEvent(&event) ) {
    if ( !HandleEvent(event) ) {
      return false;
    }
  }
  return true;
}

void LsSdlMainWindow::Destroy() {
  if ( windowHandle ) {
    SDL_DestroyWindow(windowHandle);
    windowHandle = nullptr;
  }
  SDL_Quit();
}

void LsSdlMainWindow::Hide() {
  SDL_HideWindow(windowHandle);
}

void LsSdlMainWindow::Show() {
  SDL_ShowWindow(windowHandle);
}

void LsSdlMainWindow::Minimize() {
  SDL_MinimizeWindow(windowHandle);
}

void LsSdlMainWindow::Restore() {
  SDL_RestoreWindow(windowHandle);
}

void LsSdlMainWindow::Maximize() {
  SDL_MaximizeWindow(windowHandle);
}

void LsSdlMainWindow::ShowMouse() {
  mouseVisible = true;
  SDL_ShowCursor();
}

void LsSdlMainWindow::HideMouse() {
  // SDL only hides the cursor while it is over its own windows
  mouseVisible = false;
  SDL_HideCursor();
}

void LsSdlMainWindow::GetClientArea(int* x, int* y, int* width, int* height) {
  int pixelWidth = 0;
  int pixelHeight = 0;
  SDL_GetWindowSizeInPixels(windowHandle, &pixelWidth, &pixelHeight);

  if (x) {
    *x = 0;
  }

  if (y) {
    *y = 0;
  }

  if (width) {
    *width = pixelWidth;
  }

  if (height) {
    *height = pixelHeight;
  }
}

SDL_FPoint LsSdlMainWindow::GetMousePosition() {
  SDL_FPoint cursor = {};
  SDL_GetMouseState(&cursor.x, &cursor.y);
  float density = SDL_GetWindowPixelDensity(windowHandle);
  cursor.x *= density;
  cursor.y *= density;
  return cursor;
}

void LsSdlMainWindow::SetEventFilter(std::function<bool(const SDL_Event&)> filter) {
  eventFilter = filter;
}

LsSdlMainWindow LsSdlMainWindow::window;

LsSdlMainWindow::LsSdlMainWindow() {

}

LsSdlMainWindow::~LsSdlMainWindow() {

}

// SDL reports positions in window coordinates, which differ from pixels on high DPI displays.
// Rendering works in pixels, so convert once here before handing events out.
void LsSdlMainWindow::ConvertEventToPixels(SDL_Event& event) {
  float density = SDL_GetWindowPixelDensity(windowHandle);
  if ( density == 1.0f || density == 0.0f ) {
    return;
  }

  switch ( event.type ) {
    case SDL_EVENT_MOUSE_MOTION:
      event.motion.x *= density;
      event.motion.y *= density;
      event.motion.xrel *= density;
      event.motion.yrel *= density;
    break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      event.button.x *= density;
      event.button.y *= density;
    break;
    case SDL_EVENT_MOUSE_WHEEL:
      event.wheel.mouse_x *= density;
      event.wheel.mouse_y *= density;
    break;
    case SDL_EVENT_PEN_MOTION:
      event.pmotion.x *= density;
      event.pmotion.y *= density;
    break;
    case SDL_EVENT_PEN_DOWN:
    case SDL_EVENT_PEN_UP:
      event.ptouch.x *= density;
      event.ptouch.y *= density;
    break;
    case SDL_EVENT_PEN_BUTTON_DOWN:
    case SDL_EVENT_PEN_BUTTON_UP:
      event.pbutton.x *= density;
      event.pbutton.y *= density;
    break;
    case SDL_EVENT_PEN_AXIS:
      event.paxis.x *= density;
      event.paxis.y *= density;
    break;
  }
}

bool LsSdlMainWindow::HandleEvent(SDL_Event& event) {
  bool consumed = eventFilter && eventFilter(event);

  if ( !consumed ) {
    ConvertEventToPixels(event);
    LsFSdlEventHandler::BroadcastSdlEvent(event);
  }

  switch( event.type ) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    return false;
  }

  return true;
}
