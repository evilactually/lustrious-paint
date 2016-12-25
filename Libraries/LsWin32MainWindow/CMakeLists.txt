cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set( LIBRARY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Libraries/")

set(INCLUDE_FILES
  Include/LsFWin32MessageHandler.h
  Include/LsWin32MainWindow.h)

set(SOURCE_FILES
  Source/LsFWin32MessageHandler.cpp
  Source/LsWin32MainWindow.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( LsWin32MainWindow STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )


target_include_directories( LsWin32MainWindow PUBLIC Include
                                                     "${CMAKE_SOURCE_DIR}/Include"
                                                     "${LIBRARY_SOURCE_DIR}/LsWintabLoader/Include" )
