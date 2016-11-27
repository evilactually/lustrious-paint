#include <windows.h>
#include <LsRenderer.h>
#include <LsPointGrid.h>
#include <LsWindowsUtils.h>
#include <LsWin32MainWindow.h>

LsPointGrid::LsPointGrid() {
  renderer = LsRenderer::Get();
}

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

  LsWin32MainWindow::Get()->GetClientArea(NULL, NULL, &windowExtent.width, &windowExtent.height);

  renderer->Clear(backgroundColor.r, backgroundColor.g, backgroundColor.b);

  const int horizontal_cell_count = static_cast<int>(ceil(((float)windowExtent.width)/spacing));
  const int vertical_cell_count = static_cast<int>(ceil(((float)windowExtent.height)/spacing));

  renderer->SetPointSize(2.0f);
  renderer->SetColor(pointColor.r, pointColor.g, pointColor.b);

  for( int y = 0; y < vertical_cell_count; ++y ) {
    float y_offset = y*spacing;
    for( int x = 0; x < horizontal_cell_count; ++x ) {
      float x_offset = x*spacing;
      renderer->DrawPoint(pixelDimensions.width*x_offset - 1.0f, pixelDimensions.height*y_offset - 1.0f);
    }
  }
}

void LsPointGrid::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  HWND windowHandle = LsWin32MainWindow::Get()->GetWindowHandle();
  switch (uMsg) {
    case WM_SIZE:
    GetVulkanPixelDimensions(windowHandle, &pixelDimensions.width, &pixelDimensions.height);
    break;
  }
}