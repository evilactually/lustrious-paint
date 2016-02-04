
use types::*;
use ctypes::*;

extern "system" {
    pub fn DefWindowProcA(hWnd: HWND, uMsg: UINT, wParam: WPARAM, lpParam: LPARAM) -> LRESULT;
}

#[link(name = "user32")]
#[allow(non_snake_case)]
extern "system" {
    pub fn RegisterClassExA(lpWndClass: *const WNDCLASSEX) -> ATOM;
    pub fn LoadIconA(hInstance: HINSTANCE, lpIconName: LPCSTR) -> HICON;
    pub fn CreateWindowExA(dwExStyle: DWORD,
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
                           lpParam: LPVOID) -> HWND;
    pub fn ShowWindow(hWnd: HWND, nCmdShow: c_int) -> BOOL;
    pub fn GetMessageA(lpMsg: LPMSG, hWnd: HWND, wMsgFilterMin: UINT, wMsgFilterMax: UINT) -> BOOL;
    pub fn TranslateMessage(lpMsg: *const MSG) -> BOOL;
    pub fn DispatchMessageA(lpMsg: *const MSG) -> LRESULT;
}

#[link(name = "kernel32")]
extern "system" {
    pub fn GetModuleHandleA(lpModuleName: LPCSTR) -> HMODULE;
    pub fn GetStartupInfoA(lpStartupInfo: LPSTARTUPINFO);
    pub fn GetCommandLineA() -> LPTSTR;
    pub fn GetLastError() -> DWORD;
}

#[link(name = "gdi32")]
extern "system" {
    pub fn GetStockObject(i : c_int) -> HGDIOBJ;
}
