#include <windows.h>
#include "Vulkan.hpp"

//void assert(bool flag, char *msg = "") {
//    if (!flag) {
//        OutputDebugStringA("ASSERT: ");
//        OutputDebugStringA(msg);
//        OutputDebugStringA("\n");
//        int *base = 0;
//        *base = 1;
//    }
//}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE: {
        PostQuitMessage(0);
        break;
    }
    case WM_PAINT: {
        break;
    }
    default: {
        break;
    }
    }

    // a pass-through for now. We will return to this callback
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASSEX windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = "VulkanWindowClass";
    RegisterClassEx(&windowClass);

    int width = 800;
    int height = 600;

    HWND windowHandle = CreateWindowEx(NULL,
        "VulkanWindowClass",
        "Core",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100,
        100,
        width,
        height,
        NULL,
        NULL,
        hInstance,
        NULL);

    MSG msg;
    bool done = false;
    while (!done) {
        PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);
        if (msg.message == WM_QUIT) {
            done = true;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RedrawWindow(windowHandle, NULL, NULL, RDW_INTERNALPAINT);
    }

    return msg.wParam;
}

