cmake_minimum_required ( VERSION 3.16 )
enable_testing()
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

project ( LustriousPaint C CXX )

# MSVC's default; the vendored Catch and FactoradicPermutation still use library parts C++17 removed
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL3 ships with the LunarG Vulkan SDK on Windows; any other SDL3 install can be supplied via CMAKE_PREFIX_PATH
find_package( SDL3 CONFIG COMPONENTS SDL3 HINTS "$ENV{VULKAN_SDK}/cmake" "$ENV{VULKAN_SDK}" )

# Distributions that don't package SDL3 yet, such as Ubuntu 22.04, get it built from source
if (NOT SDL3_FOUND)
  message(STATUS "SDL3 not found, fetching and building it from source")
  include(FetchContent)
  FetchContent_Declare( SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
    GIT_SHALLOW TRUE )
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable( SDL3 )
endif()

add_subdirectory( Application )
add_subdirectory( Libraries )
#add_subdirectory( Tools )
add_subdirectory( Shaders )
