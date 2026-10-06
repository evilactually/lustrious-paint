cmake_minimum_required ( VERSION 3.5.2 )
enable_testing()
set( LIB_NAME LsScene )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set( INCLUDE_FILES
  Include/LsCamera.h
  Include/LsScene.h
  Include/LsShape.h
  Include/LsTransform.h
)

set( SOURCE_FILES
  Source/LsCamera.cpp
  Source/LsScene.cpp
  Source/LsShape.cpp
  Source/LsTransform.cpp
)

set( TEST_FILES
  Tests/Main.cpp
)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( ${LIB_NAME} STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

target_include_directories( ${LIB_NAME} PUBLIC Include
                                               "${CMAKE_SOURCE_DIR}/Include" )

add_executable( _Test${LIB_NAME}
  ${TEST_FILES}
)

target_link_libraries( _Test${LIB_NAME} ${LIB_NAME} )

add_test( NAME _Test${LIB_NAME} COMMAND _Test${LIB_NAME} )
