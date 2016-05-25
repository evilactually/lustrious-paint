#include <vulkan/vulkan.h>
#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include "WindowState.h"
#include "Utils.h"
#include "Vulkan.h"

namespace Ls {

    // WS_CAPTION is needed to make maximizing behave properly
    int const WS_BORDERLESS = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
                              | WS_SYSMENU | WS_THICKFRAME | WS_GROUP | /*WS_TABSTOP |*/ WS_BORDER
                              | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION;
    int const BORDER_WIDTH = 8;
    int const CAPTION_HEIGHT = 50;

    LRESULT CALLBACK WndProc(
        HWND    hWnd,
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam)
    {
        switch(uMsg) {
            case WM_NCHITTEST: {
                RECT winrect;
                GetWindowRect(hWnd, &winrect);
                long x = GET_X_LPARAM(lParam);
                long y = GET_Y_LPARAM(lParam);

                //bottom left corner
                if (x >= winrect.left && x < winrect.left + BORDER_WIDTH &&
                    y < winrect.bottom && y >= winrect.bottom - BORDER_WIDTH)
                {
                    return HTBOTTOMLEFT;
                }
                //bottom right corner
                if (x < winrect.right && x >= winrect.right - BORDER_WIDTH &&
                    y < winrect.bottom && y >= winrect.bottom - BORDER_WIDTH)
                {
                    return HTBOTTOMRIGHT;
                }
                //top left corner
                if (x >= winrect.left && x < winrect.left + BORDER_WIDTH &&
                    y >= winrect.top && y < winrect.top + BORDER_WIDTH)
                {
                    return HTTOPLEFT;
                }
                //top right corner
                if (x < winrect.right && x >= winrect.right - BORDER_WIDTH &&
                    y >= winrect.top && y < winrect.top + BORDER_WIDTH)
                {
                    return HTTOPRIGHT;
                }
                //left border
                if (x >= winrect.left && x < winrect.left + BORDER_WIDTH)
                {
                    return HTLEFT;
                }
                //right border
                if (x < winrect.right && x >= winrect.right - BORDER_WIDTH)
                {
                    return HTRIGHT;
                }
                //bottom border
                if (y < winrect.bottom && y >= winrect.bottom - BORDER_WIDTH)
                {
                    return HTBOTTOM;
                }
                //top border
                if (y >= winrect.top && y < winrect.top + BORDER_WIDTH)
                {
                    return HTTOP;
                }
                // caption
                if (y >= winrect.top && y < winrect.top + CAPTION_HEIGHT)
                {
                    return HTCAPTION;
                }
                // client
                return HTCLIENT;
            }
            break;
            case WM_NCCALCSIZE:
            //overdraw border
            if (wParam) {
                return 0;
            }
            break;
            case WM_DESTROY:
            // close the application entirely
            PostQuitMessage(0);
            break;
            case WM_MOVE: 
            if(!IsIconic(hWnd)) { // Do not update position while minimized
                long x = GET_X_LPARAM(lParam);
                long y = GET_Y_LPARAM(lParam);
                StoreWindowState({x,y,0,0}, WINDOW_STATE_UPDATE_X_FLAG | WINDOW_STATE_UPDATE_Y_FLAG);
            }
            break;
            case WM_SIZE:
            if (wParam == SIZE_MAXIMIZED)
            {
                WindowState state;
                state.mode = WINDOW_MODE_MAXIMIZED;
                StoreWindowState(state, WINDOW_STATE_UPDATE_MODE_FLAG);
            }
            else if (wParam == SIZE_MINIMIZED)
            {
                WindowState state;
                state.mode = WINDOW_MODE_MINIMIZED;
                StoreWindowState(state, WINDOW_STATE_UPDATE_MODE_FLAG);
            } else if (wParam == SIZE_RESTORED)
            {
                WORD width = LOWORD(lParam);
                WORD height = HIWORD(lParam);
                StoreWindowState({0,0,width,height,WINDOW_MODE_NORMAL}, WINDOW_STATE_UPDATE_WIDTH_FLAG 
                                                                        | WINDOW_STATE_UPDATE_HEIGHT_FLAG 
                                                                        | WINDOW_STATE_UPDATE_MODE_FLAG );
            }
            break;
            case WM_GETMINMAXINFO: {
                HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
                if(!monitor) break; // window may be iconified
                MONITORINFO monitorInfo;
                monitorInfo.cbSize = sizeof(MONITORINFO);
                int result = GetMonitorInfo(monitor, &monitorInfo);
                Assert(result,"GetMonitorInfo");
                MINMAXINFO* minmaxinfo = (MINMAXINFO*)lParam;
                minmaxinfo->ptMaxSize.y = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
                return 0;
            }
            break;
            case WM_LBUTTONDOWN:
            ShowWindow(hWnd, SW_MINIMIZE);
            break;
            default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
}

using namespace Ls;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
    WindowState windowState;
    if(!LoadWindowState(&windowState)){
        windowState.x = 100;
        windowState.y = 100;
        windowState.width = 640;
        windowState.height = 480;
        windowState.mode = WINDOW_MODE_NORMAL;
    }
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = LoadIcon(hInstance, "LUSTRIOUS_PAINT");
    windowClass.hIconSm = LoadIcon(hInstance, "LUSTRIOUS_PAINT");
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    windowClass.lpszClassName = "WindowClass";
    ATOM ca = RegisterClassEx(&windowClass);

    HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
    "WindowClass",        // class name
    "Lustrious Paint",    // title
    WS_BORDERLESS,        // style
    windowState.x,        // x-position
    windowState.y,        // y-position
    windowState.width,    // width
    windowState.height,   // height
    NULL,                 // no parent
    NULL,                 // no menus
    hInstance,
    NULL);

    Vulkan::Initialize();

    ShowWindow(hwnd, static_cast<int>(windowState.mode));

    // this struct holds Windows event messages
    MSG msg;
    bool done = false;
    // wait for the next message in the queue, store the result in 'msg'
    while (!done)
    {
        PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);
        if (msg.message == WM_QUIT)
        {
            done = true;
        } else {
            // translate keystroke messages into the right format
            TranslateMessage(&msg);
            // send the message to the WindowProc function
            DispatchMessage(&msg);
        }

        RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT);
    }

    return msg.wParam;
}