#include <cmath>
#include <LsRenderer.h>
#include <LsPointGrid.h>
#include <LsSdlMainWindow.h>

LsPointGrid::LsPointGrid(std::shared_ptr<LsRenderer> renderer):renderer(renderer) {}

LsPointGrid::~LsPointGrid() {

}

void LsPointGrid::SetPointColor(float r, float g, float b) {
  pointColor.r = r;
  pointColor.g = g;
  pointColor.b = b;
}

void LsPointGrid::SetBackgroundColor(float r, float g, float b) {
  backgroundColor.r = r;
  backgroundColor.g = g;
  backgroundColor.b = b;
}

void LsPointGrid::SetSpacing(float spacing) {
  this->spacing = spacing;
}

void LsPointGrid::Render() {
  struct {
    int width;
    int height;
  } windowExtent;

  LsSdlMainWindow::Get()->GetClientArea(NULL, NULL, &windowExtent.width, &windowExtent.height);

  renderer->Clear(backgroundColor.r, backgroundColor.g, backgroundColor.b);

  // The grid is attached to the canvas, so it pans and zooms along with it
  const glm::vec2 offset = renderer->GetCanvasOffset();
  const float zoom = renderer->GetCanvasZoom();

  // Zoomed out, skip every other point until they're far enough apart on screen
  const float minScreenSpacing = 16.0f;
  float screenSpacing = spacing*zoom;
  while( screenSpacing < minScreenSpacing ) {
    screenSpacing *= 2.0f;
  }

  // Grid lines that come before the window's top left corner
  const float firstX = offset.x - ceil(offset.x/screenSpacing)*screenSpacing;
  const float firstY = offset.y - ceil(offset.y/screenSpacing)*screenSpacing;

  renderer->SetPointSize(2.0f);
  renderer->SetColor(pointColor.r, pointColor.g, pointColor.b);

  for( float y = firstY; y < windowExtent.height + 1.0f; y += screenSpacing ) {
    for( float x = firstX; x < windowExtent.width + 1.0f; x += screenSpacing ) {
      renderer->DrawPoint(x - 1.0f, y - 1.0f);
    }
  }
}

void LsPointGrid::OnSdlEvent(const SDL_Event& event) { }