#include <windows.h>

void GetVulkanPixelDimensions(HWND windowHandle, float* width, float* height) {
  RECT clientRect;
  ::GetClientRect(windowHandle, &clientRect);

  if (width) {
    *width = 2.0f/(float)(clientRect.right - clientRect.left);
  }

  if (height) {
    *height = 2.0f/(float)(clientRect.bottom - clientRect.top);
  }
}