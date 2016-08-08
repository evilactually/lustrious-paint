#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

using namespace std;

//#ifdef _DEBUG

namespace Ls {

void AttachConsole() {
  
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONOUT$", "r", stdin);

}

}

//#endif
