#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <LsRenderer.h>
#include <LsCanvasPainter.h>

LsCanvasPainter::LsCanvasPainter(std::shared_ptr<LsRenderer> renderer, std::shared_ptr<LsCanvasNavigator> navigator,
                                 std::shared_ptr<LsCanvas> canvas) :
  renderer(renderer), navigator(navigator), canvas(canvas) {}

LsCanvasPainter::~LsCanvasPainter() {

}

void LsCanvasPainter::Render() {
  renderer->SetColor(1.0f, 1.0f, 1.0f);
  renderer->SetLineWidth(1.0f);
  renderer->DrawLine(x, y, x + width, y);
  renderer->DrawLine(x, y, x, y + height);
  renderer->DrawLine(x, y + width, x + width, y + height);
  renderer->DrawLine(x + width, y, x + width, y + height);
}

bool LsCanvasPainter::HitTest(SDL_FPoint point) {
  return point.x >= x && point.x <= (x + width) && point.y >= y && point.y <= (y + height);
}

float LsCanvasPainter::GetBrushRadius() {
  return brushRadius;
}

void LsCanvasPainter::SetBrushRadius(float radius) {
  brushRadius = radius;
}

// A quarter of the radius keeps the stroke edge smooth, but never less than half a canvas pixel
float LsCanvasPainter::GetDabSpacing() {
  return glm::max(0.25f*brushRadius/navigator->GetZoom(), 0.5f);
}

void LsCanvasPainter::BeginStroke(glm::vec2 canvasPoint) {
  strokeBrush.radius = brushRadius/navigator->GetZoom();
  strokeBrush.opacity = brushOpacity;
  strokeBrush.falloff = brushFalloff;
  strokeBrush.mode = mode;
  // Shift blends instead, as smoothing does in sculpting applications
  strokeBrush.blend = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
  strokeBrush.blendEdgeThreshold = blendKeepsEdges ? blendEdgeThreshold : 0.0f;
  canvas->Paint({ canvasPoint }, strokeBrush, true);
  lastStrokeSample = canvasPoint;
  distanceToNextDab = GetDabSpacing();
}

// Fills the gap between the previous input sample and this one with dabs, the leftover
// distance carries over to the next segment so spacing stays even across samples
void LsCanvasPainter::ContinueStroke(glm::vec2 canvasPoint) {
  glm::vec2 delta = canvasPoint - lastStrokeSample;
  float length = glm::length(delta);
  float spacing = GetDabSpacing();

  std::vector<glm::vec2> dabs;
  if (length > 0.0f) {
    glm::vec2 direction = delta/length;
    float distance = distanceToNextDab;
    for (; distance <= length; distance += spacing) {
      dabs.push_back(lastStrokeSample + direction*distance);
    }
    distanceToNextDab = distance - length;
  }
  lastStrokeSample = canvasPoint;

  canvas->Paint(dabs, strokeBrush, false);
}

void LsCanvasPainter::OnSdlEvent(const SDL_Event& event) {
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
      // The canvas may be panned and zoomed, the brush radius stays in screen pixels
      ContinueStroke(navigator->WindowToCanvas(glm::vec2(cursor.x, cursor.y)));
    }
    if ( dragging ) {
      x = cursor.x - grabOffset.x;
      y = cursor.y - grabOffset.y;
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    // A drag with space or ctrl held navigates the canvas instead
    if (event.button.button != SDL_BUTTON_LEFT || navigator->IsActive()) {
      break;
    }
    cursor = { event.button.x, event.button.y };
    painting = true;
    BeginStroke(navigator->WindowToCanvas(glm::vec2(cursor.x, cursor.y)));
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
