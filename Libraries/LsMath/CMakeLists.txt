cmake_minimum_required ( VERSION 3.5.2 )
enable_testing()
set( LIB_NAME LsMath )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set(
	INCLUDE_FILES
  Include/LsMath.h
  Include/LsVector3.h
  Include/LsGeometry.h
)

set(
	SOURCE_FILES
  Source/LsVector3.cpp
  Source/LsMath.cpp
  Source/LsGeometry.cpp
)

set(
	TEST_FILES
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

# include_directories( Include ) # target_include_directories does this, no?