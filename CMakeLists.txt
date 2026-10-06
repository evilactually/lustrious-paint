cmake_minimum_required ( VERSION 3.5.2 )
enable_testing()
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

project ( LustriousPaint C CXX )

# SDL3 ships with the LunarG Vulkan SDK; any other SDL3 install can be supplied via CMAKE_PREFIX_PATH
find_package( SDL3 REQUIRED CONFIG COMPONENTS SDL3 HINTS "$ENV{VULKAN_SDK}/cmake" "$ENV{VULKAN_SDK}" )

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/Build)

add_subdirectory( Application )
add_subdirectory( Libraries )
#add_subdirectory( Tools )
add_subdirectory( Shaders )
