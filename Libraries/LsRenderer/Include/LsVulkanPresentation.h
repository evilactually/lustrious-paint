#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

void CreatePresentationSurface(vk::Instance instance, HINSTANCE hInstance, HWND windowHandle, vk::SurfaceKHR* presentationSurface) {
  vk::Win32SurfaceCreateInfoKHR surface_create_info(
    vk::Win32SurfaceCreateFlagsKHR(), // vk::Win32SurfaceCreateFlagsKHR   flags
    hInstance,                        // HINSTANCE                        hinstance
    windowHandle                      // HWND                             hwnd
  );

  if( instance.createWin32SurfaceKHR( &surface_create_info, nullptr, presentationSurface ) != vk::Result::eSuccess ) {
    throw std::string("Could not create presentation surface!");
  }
}