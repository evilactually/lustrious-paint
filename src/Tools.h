#pragma once

#include "vulkan/vk_cpp.hpp"
#include <windows.h>
#include <string>

namespace Ls::Vulkan::Utils {
    void ExitFatal(std::string message, std::string caption) 
    {
        MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
        exit(1);
    }

    VkBool32 getSupportedDepthFormat(vk::PhysicalDevice physicalDevice, vk::Format* depthFormat)
    {
        // Since all depth formats may be optional, we need to find a suitable depth format to use
        // Start with the highest precision packed format
        std::vector<vk::Format> depthFormats = { 
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD32Sfloat,
            vk::Format::eD24UnormS8Uint,
            vk::Format::eD16UnormS8Uint, 
            vk::Format::eD16Unorm
        };

        for (auto& format : depthFormats)
        {
            vk::FormatProperties formatProps;
            physicalDevice.getFormatProperties(format, &formatProps);
            // Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            {
                *depthFormat = format;
                return true;
            }
        }

        return false;
    }

#ifdef _DEBUG
#include <stdlib.h>
#include <stdarg.h>
#endif

    static void tracef(LPCTSTR lpszFormat, ...)
    {
#ifdef _DEBUG
        static const int BUFFERSIZE = 0x800;
        TCHAR    lpszBuffer[BUFFERSIZE];
        va_list  fmtList;

        va_start(fmtList, lpszFormat);
        vsprintf_s(lpszBuffer, lpszFormat, fmtList);
        va_end(fmtList);

        ::OutputDebugString(lpszBuffer);
#endif
    }
}