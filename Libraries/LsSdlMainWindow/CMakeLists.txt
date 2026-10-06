cmake_minimum_required ( VERSION 3.16 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set(INCLUDE_FILES
  Include/LsFSdlEventHandler.h
  Include/LsSdlMainWindow.h)

set(SOURCE_FILES
  Source/LsFSdlEventHandler.cpp
  Source/LsSdlMainWindow.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( LsSdlMainWindow STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

target_include_directories( LsSdlMainWindow PUBLIC Include
                                                   "${CMAKE_SOURCE_DIR}/Include" )

target_link_libraries( LsSdlMainWindow PUBLIC SDL3::SDL3 )
