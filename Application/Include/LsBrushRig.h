#pragma once

#include <windows.h>
#include <LsFWin32MessageHandler.h>

class LsBrushRig: public LsFWin32MessageHandler
{
public:
  LsBrushRig();
  ~LsBrushRig();
  void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
  void Render();
};
