cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set(INCLUDE_FILES
  Include/LsWintabFunctions.inl
  Include/LsWintabLoader.h
  Include/DWINTAB.h)

set(SOURCE_FILES
  Source/DWINTAB.cpp
  Source/LsWintabLoader.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( LsWintabLoader STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

include_directories( Include )
include_directories( "${CMAKE_SOURCE_DIR}/Include" )
