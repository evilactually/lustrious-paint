
#![allow(non_snake_case)]

use ::types::*;
use ::ctypes::*;

pub fn RegisterClassEx(lpWndClass: *const WNDCLASSEX) -> ATOM {
    unsafe {
        ::imports::RegisterClassExA(lpWndClass)
    }
}

pub fn LoadIcon(hInstance: HINSTANCE, lpIconName: LPCSTR) -> HICON {
    unsafe {
        ::imports::LoadIconA(hInstance, lpIconName)
    }
}

pub fn LoadCursor(hInstance: HINSTANCE, lpCursorName: LPCSTR) -> HICON {
    unsafe {
        ::imports::LoadCursorA(hInstance, lpCursorName)
    }
}

pub fn CreateWindowEx(dwExStyle: DWORD,
                      lpClassName: LPCTSTR,
                      lpWindowName: LPCTSTR,
                      dwStyle: DWORD,
                      x: c_int,
                      y: c_int,
                      nWidth: c_int,
                      nHeight: c_int,
                      hWndParent: HWND,
                      hMenu: HMENU,
                      hInstance: HINSTANCE,
                      lpParam: LPVOID) -> HWND {
    unsafe {
        ::imports::CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
    }
}

pub fn ShowWindow(hWnd: HWND, nCmdShow: c_int) -> BOOL {
    unsafe {
        ::imports::ShowWindow(hWnd, nCmdShow)
    }
}

pub fn GetMessage(lpMsg: LPMSG, hWnd: HWND, wMsgFilterMin: UINT, wMsgFilterMax: UINT) -> BOOL {
    unsafe {
        ::imports::GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax)
    }
}

pub fn TranslateMessage(lpMsg: *const MSG) -> BOOL {
    unsafe {
        ::imports::TranslateMessage(lpMsg)
    }
}

pub fn DispatchMessage(lpMsg: *const MSG) -> LRESULT {
    unsafe {
        ::imports::DispatchMessageA(lpMsg)
    }
}

pub fn GetModuleHandle(lpModuleName: LPCSTR) -> HMODULE {
    unsafe {
        ::imports::GetModuleHandleA(lpModuleName)
    }
}

pub fn GetStartupInfo() -> STARTUPINFO {
    unsafe {
        let mut startupinfo: STARTUPINFO = STARTUPINFO::default();
        ::imports::GetStartupInfoA(&mut startupinfo);
        startupinfo
    }   
}

pub fn GetCommandLine() -> LPTSTR {
    unsafe {
        ::imports::GetCommandLineA()
    }
}

pub fn GetLastError() -> DWORD {
    unsafe {
        ::imports::GetLastError()
    }
}

pub fn GetStockObject(i : c_int) -> HGDIOBJ {
    unsafe {
        ::imports::GetStockObject(i)
    }   
}
