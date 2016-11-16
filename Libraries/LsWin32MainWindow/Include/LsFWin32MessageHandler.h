#pragma once

#include <vector>
#include <windows.h>

class LsFWin32MessageHandler
{
  static std::vector<LsFWin32MessageHandler*> Handlers;
public:
  LsFWin32MessageHandler();
  ~LsFWin32MessageHandler();
  virtual void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
  static void BroadCastWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam);
};