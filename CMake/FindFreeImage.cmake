# Find FreeImage
#
# FREEIMAGE_INCLUDE_DIR
# FREEIMAGE_LIBRARY
# FREEIMAGE_FOUND

if (WIN32)

  if (CMAKE_CL_64)
    set(FREEIMAGE_DISTRIBUTION "$ENV{FREEIMAGE_DIR}/Dist/x64/")
  else()
    set(FREEIMAGE_DISTRIBUTION "$ENV{FREEIMAGE_DIR}/Dist/x32/")
  endif()
  find_path(FREEIMAGE_INCLUDE_DIR NAMES FreeImage.h HINTS ${FREEIMAGE_DISTRIBUTION})
  find_library(FREEIMAGE_LIBRARY_STATIC NAMES "FreeImage.lib" HINTS ${FREEIMAGE_DISTRIBUTION})
  find_file(FREEIMAGE_LIBRARY_SHARED NAMES "FreeImage.dll" HINTS ${FREEIMAGE_DISTRIBUTION})

endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeImage DEFAULT_MSG FREEIMAGE_INCLUDE_DIR
                                                        FREEIMAGE_LIBRARY_STATIC
                                                        FREEIMAGE_LIBRARY_SHARED)

mark_as_advanced(FREEIMAGE_INCLUDE_DIR
                 FREEIMAGE_LIBRARY_STATIC
                 FREEIMAGE_LIBRARY_SHARED)