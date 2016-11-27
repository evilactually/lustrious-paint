#include <LsP

PointGrid::PointGrid() {

}

PointGrid::~PointGrid() {

}

void PointGrid::SetPointColor(float r, float g, float b) {
  pointColot.r = r;
  pointColot.g = g;
  pointColot.b = b;
}

void PointGrid::SetBackgroundColor(float r, float g, float b) {
  backgroundColot.r = r;
  backgroundColot.g = g;
  backgroundColot.b = b;
}

void PointGrid::SetSpacing(float spacing) {
  this->spacing = spacing;
}

void PointGrid::Render() {
  renderer->Clear(0.1f, 0.0f, 0.1f);
}