cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set( LIBRARY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Libraries/")

set(INCLUDE_FILES
  Include/LsCanvas.h
  Include/LsImage.h
  Include/LsRenderer.h
  Include/LsSceneView.h
  Include/LsUiTheme.h
  Include/LsPushConstantTypes.h
  Include/LsVulkanCommandBuffers.h
  Include/LsVulkanCommon.h
  Include/LsVulkanDevice.h
  Include/LsVulkanInstance.h
  Include/LsVulkanPresentation.h
  Include/LsVulkanRendering.h)

set(SOURCE_FILES
  Source/LsRenderer.cpp
  Source/LsCanvas.cpp
  Source/LsImage.cpp
  Source/LsSceneView.cpp
  Source/LsUiTheme.cpp )

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

find_package( VulkanSDK )

add_library( LsRenderer STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )


target_include_directories( LsRenderer PUBLIC Include
                                              "${CMAKE_SOURCE_DIR}/Include"
                                              "${LIBRARY_SOURCE_DIR}/LsVulkanLoader/Include"
                                              "${LIBRARY_SOURCE_DIR}/LsUtility/Include"
                                              ${VULKAN_INCLUDE_DIR} )

# include_directories( Include )

# include_directories( ${VULKAN_INCLUDE_DIR} )
# include_directories( "${CMAKE_SOURCE_DIR}/Include"
#                      "${LIBRARY_SOURCE_DIR}/LsVulkanLoader/Include"
#                      "${LIBRARY_SOURCE_DIR}/LsWin32MainWindow/Include"
#                      "${LIBRARY_SOURCE_DIR}/LsUtility/Include" )

target_link_libraries( LsRenderer ${VULKAN_LIBRARY}
                                  imgui
                                  LsVulkanLoader
                                  LsUtility
                                  LsScene
                                  SDL3::SDL3 )

target_compile_definitions(LsRenderer PUBLIC -DVULKAN_VALIDATION)

# F12 starts and F11 stops writing every presented frame to a BMP file
# target_compile_definitions(LsRenderer PUBLIC -DFRAME_RECORDING)