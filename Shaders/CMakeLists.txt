cmake_minimum_required ( VERSION 3.5.2 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH} )

file(GLOB_RECURSE SHADER_SOURCE_FILES
    "*.frag"
    "*.vert"
    "*.comp")

source_group( "Source" FILES ${SHADER_SOURCE_FILES} )

find_package(VulkanSDK)

message("${GLSLANG_VALIDATOR}")

foreach(SHADER_SOURCE_FILE ${SHADER_SOURCE_FILES})
  get_filename_component(FILE_NAME ${SHADER_SOURCE_FILE} NAME)
  set(SHADER_BINARY_FILE "${CMAKE_CURRENT_BINARY_DIR}/Binary/${FILE_NAME}.spv")
  ADD_CUSTOM_COMMAND(OUTPUT
    ${SHADER_BINARY_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/Binary"
    COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SOURCE_FILE} -o ${SHADER_BINARY_FILE}
    DEPENDS ${SHADER_SOURCE_FILE})
  list(APPEND SHADER_BINARY_FILES ${SHADER_BINARY_FILE})
endforeach(SHADER_SOURCE_FILE)

add_custom_target(
  Shaders
  DEPENDS ${SHADER_BINARY_FILES}
  SOURCES ${SHADER_SOURCE_FILES})