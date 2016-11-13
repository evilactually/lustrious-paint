#include <LsInput.h>
#include <FWin32MessageHandler.h>

class BrushRig: public FWin32MessageHandler
{
public:
  BrushRig();
  ~BrushRig();
  void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
  void Render();
protected:
  
};
