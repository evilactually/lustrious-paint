#pragma once

#include <windows.h>
#include <string>
#include <assert.h>

namespace Ls {
    namespace Utils {
        void ExitFatal(std::string message, std::string caption)
        {
            MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
            exit(1);
        }

        bool RegKeyExist(_In_     HKEY    hKey,
                         _In_opt_ LPCTSTR lpSubKey) {
            HKEY key;
            bool exists = RegOpenKeyEx(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS;
            RegCloseKey(key);
            return exists;
        }

        bool CheckBit(int bits, int bit) {
            return (bits & bit) == bit;
        }
    }
}

#define Assert(flag, msg)\
    if((msg) && !(flag)) {\
        OutputDebugStringA("ASSERT: ");\
        OutputDebugStringA(msg);\
        OutputDebugStringA("\n");\
    }\
    assert(flag);

#define CheckVulkanResult(result, msg)\
    Assert((result) == VK_SUCCESS, msg)
