cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

file(GLOB_RECURSE SHADER_SOURCE_FILES "*.slang")

source_group( "Source" FILES ${SHADER_SOURCE_FILES} )

find_package(VulkanSDK)

if (NOT SLANGC)
  message(FATAL_ERROR "slangc not found. Install the Vulkan SDK (1.3.296+) and set VULKAN_SDK.")
endif()

# Each Slang module can hold several entry points; every entry point is
# compiled into its own SPIR-V binary named <module>.<suffix>.spv.
# The application targets Vulkan 1.0, hence SPIR-V 1.0.
function(add_slang_entry_point MODULE ENTRY STAGE SUFFIX)
  set(SHADER_SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}.slang")
  set(SHADER_BINARY_FILE "${CMAKE_CURRENT_BINARY_DIR}/Binary/${MODULE}.${SUFFIX}.spv")
  ADD_CUSTOM_COMMAND(OUTPUT
    ${SHADER_BINARY_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/Binary"
    COMMAND ${SLANGC} ${SHADER_SOURCE_FILE}
            -target spirv -profile spirv_1_0 -warnings-disable 50011
            -entry ${ENTRY} -stage ${STAGE}
            -o ${SHADER_BINARY_FILE}
    # Modules import each other, so any change rebuilds every entry point
    DEPENDS ${SHADER_SOURCE_FILES})
  set(SHADER_BINARY_FILES ${SHADER_BINARY_FILES} ${SHADER_BINARY_FILE} PARENT_SCOPE)
endfunction()

add_slang_entry_point(line  vertexMain   vertex   vert)
add_slang_entry_point(line  fragmentMain fragment frag)
add_slang_entry_point(point vertexMain   vertex   vert)
add_slang_entry_point(point fragmentMain fragment frag)
add_slang_entry_point(image vertexMain   vertex   vert)
add_slang_entry_point(image fragmentMain fragment frag)
add_slang_entry_point(canvas paintMain   compute  paint.comp)
add_slang_entry_point(canvas blendMain   compute  blend.comp)
add_slang_entry_point(canvas pickMain    compute  pick.comp)
add_slang_entry_point(canvas rigCountMain compute rigcount.comp)
add_slang_entry_point(canvas rangeMain   compute  range.comp)
add_slang_entry_point(canvas renderMain  compute  render.comp)
add_slang_entry_point(canvas resolveMain compute  resolve.comp)
add_slang_entry_point(viewport computeMain compute comp)
add_slang_entry_point(sceneline vertexMain   vertex   vert)
add_slang_entry_point(sceneline fragmentMain fragment frag)

add_custom_target(
  Shaders
  DEPENDS ${SHADER_BINARY_FILES}
  SOURCES ${SHADER_SOURCE_FILES})
