// BUG: nCmdShow == 0 when starting from terminal
// BUG: Windows spawns a game overlay over my window for unknown reason

#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(unused_variables)]
#![allow(dead_code)]

extern crate win32;
extern crate dx11;
#[macro_use]
extern crate ctypes;

use std::mem::{size_of};
use win32::*;
use ctypes::*;
use std::ffi::{CString};
use dx11::*;

pub const BORDER_WIDTH: DWORD = 8;
pub const CAPTION_HEGHT: DWORD = 40;
pub const WINDOW_WIDTH: c_int = 640;
pub const WINDOW_HEIGHT: c_int = 480;

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
        hCursor: LoadCursor(NULL!(), IDC_CROSS), // Use NULL for built-in icons
        hbrBackground: GetStockObject(BLACK_BRUSH),
        lpszMenuName: CNULL!(),
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
                   WINDOW_WIDTH,
                   WINDOW_HEIGHT,
                   NULL!(),
                   NULL!(),
                   hInstance,
                   NULL!());

    assert!(!wnd.is_null());
    ShowWindow(wnd, if nCmdShow > 0 {nCmdShow} else {SW_SHOW});

    let pFeatureLevels: [D3D_FEATURE_LEVEL; 2] = [D3D_FEATURE_LEVEL::DX_11_0, D3D_FEATURE_LEVEL::DX_11_1];
    //let p: *const D3D_FEATURE_LEVEL = &pFeatureLevels as *const D3D_FEATURE_LEVEL;
    
    let swap_chain_desc = DXGI_SWAP_CHAIN_DESC {
        BufferDesc: DXGI_MODE_DESC
        {
            Width: WINDOW_WIDTH as UINT,
            Height: WINDOW_HEIGHT as UINT,
            RefreshRate: DXGI_RATIONAL {
                Numerator: 0,
                Denominator: 0,
            },
            Format: DXGI_FORMAT::UNKNOWN, // FIXME: Don't know format yet
            ScanlineOrdering: DXGI_MODE_SCANLINE_ORDER::UNSPECIFIED,
            Scaling: DXGI_MODE_SCALING::STRETCHED
        },
        SampleDesc: DXGI_SAMPLE_DESC {
            Count: 1,
            Quality: 0 
        },
        BufferUsage: DXGI_USAGE_RENDER_TARGET_OUTPUT,
        BufferCount: 2,
        OutputWindow: wnd,
        Windowed: TRUE,
        SwapEffect: DXGI_SWAP_EFFECT::DISCARD,
        Flags: 0
    };

   let mut pDevice: *const ID3D11Device = std::ptr::null();
   let mut feature_level: *const D3D_FEATURE_LEVEL = std::ptr::null();
   let mut pImmediateContext: *const ID3D11DeviceContext = std::ptr::null();
   let mut p_swap_chain: *const IDXGISwapChain = std::ptr::null();

   unsafe {
   let hresult = D3D11CreateDeviceAndSwapChain(NULL!(), 
                                               D3D_DRIVER_TYPE::HARDWARE,
                                               NULL!(),
                                               0,
                                               &pFeatureLevels as *const D3D_FEATURE_LEVEL,
                                               pFeatureLevels.len() as u32,
                                               D3D11_SDK_VERSION,
                                               &swap_chain_desc as *const DXGI_SWAP_CHAIN_DESC,
                                               &mut p_swap_chain as *mut *const IDXGISwapChain,
                                               &mut pDevice as *mut *const ID3D11Device,
                                               &mut feature_level as *mut *const D3D_FEATURE_LEVEL,
                                               &mut pImmediateContext as *mut *const ID3D11DeviceContext);
    };

    let mut msg: MSG = MSG::default();
    while GetMessage(&mut msg, NULL!(), 0, 0) > 0 {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

macro_rules! three {
    () => { 3 }
}


fn main() {
    let hInstance = GetModuleHandle(NULL!());
    let lpCmdLine = GetCommandLine();
    let nCmdShow = GetStartupInfo().wShowWindow as c_int;

    let d = unsafe {dx11::GetDevice()};
    println!("{:?}", unsafe {((*d).square)(2)});

    WinMain(hInstance, lpCmdLine, nCmdShow);
}
