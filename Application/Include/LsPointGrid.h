#include <LsRenderer.h>
#include <LsFWin32MessageHandler.h>

class LsPointGrid: public LsFWin32MessageHandler
{
  struct {
    float r;
    float g;
    float b;
  } pointColor = {0.2f, 0.2f, 0.2f};

  struct {
    float r;
    float g;
    float b;
  } backgroundColor = {0.1f, 0.1f, 0.1f};

  float spacing = 32.0f;

  struct {
    float width;
    float height;
  } pixelDimensions;

  LsRenderer* renderer;

public:
  LsPointGrid();
  ~LsPointGrid();
  void SetPointColor(float r, float g, float b);
  void SetBackgroundColor(float r, float g, float b);
  void SetSpacing(float spacing);
  void Render();
  void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};
