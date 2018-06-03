cmake_minimum_required ( VERSION 3.5.2 )
enable_testing()
set( LIB_NAME LsStuffing )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set( LIBRARY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Libraries/" )

set( INCLUDE_FILES
  Include/LsIsosurfaceStuffer.h
  Include/LsTetrahedronMesh.h
  Include/LsBCCLattice.h
  Include/LsPatternMatchers.h
  Include/LsIsosphere.h
  Include/LsITetrahedronProcessor.h
  Include/LsSpringMeshBuilder.h
  Include/LsBCCLatticeRef.h
  Include/LsHashedBCCLatticeVertex.h
)

set(SOURCE_FILES 
  Source/LsIsosurfaceStuffer.cpp
  Source/LsTetrahedronMesh.cpp
  Source/LsBCCLattice.cpp
  Source/LsPatternMatchers.cpp
  Source/LsIsosphere.cpp
  Source/LsSpringMeshBuilder.cpp
  Source/LsBCCLatticeRef.cpp
  Source/LsHashedBCCLatticeVertex.cpp
)

set( TEST_FILES
  Tests/LsTetrahedronMeshTest
  Tests/LsBCCLatticeTests.h
  Tests/LsPatternMatchersTest.h
  Tests/LsIsosphereTests.h
  Tests/LsHashingTest.h
  Tests/Main.cpp
)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

add_library( ${LIB_NAME} STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

target_include_directories( ${LIB_NAME} PUBLIC Include
                                               "${CMAKE_SOURCE_DIR}/Include"
                                               "${LIBRARY_SOURCE_DIR}/LsUtility/Include"
                                               "${LIBRARY_SOURCE_DIR}/LsMath/Include" )

target_link_libraries( ${LIB_NAME} LsMath )

add_executable( _Test${LIB_NAME}
  ${TEST_FILES}
)

target_include_directories( _Test${LIB_NAME} PUBLIC Include
                                                    "${CMAKE_SOURCE_DIR}/Include" )

target_link_libraries( _Test${LIB_NAME} ${LIB_NAME} )

add_test( NAME _Test${LIB_NAME} COMMAND _Test${LIB_NAME} )