#include <LsFSdlEventHandler.h>
#include <vector>
#include <algorithm>

std::vector<LsFSdlEventHandler*> LsFSdlEventHandler::Handlers;

LsFSdlEventHandler::LsFSdlEventHandler() {
  Handlers.push_back(this);
}

LsFSdlEventHandler::~LsFSdlEventHandler() {
  Handlers.erase(std::remove(Handlers.begin(), Handlers.end(), this), Handlers.end());
}

void LsFSdlEventHandler::BroadcastSdlEvent(const SDL_Event& event) {
  for (auto i = Handlers.begin(); i != Handlers.end(); ++i)
  {
    (*i)->OnSdlEvent(event);
  }
}
