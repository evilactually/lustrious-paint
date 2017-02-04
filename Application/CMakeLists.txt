cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

set( LIBRARY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Libraries/")

include( BuildInfo )

set(INCLUDE_FILES
  "Include/Version.h.in")

set(SOURCE_FILES
  "${CMAKE_SOURCE_DIR}/Resources/Version.rc.in"
  "Source/Main.cpp"
  "Source/LsBrushRig.cpp"
  "Source/LsPointGrid.cpp"
  "Source/LsUselessBox.cpp")

set(RESOURCE_FILES
  "${CMAKE_BINARY_DIR}/Resources/Version.rc"
  "${CMAKE_SOURCE_DIR}/Resources/Icon.rc")

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

configure_file(
  "${CMAKE_SOURCE_DIR}/Resources/Version.rc.in"
  "${CMAKE_BINARY_DIR}/Resources/Version.rc")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Include/Version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Include/Version.h")

find_package( FreeImage REQUIRED )
find_package( VulkanSDK REQUIRED )

add_executable( LustriousPaint WIN32 ${INCLUDE_FILES} ${SOURCE_FILES} ${RESOURCE_FILES} )

target_include_directories( LustriousPaint PUBLIC Include
                                                  "${CMAKE_SOURCE_DIR}/Include"
                                                  "${CMAKE_CURRENT_BINARY_DIR}/Include/"
                                                  "${LIBRARY_SOURCE_DIR}/LsRenderer/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsWin32MainWindow/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsMath/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsUtility/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsVulkanLoader/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsWintabLoader/Include"
                                                  "${LIBRARY_SOURCE_DIR}/LsVulkanSystem/Include"
                                                  ${VULKAN_INCLUDE_DIR}
                                                  ${FREEIMAGE_INCLUDE_DIR} )

target_link_libraries( LustriousPaint LsMath
                                      LsUtility
                                      LsVulkanLoader
                                      LsWintabLoader
                                      LsWin32MainWindow
                                      LsRenderer
                                      LsStuffing
                                      ${FREEIMAGE_LIBRARY_STATIC} )

add_dependencies( LustriousPaint Shaders )

add_custom_command(TARGET LustriousPaint POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    "${FREEIMAGE_LIBRARY_SHARED}"
    "$<TARGET_FILE_DIR:LustriousPaint>/")

add_custom_command(TARGET LustriousPaint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:LustriousPaint>/Shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/Shaders/Binary"
        "$<TARGET_FILE_DIR:LustriousPaint>/Shaders")

file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user" "\
<?xml version=\"1.0\" encoding=\"utf-8\"?>
<Project ToolsVersion=\"14.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">
 <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">
   <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
   <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
 </PropertyGroup>
 <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">
   <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
   <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
</Project>\
")
