#include <windows.h>
#include <string>
#include "LsWin32MainWindow.h"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  LsWin32MainWindow::Get().Create(hInstance, "Lustrious Paint", 100, 100, 1024, 640);
  LsWin32MainWindow& window = LsWin32MainWindow::Get();
  window.Show();
  while( window.ProcessMessages() ){
    window.WaitForMessages();
  };
}