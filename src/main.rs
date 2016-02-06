// BUG: nCmdShow == 0 when starting from terminal
// BUG: Windows spawns a game overlay over my window for unknown reason

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

pub const BORDER_WIDTH: DWORD = 8;
pub const CAPTION_HEGHT: DWORD = 40;

unsafe extern "system" fn WndProc(hWnd: HWND, uMsg: UINT, wParam: WPARAM, lpParam: LPARAM) -> LRESULT {
    match uMsg {
        // caption and border
        WM_NCHITTEST => {
            let wnd_rect = GetWindowRect(hWnd);
            let x = get_x_lparam(lpParam);
            let y = get_y_lparam(lpParam);

            //bottom left corner
            if x >= wnd_rect.left && x < wnd_rect.left + BORDER_WIDTH &&
               y < wnd_rect.bottom && y >= wnd_rect.bottom - BORDER_WIDTH
            {
                return HTBOTTOMLEFT;
            }
            //bottom right corner
            if x < wnd_rect.right && x >= wnd_rect.right - BORDER_WIDTH &&
               y < wnd_rect.bottom && y >= wnd_rect.bottom - BORDER_WIDTH
            {
                return HTBOTTOMRIGHT;
            }
            //top left corner
            if x >= wnd_rect.left && x < wnd_rect.left + BORDER_WIDTH &&
               y >= wnd_rect.top && y < wnd_rect.top + BORDER_WIDTH
            {
                return HTTOPLEFT;
            }
            //top right corner
            if x < wnd_rect.right && x >= wnd_rect.right - BORDER_WIDTH &&
               y >= wnd_rect.top && y < wnd_rect.top + BORDER_WIDTH
            {
                return HTTOPRIGHT;
            }
            //left border
            if x >= wnd_rect.left && x < wnd_rect.left + BORDER_WIDTH
            {
                return HTLEFT;
            }
            //right border
            if x < wnd_rect.right && x >= wnd_rect.right - BORDER_WIDTH
            {
                return HTRIGHT;
            }
            //bottom border
            if y < wnd_rect.bottom && y >= wnd_rect.bottom - BORDER_WIDTH
            {
                return HTBOTTOM;
            }
            //top border
            if y >= wnd_rect.top && y < wnd_rect.top + BORDER_WIDTH
            {
                return HTTOP;
            }
            //caption
            if y >= wnd_rect.top && y < wnd_rect.top + CAPTION_HEGHT
            {
                return HTCAPTION;
            }

            HTCLIENT
        },
        // overdraw
        WM_NCCALCSIZE => {
            if wParam > 0 {
                return 0;
            }
            DefWindowProcA(hWnd, uMsg, wParam, lpParam)
        },
        WM_DESTROY => {
            PostQuitMessage(0);
            0
        },
        _ => DefWindowProcA(hWnd, uMsg, wParam, lpParam),
    }
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
        hIcon: LoadIcon(hInstance, app_ico.as_ptr() as LPCSTR),
        hCursor: LoadCursor(NULL, IDC_CROSS), // Use NULL for built-in icons
        hbrBackground: GetStockObject(BLACK_BRUSH),
        lpszMenuName: NULL,
        lpszClassName: class.as_ptr() as LPCTSTR,
        hIconSm: LoadIcon(hInstance, app_ico.as_ptr() as LPCVOID)
    };

    let class_atom : ATOM = RegisterClassEx(&wndclassex);
    assert!(class_atom > 0);

    let title = CString::new("Lustrious Paint").unwrap();
    let wnd_style = WS_POPUP | 
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
                   wnd_style,
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
