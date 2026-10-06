#include <cmath>
#include <imgui.h>
#include <LsRenderer.h>
#include <LsCanvasNavigator.h>

// Dragging this many pixels to the right doubles the zoom
static const float kZoomDragPerDoubling = 200.0f;
// Each wheel notch zooms by this factor
static const float kZoomPerWheelNotch = 1.25f;
static const float kMinZoom = 1.0f/32.0f;
static const float kMaxZoom = 64.0f;

LsCanvasNavigator::LsCanvasNavigator(std::shared_ptr<LsRenderer> renderer) :renderer(renderer) {
  ApplyView();
}

LsCanvasNavigator::Mode LsCanvasNavigator::GetArmedMode() const {
  // Don't steal keys while the user is typing into a text field
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput) {
    return Mode::None;
  }
  if (SDL_GetModState() & SDL_KMOD_CTRL) {
    return Mode::Zoom;
  }
  if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_SPACE]) {
    return Mode::Pan;
  }
  return Mode::None;
}

bool LsCanvasNavigator::IsActive() const {
  return mode != Mode::None || GetArmedMode() != Mode::None;
}

void LsCanvasNavigator::UpdateCursor() const {
  Mode shown = mode != Mode::None ? mode : GetArmedMode();
  if (shown == Mode::Pan) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  }
  else if (shown == Mode::Zoom) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
  }
}

glm::vec2 LsCanvasNavigator::WindowToCanvas(glm::vec2 point) const {
  return (point - offset)/zoom;
}

void LsCanvasNavigator::ZoomAround(glm::vec2 windowPoint, glm::vec2 canvasPoint, float newZoom) {
  zoom = glm::clamp(newZoom, kMinZoom, kMaxZoom);
  offset = windowPoint - canvasPoint*zoom;
  ApplyView();
}

void LsCanvasNavigator::ApplyView() {
  renderer->SetCanvasView(offset, zoom);
}

void LsCanvasNavigator::OnSdlEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (mode != Mode::None) {
      break;
    }
    if (event.button.button == SDL_BUTTON_MIDDLE) {
      mode = Mode::Pan;
    }
    else if (event.button.button == SDL_BUTTON_LEFT) {
      mode = GetArmedMode();
    }
    if (mode == Mode::None) {
      break;
    }
    dragButton = event.button.button;
    dragStart = glm::vec2(event.button.x, event.button.y);
    startOffset = offset;
    startZoom = zoom;
    break;
    case SDL_EVENT_MOUSE_MOTION: {
      glm::vec2 cursor(event.motion.x, event.motion.y);
      if (mode == Mode::Pan) {
        offset = startOffset + (cursor - dragStart);
        ApplyView();
      }
      else if (mode == Mode::Zoom) {
        // Keep the canvas point under the drag start fixed on screen
        glm::vec2 anchor = (dragStart - startOffset)/startZoom;
        ZoomAround(dragStart, anchor, startZoom*std::exp2((cursor.x - dragStart.x)/kZoomDragPerDoubling));
      }
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
    if (event.button.button == dragButton) {
      mode = Mode::None;
      dragButton = 0;
    }
    break;
    case SDL_EVENT_MOUSE_WHEEL: {
      // Wheel zoom would fight a drag over where the view is
      if (mode != Mode::None) {
        break;
      }
      float notches = event.wheel.y;
      if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
        notches = -notches;
      }
      glm::vec2 cursor(event.wheel.mouse_x, event.wheel.mouse_y);
      ZoomAround(cursor, WindowToCanvas(cursor), zoom*std::pow(kZoomPerWheelNotch, notches));
    }
    break;
  }
}
