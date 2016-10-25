cmake_minimum_required (VERSION 3.5.2)
project (LustriousPaint)

# Return a list of all subdirectories containing headers
MACRO(HEADER_DIRECTORIES scan_dir return_list )
    FILE(GLOB_RECURSE new_list "${scan_dir}/*.h" "${scan_dir}/*.hpp")
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

# Custom modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Build numbers
set (VERSION_BUILD_x86 1)
set (VERSION_BUILD_x86_64 1)

# The version number
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (VERSION_REVISION 0)
if (CMAKE_CL_64)
  set (VERSION_BUILD ${VERSION_BUILD_x86_64})
else()
  set (VERSION_BUILD ${VERSION_BUILD_x86})
endif()

# File info
set(COMPANY_NAME "Lustrious Labs Ltd")
set(FILE_DESCRIPTION "Lustrious Paint")
set(FILE_VERSION "1.0.0")
set(INTERNAL_NAME "LustriousPaint")
set(LEGAL_COPYRIGHT "Copyright (C) 2015-2016 Vladislav Shcherbakov")
set(ORIGINAL_FILENAME "${INTERNAL_NAME}.exe")
set(PRODUCT_NAME "Lustrious Paint")
set(PRODUCT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}.${VERSION_BUILD}")

# Configure version resource file
configure_file (
  "${PROJECT_SOURCE_DIR}/resources/Version.rc.in"
  "${PROJECT_BINARY_DIR}/resources/Version.rc"
  )

# Configure version header file
configure_file (
  "${PROJECT_SOURCE_DIR}/src/version.h.in"
  "${PROJECT_BINARY_DIR}/src/version.h"
  )

# Depricated. Simplify builds, store external libraries in external/ folder, manually update them.
# Dowload C++ Vulkan wrapper
# file(DOWNLOAD "https://raw.githubusercontent.com/nvpro-pipeline/vkcpp/b81f8ec376047f488b90182a7f291a2664fe6779/vulkan/vk_cpp.hpp" "${PROJECT_BINARY_DIR}/include/vulkan/vk_cpp.hpp")

# Glob all project source files
file(GLOB_RECURSE SOURCE_FILES
  "src/*.h"
  "src/*.cpp"
  "src/*.hpp"
  "src/*.inl"
  "${PROJECT_BINARY_DIR}/resources/*.rc"
  "${PROJECT_BINARY_DIR}/src/*.h"
  "${PROJECT_BINARY_DIR}/src/*.hpp"
  "resources/*.rc"
)

# Include all directories from sources that contain headers
HEADER_DIRECTORIES("src" INCLUDE_DIRS)
include_directories(${INCLUDE_DIRS})

# Include external headers
include_directories("include/")

message("${INCLUDE_DIRS}")

# Definitions
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES)

# Find Vulkan SDK
find_package(Vulkan)
if(VULKAN_FOUND)
  include_directories(${VULKAN_INCLUDE_DIR})
  message(${VULKAN_LIBRARY})
else()
  message(FATAL_ERROR "Vulkan SDK not found")
endif()

# Include generated headers
include_directories("${PROJECT_BINARY_DIR}/src/")

# Set link path
if(CMAKE_CL_64) # TODO: Why does CMAKE_SYSTEM_PROCESSOR return AMD64 even for 32 bit?
  set(LIBRARY_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/x64/")
else()
  set(LIBRARY_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/x32/")
endif()

link_directories("${LIBRARY_DIRECTORY}")

add_executable(LustriousPaint WIN32 ${SOURCE_FILES})
target_link_libraries (LustriousPaint ${VULKAN_LIBRARY})

# Link FreeImage
target_link_libraries (LustriousPaint "FreeImage")

# Copy Freemake.dll
add_custom_command(TARGET LustriousPaint POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    "${LIBRARY_DIRECTORY}/FreeImage.dll"
    "$<TARGET_FILE_DIR:LustriousPaint>/")

### Shaders Target ###

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

message(STATUS ${GLSL_VALIDATOR})

file(GLOB_RECURSE SHADER_SOURCE_FILES
    "shaders/*.frag"
    "shaders/*.vert")

foreach(SHADER_SOURCE_FILE ${SHADER_SOURCE_FILES})
  get_filename_component(FILE_NAME ${SHADER_SOURCE_FILE} NAME)
  set(SHADER_BINARY_FILE "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  ADD_CUSTOM_COMMAND(OUTPUT
    ${SHADER_BINARY_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSL_VALIDATOR} -V ${SHADER_SOURCE_FILE} -o ${SHADER_BINARY_FILE}
    DEPENDS ${SHADER_SOURCE_FILE})
  list(APPEND SHADER_BINARY_FILES ${SHADER_BINARY_FILE})
endforeach(SHADER_SOURCE_FILE)

add_custom_target(
    Shaders 
    DEPENDS ${SHADER_BINARY_FILES}
    )

### Shaders Target ###

add_dependencies(LustriousPaint Shaders)

# Copy compiled shaders to executable directory
add_custom_command(TARGET LustriousPaint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:LustriousPaint>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:LustriousPaint>/shaders")

### Tests ###

file(GLOB_RECURSE TESTS_SOURCE_FILES
  "tests/*.h"
  "tests/*.cpp"
  "tests/*.hpp"
  "tests/*.inl"
)

list(APPEND TESTS_SOURCE_FILES ${SOURCE_FILES})

add_executable(Tests ${TESTS_SOURCE_FILES})
set_property(TARGET Tests PROPERTY COMPILE_DEFINITIONS "TESTING")

# Link FreeImage
target_link_libraries (Tests "FreeImage")

# Copy Freemake.dll
add_custom_command(TARGET Tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    "${LIBRARY_DIRECTORY}/FreeImage.dll"
    "$<TARGET_FILE_DIR:LustriousPaint>/")