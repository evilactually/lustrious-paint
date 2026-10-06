#include <iostream>
#include <glm/glm.hpp>
#include <LsRenderer.h>
#include <LsUselessBox.h>

LsUselessBox::LsUselessBox(std::shared_ptr<LsRenderer> renderer) :renderer(renderer) {}

LsUselessBox::~LsUselessBox() {

}

void LsUselessBox::Render() {
  renderer->SetColor(1.0f, 1.0f, 1.0f);
  renderer->SetLineWidth(1.0f);
  renderer->DrawLine(x, y, x + width, y);
  renderer->DrawLine(x, y, x, y + height);
  renderer->DrawLine(x, y + width, x + width, y + height);
  renderer->DrawLine(x + width, y, x + width, y + height);
}

bool LsUselessBox::HitTest(SDL_FPoint point) {
  return point.x >= x && point.x <= (x + width) && point.y >= y && point.y <= (y + height);
}

void LsUselessBox::OnSdlEvent(const SDL_Event& event) {
  // Pen input also arrives here as mouse events (SDL_HINT_PEN_MOUSE_EVENTS)
  SDL_FPoint cursor;
  switch (event.type) {
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    dragging = false;
    break;
    case SDL_EVENT_MOUSE_MOTION:
    cursor = { event.motion.x, event.motion.y };
    if (painting)
    {
      renderer->CanvasStroke(cursor.x, cursor.y, 10.0f);
    }
    if ( dragging ) {
      x = cursor.x - grabOffset.x;
      y = cursor.y - grabOffset.y;
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (event.button.button != SDL_BUTTON_LEFT) {
      break;
    }
    cursor = { event.button.x, event.button.y };
    painting = true;
    if ( HitTest(cursor) ) {
      dragging = true;
      grabOffset.x = cursor.x - x;
      grabOffset.y = cursor.y - y;
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
    if (event.button.button != SDL_BUTTON_LEFT) {
      break;
    }
    dragging = false;
    painting = false;
    break;
  }
}
