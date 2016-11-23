cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set( LIBRARY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Libraries/")

set(INCLUDE_FILES
  Include/LsRenderer.h)

set(SOURCE_FILES
  Source/LsRenderer.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

find_package( VulkanSDK )

add_library( LsRenderer STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

message( "${VULKAN_INCLUDE_DIR}" )

# target_include_directories( LsRenderer PUBLIC 
#                                               ${VULKAN_INCLUDE_DIR}
#                                               "${LIBRARY_SOURCE_DIR}/LsVulkanLoader/Include" )

include_directories( Include )

include_directories( ${VULKAN_INCLUDE_DIR} )
include_directories( "${CMAKE_SOURCE_DIR}/Include"
                     "${LIBRARY_SOURCE_DIR}/LsVulkanLoader/Include"
                     "${LIBRARY_SOURCE_DIR}/LsUtility/Include" )

target_link_libraries( LsRenderer ${VULKAN_LIBRARY}
                                  LsVulkanLoader )

target_compile_definitions(LsRenderer PUBLIC -DVULKAN_VALIDATION)