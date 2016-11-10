cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set(INCLUDE_FILES
  Include/LsMath.h
  Include/LsVector3.h)

set(SOURCE_FILES
  Source/LsVector3.cpp
  Source/LsMath.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( LsMath STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

include_directories( Include )