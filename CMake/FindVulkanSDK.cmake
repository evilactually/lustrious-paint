# Find VulkanSDK
#
# VULKAN_INCLUDE_DIR
# VULKAN_LIBRARY
# SLANGC
# VULKAN_FOUND

if (WIN32)
  find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
    "$ENV{VULKAN_SDK}/Include"
    "$ENV{VK_SDK_PATH}/Include")
  if (CMAKE_CL_64)
    find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS
      "$ENV{VULKAN_SDK}/Bin"
      "$ENV{VULKAN_SDK}/Lib"
      "$ENV{VK_SDK_PATH}/Bin"
      "$ENV{VK_SDK_PATH}/Lib")
    find_program(SLANGC NAMES slangc HINTS
      "$ENV{VULKAN_SDK}/Bin"
      "$ENV{VK_SDK_PATH}/Bin")
  else()
    find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS
      "$ENV{VULKAN_SDK}/Bin32"
      "$ENV{VULKAN_SDK}/Lib32"
      "$ENV{VK_SDK_PATH}/Bin32"
      "$ENV{VK_SDK_PATH}/Lib32")
    find_program(SLANGC NAMES slangc HINTS
      "$ENV{VULKAN_SDK}/Bin32"
      "$ENV{VK_SDK_PATH}/Bin32")
  endif()
else()
  find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
    "$ENV{VULKAN_SDK}/include")
  find_library(VULKAN_LIBRARY NAMES vulkan HINTS
    "$ENV{VULKAN_SDK}/lib")
  find_program(SLANGC NAMES slangc HINTS
    "$ENV{VULKAN_SDK}/bin")
endif()

# Fall back to the SDK the Vulkan library was found in, in case VULKAN_SDK
# isn't set in the environment CMake is regenerated from (e.g. Visual Studio).
if (VULKAN_LIBRARY AND NOT SLANGC)
  get_filename_component(VULKAN_LIBRARY_DIR "${VULKAN_LIBRARY}" DIRECTORY)
  find_program(SLANGC NAMES slangc HINTS
    "${VULKAN_LIBRARY_DIR}/../Bin"
    "${VULKAN_LIBRARY_DIR}/../bin")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan DEFAULT_MSG VULKAN_LIBRARY
 VULKAN_INCLUDE_DIR
 SLANGC)

mark_as_advanced(VULKAN_INCLUDE_DIR
 VULKAN_LIBRARY
 SLANGC)