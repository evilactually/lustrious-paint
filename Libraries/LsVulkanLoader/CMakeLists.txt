cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set(INCLUDE_FILES
  Include/LsVulkanCommands.h
  Include/LsVulkanCommands.inl
  Include/LsVulkanLoader.h
  Include/vulkan_dynamic.hpp)

set(SOURCE_FILES
  Source/LsVulkanCommands.cpp
  Source/LsVulkanLoader.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

find_package( VulkanSDK )

add_library( LsVulkanLoader STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

include_directories( Include )
include_directories( ${VULKAN_INCLUDE_DIR} )

target_link_libraries( LsVulkanLoader ${VULKAN_LIBRARY} )