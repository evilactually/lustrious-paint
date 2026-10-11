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
  // Sculpt dabs build up, so they are closer, wider spacing leaves ripples along strokes
  float fraction = strokeBrush.mode == LsCanvasDabMode::Sculpt ? 0.1f : 0.25f;
  return glm::max(fraction*brushRadius/navigator->GetZoom(), 0.5f);
}

void LsCanvasPainter::Pick(glm::vec2 canvasPoint) {
  canvas->RequestNormalPick(canvasPoint, brushRadius/navigator->GetZoom());
  pickPoint = canvasPoint;
  hasPickPoint = true;
}

LsCanvasBrush LsCanvasPainter::MakeBrush(bool shift) const {
  LsCanvasBrush brush;
  brush.radius = brushRadius/navigator->GetZoom();
  brush.opacity = brushOpacity;
  brush.falloff = brushFalloff;
  brush.mode = mode;
  // Shift blends instead, as smoothing does in sculpting applications. Lights
  // are on or off, so for light strokes it paints an empty rig instead, which
  // leaves only the scene's emitters, and the normals as they are.
  brush.blend = shift && mode != LsCanvasDabMode::Light;
  brush.rig = shift ? 0u : rig;
  brush.push = sculptPush;
  brush.alongNormal = sculptAlongNormal;
  brush.fixedNormal = mode == LsCanvasDabMode::Light ? rigNormal && !shift : fixedNormal;
  brush.normal = normal;
  brush.blendEdgeThreshold = blendKeepsEdges ? blendEdgeThreshold : 0.0f;
  return brush;
}

void LsCanvasPainter::Fill(bool shift) {
  // A dab the size of the canvas would move everything by the canvas's size
  if (mode == LsCanvasDabMode::Sculpt) {
    return;
  }
  // One dab over the whole canvas, evenly at the brush opacity. Blending isn't
  // a fill, its smoothing reaches with the dab, so Shift only empties the rig.
  LsCanvasBrush brush = MakeBrush(shift && mode == LsCanvasDabMode::Light);
  glm::vec2 size(canvas->GetSize());
  brush.radius = glm::length(size);
  brush.falloff = 0.0f;
  canvas->Paint({ size*0.5f }, brush, true);
}

void LsCanvasPainter::BeginStroke(glm::vec2 canvasPoint) {
  // Alt picks the normal under the brush instead, as the eyedropper does in
  // painting applications, averaged over the brush
  picking = pickArmed || (SDL_GetModState() & SDL_KMOD_ALT) != 0;
  pickArmed = false;
  if (picking) {
    Pick(canvasPoint);
    return;
  }
  strokeBrush = MakeBrush((SDL_GetModState() & SDL_KMOD_SHIFT) != 0);
  canvas->Paint({ canvasPoint }, strokeBrush, true);
  lastStrokeSample = canvasPoint;
  distanceToNextDab = GetDabSpacing();
}

// Fills the gap between the previous input sample and this one with dabs, the leftover
// distance carries over to the next segment so spacing stays even across samples
void LsCanvasPainter::ContinueStroke(glm::vec2 canvasPoint) {
  if (picking) {
    Pick(canvasPoint);
    return;
  }
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
