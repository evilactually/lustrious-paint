
class PointGrid
{
  struct {
    float r;
    float g;
    float b;
  } pointColor;

  struct {
    float r;
    float g;
    float b;
  } backgroundColor;

public:
  PointGrid();
  ~PointGrid();
  SetPointColor(float r, float g, float b);
  SetBackgroundColor(float r, float g, float b);
  SetSpacing(float spacing);
  Render();
};
