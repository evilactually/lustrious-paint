cmake_minimum_required ( VERSION 3.5.2 )
enable_testing()
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

project ( LustriousPaint C CXX )

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/Build)

add_subdirectory( Application )
add_subdirectory( Libraries )
#add_subdirectory( Tools )
add_subdirectory( Shaders )
