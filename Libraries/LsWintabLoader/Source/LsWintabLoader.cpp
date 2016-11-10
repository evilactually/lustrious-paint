//===============================================================================
// @ WintabLoader.cpp
// 
// Loader for Wintab library
//
//===============================================================================

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <windows.h>
#include <iostream>
#include <DWINTAB.h>

//-------------------------------------------------------------------------------
//-- Globals --------------------------------------------------------------------
//-------------------------------------------------------------------------------

static HMODULE wintabLibrary;

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsLoadWintabLibrary() {
  wintabLibrary = LoadLibrary("Wintab32.dll");
  if (wintabLibrary == nullptr) {
    std::cout << "Could not load Wintab library!" << std::endl;
    throw 1;
  }
}

void LsUnloadWintabLibrary() {
  if ( wintabLibrary ) {
    FreeLibrary( wintabLibrary );
  }
}

void LsLoadEntryPoints() {
  #define WT_EXPORTED_FUNCTION( fun )                                         \
    if(!(fun = (PFN_##fun)GetProcAddress( wintabLibrary, #fun ))) {           \
      std::cout << "Could not load exported function: ##fun!" << std::endl;   \
      throw 1;                                                                \
    }
  #include "LsWintabFunctions.inl"
}