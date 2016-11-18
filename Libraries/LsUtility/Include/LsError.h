//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <windows.h>
#include <string>
#include <vector>

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsSetDialogParentWindow(HWND handle);

bool LsIsDialogShowing();

void LsAbort(std::string& msg);

void LsAbort(const char* msg);

void LsAbort();

void LsError();

void LsExitFatal(std::string message, std::string caption);

void LsErrorMessage(std::string message, std::string caption);

void LsMessageBox(std::string message, std::string caption);