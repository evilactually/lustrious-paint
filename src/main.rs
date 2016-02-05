#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(unused_variables)]
#![allow(dead_code)]

extern crate win32;
extern crate ctypes;

use std::mem::{size_of};
use win32::*;
use ctypes::*;
use std::ffi::{CString};

unsafe extern "system" fn WndProc(hWnd: HWND, uMsg: UINT, wParam: WPARAM, lpParam: LPARAM) -> LRESULT {
    DefWindowProcA(hWnd, uMsg, wParam, lpParam)
}

fn WinMain(hInstance : HINSTANCE,
           lpCmdLine : LPTSTR,
           nCmdShow : c_int) {
    let class = CString::new("WindowClass").unwrap();
    let app_ico = CString::new("LUSTRIOUS_PAINT").unwrap();

    let wndclassex = WNDCLASSEX {
        cbSize:size_of::<WNDCLASSEX>() as u32,
        style: (CS_HREDRAW | CS_VREDRAW) as u32,
        lpfnWndProc: Some(WndProc),
        cbClsExtra: 0,
        cbWndExtra: 0,
        hInstance: hInstance,
        hIcon: LoadIcon(hInstance, app_ico.as_ptr() as LPCVOID),
        hCursor: LoadCursor(hInstance, 32513 as LPVOID),
        hbrBackground: GetStockObject(BLACK_BRUSH),
        lpszMenuName: NULL,
        lpszClassName: class.as_ptr() as LPCTSTR,
        hIconSm: LoadIcon(hInstance, app_ico.as_ptr() as LPCVOID)
    };

    let class_atom : ATOM = RegisterClassEx(&wndclassex);
    assert!(class_atom > 0);

    let title = CString::new("Lustrious Paint").unwrap();
    let wndstyle = WS_POPUP | 
                   WS_CLIPCHILDREN | 
                   WS_CLIPSIBLINGS | 
                   WS_SYSMENU | 
                   WS_THICKFRAME | 
                   WS_GROUP | /*WS_TABSTOP |*/ 
                   WS_BORDER | 
                   WS_MINIMIZEBOX | 
                   WS_MAXIMIZEBOX;

    let wnd:HWND = CreateWindowEx(WS_EX_APPWINDOW,
                   class_atom as LPCVOID,
                   title.as_ptr() as LPCTSTR,
                   WS_OVERLAPPEDWINDOW,
                   100,
                   100,
                   640,
                   480,
                   NULL,
                   NULL,
                   hInstance,
                   NULL);

    assert!(wnd != NULL);
    ShowWindow(wnd, if nCmdShow > 0 {nCmdShow} else {SW_SHOW});
    let mut msg: MSG = MSG::default();

    while GetMessage(&mut msg, NULL, 0, 0) > 0 {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

fn main() {
    let hInstance = GetModuleHandle(NULL);
    let lpCmdLine = GetCommandLine();
    let nCmdShow = GetStartupInfo().wShowWindow as c_int;
    WinMain(hInstance, lpCmdLine, nCmdShow);
}
