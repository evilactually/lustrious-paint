cmake_minimum_required (VERSION 3.5.2)
project (LustriousPaint)

# Build numbers
set (VERSION_BUILD_x86 1)
set (VERSION_BUILD_x86_64 1)

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

# Glob all project source files
file(GLOB_RECURSE SOURCE_FILES
    "src/*.h"
    "src/*.cpp"
	"${PROJECT_BINARY_DIR}/resources/*.rc"
	"resources/*.rc"
)

# Use LunarG's SDK
find_library(VULKAN_LIB NAMES vulkan-1
             HINTS "C:/VulkanSDK/" )
             
add_executable(LustriousPaint WIN32 ${SOURCE_FILES} )