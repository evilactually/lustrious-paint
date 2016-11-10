
//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <windows.h>
#include <string>

//-------------------------------------------------------------------------------
//-- Globals --------------------------------------------------------------------
//-------------------------------------------------------------------------------

bool dialogShowing;
HWND windowHandle;

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsSetDialogParentWindow(HWND handle) {
  windowHandle = handle;
}

bool LsIsDialogShowing() {
  return dialogShowing;
}

void LsAbort(std::string& msg) {
  dialogShowing = true;
  MessageBox(windowHandle,
             msg.c_str(),
             "Error",
             MB_OK | MB_ICONERROR);
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
  MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
  throw 1;
}
