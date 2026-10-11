cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set( LIBRARY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Libraries/")

include( BuildInfo )

set(INCLUDE_FILES
  "Include/LsBrushRig.h"
  "Include/LsPointGrid.h"
  "Include/LsCanvasPainter.h"
  "Include/LsStuffingDemo.h"
  "Include/LsSceneEditor.h"
  "Include/LsCanvasNavigator.h"
  "Include/LsSceneViewport.h"
  "Include/LsTransformGizmo.h"
  "Include/LsLightingPanel.h"
  "Include/LsDocument.h"
  "Include/LsViewportProjection.h"
  "Include/Version.h.in")

set(SOURCE_FILES
  "${CMAKE_SOURCE_DIR}/Resources/Version.rc.in"
  "Source/main.cpp"
  "Source/LsBrushRig.cpp"
  "Source/LsPointGrid.cpp"
  "Source/LsCanvasPainter.cpp"
  "Source/LsStuffingDemo.cpp"
  "Source/LsSceneEditor.cpp"
  "Source/LsCanvasNavigator.cpp"
  "Source/LsSceneViewport.cpp"
  "Source/LsTransformGizmo.cpp"
  "Source/LsLightingPanel.cpp"
  "Source/LsDocument.cpp")

# Version information and the icon are Windows resources
if (WIN32)
  set(RESOURCE_FILES
    "${CMAKE_BINARY_DIR}/Resources/Version.rc"
    "${CMAKE_SOURCE_DIR}/Resources/Icon.rc")
endif()

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

configure_file(
  "${CMAKE_SOURCE_DIR}/Resources/Version.rc.in"
  "${CMAKE_BINARY_DIR}/Resources/Version.rc")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Include/Version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Include/Version.h")

find_package( VulkanSDK REQUIRED )

add_executable( LustriousPaint WIN32 ${INCLUDE_FILES} ${SOURCE_FILES} ${RESOURCE_FILES} )

target_include_directories( LustriousPaint PUBLIC Include
                                                  "${CMAKE_SOURCE_DIR}/Include"
                                                  "${CMAKE_CURRENT_BINARY_DIR}/Include/"
                                                  "${LIBRARY_SOURCE_DIR}/LsRenderer/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsMath/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsUtility/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsVulkanLoader/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsVulkanSystem/Include"
                                                  ${VULKAN_INCLUDE_DIR} )

target_link_libraries( LustriousPaint LsRenderer
                                      LsSdlMainWindow
                                      LsStuffing
                                      LsScene
                                      imgui
                                      LsVulkanLoader
                                      LsMath
                                      LsUtility
                                      SDL3::SDL3 )

add_dependencies( LustriousPaint Shaders )

# Windows finds DLLs next to the executable, elsewhere the build RPATH points at SDL
if (WIN32)
  add_custom_command(TARGET LustriousPaint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      "$<TARGET_FILE:SDL3::SDL3>"
      "$<TARGET_FILE_DIR:LustriousPaint>/")
endif()

add_custom_command(TARGET LustriousPaint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:LustriousPaint>/Shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/Shaders/Binary"
        "$<TARGET_FILE_DIR:LustriousPaint>/Shaders")

add_custom_command(TARGET LustriousPaint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Resources/Fonts"
        "$<TARGET_FILE_DIR:LustriousPaint>/Fonts")

set_target_properties( LustriousPaint PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:LustriousPaint>" )
