# Build numbers
set (VERSION_BUILD_x86 1)
set (VERSION_BUILD_x86_64 1)

# The version number
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (VERSION_REVISION 0)
if (CMAKE_CL_64)
  set (VERSION_BUILD ${VERSION_BUILD_x86_64})
else()
  set (VERSION_BUILD ${VERSION_BUILD_x86})
endif()

# File info
set(COMPANY_NAME "Lustrious Labs Ltd")
set(FILE_DESCRIPTION "Lustrious Paint")
set(FILE_VERSION "1.0.0")
set(INTERNAL_NAME "LustriousPaint")
set(LEGAL_COPYRIGHT "Copyright (C) 2015-2016 Vladislav Shcherbakov")
set(ORIGINAL_FILENAME "${INTERNAL_NAME}.exe")
set(PRODUCT_NAME "Lustrious Paint")
set(PRODUCT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}.${VERSION_BUILD}")