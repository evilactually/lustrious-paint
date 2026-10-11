//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <SDL3/SDL.h>
#include <string>
#include <vector>

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsSetDialogParentWindow(SDL_Window* handle);

bool LsIsDialogShowing();

void LsAbort(const std::string& msg);

void LsAbort(const char* msg);

void LsAbort();

void LsError();

void LsExitFatal(std::string message, std::string caption);

void LsErrorMessage(std::string message, std::string caption);

void LsMessageBox(std::string message, std::string caption);