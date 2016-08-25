#pragma once

#include "utility.h"

#define NOWTBASICFXNS
#define NOWTCTXEDITFXNS
#define NOWTVISIBILITYFXNS
#define NOWTQUEUEFXNS
#include "wintab/WINTAB.H"

#include "wt_pfns.inl"

#define WT_EXPORTED_FUNCTION( fun ) PFN_##fun fun;

#include "wt_functions.inl"

namespace wt
{
  HMODULE wintabLibrary;
  
  void LoadWintabLibrary() {
    wintabLibrary = LoadLibrary("Wintab32.dll");
    if (wintabLibrary == nullptr) {
      std::cout << "Could not load Wintab library!" << std::endl;
      ls::Error();
      throw 1;
    }
  }

  void UnloadWintabLibrary() {
    if ( wintabLibrary ) {
      FreeLibrary( wintabLibrary );
    }
  }

  void LoadEntryPoints() {
    #define WT_EXPORTED_FUNCTION( fun )                                         \
      if(!(fun = (PFN_##fun)GetProcAddress( wintabLibrary, #fun ))) {           \
        std::cout << "Could not load exported function: ##fun!" << std::endl;   \
        throw 1;                                                                \
      }
    #include "wt_functions.inl"
  }
}