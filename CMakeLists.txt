cmake_minimum_required (VERSION 3.5.2)
project (LustriousPaint)

# Build numbers
set (VERSION_BUILD_x86 8563)
set (VERSION_BUILD_x86_64 7563)

# The version number.
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (VERSION_REVISION 0)
if (CMAKE_CL_64)
	set (VERSION_BUILD ${VERSION_BUILD_x86_64})
else()
	set (VERSION_BUILD ${VERSION_BUILD_x86})
endif()

# Configure version resource file
configure_file (
  "${PROJECT_SOURCE_DIR}/resources/version.rc.in"
  "${PROJECT_BINARY_DIR}/resources/version.rc"
  )

file(GLOB_RECURSE SOURCE_FILES
    "src/*.h"
    "src/*.cpp"
	"${PROJECT_BINARY_DIR}/resources/*.rc"
	"resources/*.rc"
)

add_executable(LustriousPaint WIN32 ${SOURCE_FILES} )