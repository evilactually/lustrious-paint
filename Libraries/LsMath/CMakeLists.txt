cmake_minimum_required ( VERSION 3.5.2 )

set( LIB_NAME LsMath )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set(INCLUDE_FILES
  Include/LsMath.h
  Include/LsVector3.h
  Include/LsGeometry.h)

set(SOURCE_FILES
  Source/LsVector3.cpp
  Source/LsMath.cpp
  Source/LsGeometry.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( ${LIB_NAME} STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

target_include_directories( ${LIB_NAME} PUBLIC Include
                                               "${CMAKE_SOURCE_DIR}/Include" )

# include_directories( Include ) # target_include_directories does this, no?