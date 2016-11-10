cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set(INCLUDE_FILES "")

set(SOURCE_FILES
  Source/ActorPlayground.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

find_package( CAF COMPONENTS core io REQUIRED )

add_executable( ActorPlayground WIN32 ${INCLUDE_FILES} ${SOURCE_FILES} )

target_include_directories( ActorPlayground PUBLIC Include
                                                   ${CAF_INCLUDE_DIRS} )

target_link_libraries( ActorPlayground ${CAF_LIBRARIES} )
