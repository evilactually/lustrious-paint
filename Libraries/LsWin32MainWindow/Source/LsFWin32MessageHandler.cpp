#include <windows.h>
#include <LsFWin32MessageHandler.h>
#include <vector>
#include <algorithm>

std::vector<LsFWin32MessageHandler*> LsFWin32MessageHandler::Handlers;

LsFWin32MessageHandler::LsFWin32MessageHandler() {
  Handlers.push_back(this);
}

LsFWin32MessageHandler::~LsFWin32MessageHandler() {
  Handlers.erase(std::remove(Handlers.begin(), Handlers.end(), this), Handlers.end());
}

void LsFWin32MessageHandler::BroadCastWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam){
  for (auto i = Handlers.begin(); i != Handlers.end(); ++i)
  {
    (*i)->OnWin32Message(uMsg, wParam, lParam);
  }
}