#include <vector>
#include <windows.h>

class FWin32MessageHandler
{
  static std::vector<FWin32MessageHandler*> Handlers;
public:
  FWin32MessageHandler() {
    Handlers.push_back(this);
  }

  ~FWin32MessageHandler();

  virtual void OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

  static void BroadCastWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam);
};