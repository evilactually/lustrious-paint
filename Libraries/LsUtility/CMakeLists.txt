cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set(INCLUDE_FILES
  Include/IvTypes.h
  Include/LsConsole.h
  Include/LsOptional.h
  Include/LsStdAlgorithms.h
  Include/LsTuple.h
  Include/LsError.h
  Include/LsFile.h)

set(SOURCE_FILES
  Source/LsError.cpp
  Source/LsConsole.cpp
  Source/LsFile.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( LsUtility STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

include_directories( Include )