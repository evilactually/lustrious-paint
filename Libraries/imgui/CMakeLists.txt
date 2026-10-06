cmake_minimum_required ( VERSION 3.16 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

# Dear ImGui v1.92.9b (docking branch), vendored from https://github.com/ocornut/imgui
# Only the core library and the SDL3 + Vulkan backends are kept.

set(INCLUDE_FILES
  imconfig.h
  imgui.h
  imgui_internal.h
  imstb_rectpack.h
  imstb_textedit.h
  imstb_truetype.h
  backends/imgui_impl_sdl3.h
  backends/imgui_impl_vulkan.h)

set(SOURCE_FILES
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_tables.cpp
  imgui_widgets.cpp
  backends/imgui_impl_sdl3.cpp
  backends/imgui_impl_vulkan.cpp)

source_group( "Include" FILES ${INCLUDE_FILES} )
source_group( "Source" FILES ${SOURCE_FILES} )

find_package( VulkanSDK )

add_library( imgui STATIC ${INCLUDE_FILES} ${SOURCE_FILES} )

target_include_directories( imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
                                         "${CMAKE_CURRENT_SOURCE_DIR}/backends"
                                         ${VULKAN_INCLUDE_DIR} )

# Vulkan entry points are loaded dynamically (see LsVulkanLoader), so the backend
# must not link against Vulkan prototypes; the renderer hands it a loader instead.
target_compile_definitions( imgui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES )

target_link_libraries( imgui PUBLIC SDL3::SDL3 )
