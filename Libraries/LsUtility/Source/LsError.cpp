
//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <SDL3/SDL.h>
#include <string>

//-------------------------------------------------------------------------------
//-- Globals --------------------------------------------------------------------
//-------------------------------------------------------------------------------

bool dialogShowing;
SDL_Window* windowHandle;

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsSetDialogParentWindow(SDL_Window* handle) {
  windowHandle = handle;
}

bool LsIsDialogShowing() {
  return dialogShowing;
}

void LsAbort(std::string& msg) {
  dialogShowing = true;
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", msg.c_str(), windowHandle);
  throw 1;
}

void LsAbort(const char* msg) {
  LsAbort(std::string(msg));
}

void LsAbort() {
  throw 1;
}

void LsError() {
  LsAbort(std::string("Error occurred, view log for details."));   
}

void LsExitFatal(std::string message, std::string caption) 
{
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption.c_str(), message.c_str(), nullptr);
  throw 1;
}

void LsErrorMessage(std::string message, std::string caption) 
{
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption.c_str(), message.c_str(), nullptr);
}

void LsMessageBox(std::string message, std::string caption) 
{
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, caption.c_str(), message.c_str(), nullptr);
}