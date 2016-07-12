cmake_minimum_required (VERSION 3.5.2)
project (LustriousPaint)

# Custom modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

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

# Configure version resource file
configure_file (
  "${PROJECT_SOURCE_DIR}/resources/Version.rc.in"
  "${PROJECT_BINARY_DIR}/resources/Version.rc"
  )

# Configure version header file
configure_file (
  "${PROJECT_SOURCE_DIR}/src/Version.h.in"
  "${PROJECT_BINARY_DIR}/src/Version.h"
  )

# Dowload C++ Vulkan wrapper
file(DOWNLOAD "https://raw.githubusercontent.com/nvpro-pipeline/vkcpp/b81f8ec376047f488b90182a7f291a2664fe6779/vulkan/vk_cpp.hpp" "${PROJECT_BINARY_DIR}/include/vulkan/vk_cpp.hpp")

# Glob all project source files
file(GLOB_RECURSE SOURCE_FILES
    "src/*.h"
    "src/*.cpp"
	"src/*.hpp"
	"src/*.inl"
	"${PROJECT_BINARY_DIR}/resources/*.rc"
	"${PROJECT_BINARY_DIR}/src/*.h"
	"${PROJECT_BINARY_DIR}/src/*.hpp"
	"resources/*.rc"
)

# Definitions
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

# Find Vulkan SDK
find_package(Vulkan)
if(VULKAN_FOUND)
	include_directories(${VULKAN_INCLUDE_DIR})
	message(${VULKAN_LIBRARY})
else()
	message(FATAL_ERROR "Vulkan SDK not found")
endif()

# Include generated & downloaded headers
include_directories("${PROJECT_BINARY_DIR}/src/")
include_directories("${PROJECT_BINARY_DIR}/include/")

add_executable(LustriousPaint WIN32 ${SOURCE_FILES})
target_link_libraries (LustriousPaint ${VULKAN_LIBRARY})