#include <windows.h>

#include "wt_pfns.hpp"

#define WT_EXPORTED_FUNCTION( fun ) PFN_##fun fun;

#include "wt_functions.inl"

namespace wt
{
  HMODULE wintabLibrary;
}