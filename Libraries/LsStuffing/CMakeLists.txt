cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set( LIBRARY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Libraries/")

set(INCLUDE_FILES
  Include/LsIsosurfaceStuffer.h
  Include/LsTetrahedronMesh.h
  Include/LsBCCLattice.h)

set(SOURCE_FILES
  Source/LsIsosurfaceStuffer.cpp
  Source/LsTetrahedronMesh.cpp
  Source/LsBCCLattice.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( LsStuffing STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

target_include_directories( LsStuffing PUBLIC Include
                                              "${CMAKE_SOURCE_DIR}/Include"
                                              "${LIBRARY_SOURCE_DIR}/LsUtility/Include"
                                              "${LIBRARY_SOURCE_DIR}/LsMath/Include" )
