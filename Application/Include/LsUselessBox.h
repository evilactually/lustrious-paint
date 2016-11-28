#include <windows.h>

class LsUselessBox: public LsFWin32MessageHandler
{
  LsRenderer* renderer;
  POINT grabOffset;
  bool dragging = false;
  float x = 0.0f;
  float y = 0.0f;
  float width = 64.0f;
  float height = 64.0f;
public:
  LsUselessBox();
  ~LsUselessBox();
  void Render();
  bool HitTest(POINT point);
  void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};