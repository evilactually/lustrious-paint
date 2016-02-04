#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(unused_variables)]
#![allow(dead_code)]

extern crate win32;
extern crate ctypes;

use std::mem::{size_of};
use win32::*;
// use win32::consts::*;
// use win32::types::*;
// us*;
use ctypes::*;
use std::ffi::{CString};

static GLOB: i32 = 100i32;

unsafe extern "system" fn WndProc(wnd:HWND, a:UINT, b:WPARAM, c:LPARAM) -> LRESULT {
    //println!("{:?}", "+");
    DefWindowProcA(wnd, a, b, c)
}

// extern "C" {
//     pub fn adder(a:c_int,b:c_int) -> c_int;
// }

fn WinMain(hInstance : HINSTANCE,
           lpCmdLine : LPTSTR,
           nCmdShow : c_int) {
    let class = CString::new("WindowClass").unwrap();

    let wndclassex = WNDCLASSEX {
        cbSize:size_of::<WNDCLASSEX>() as u32,
        style: (CS_HREDRAW | CS_VREDRAW) as u32,
        lpfnWndProc: Some(WndProc),
        cbClsExtra: 0,
        cbWndExtra: 0,
        hInstance: hInstance,
        hIcon: NULL, //LoadIcon(hInstance, IDI_APPLICATION),
        hCursor: NULL, //LoadIcon(hInstance, IDC_ARROW),
        hbrBackground: GetStockObject(BLACK_BRUSH),
        lpszMenuName: NULL,//class.as_ptr() as LPCVOID,
        lpszClassName: class.as_ptr() as LPCVOID,
        hIconSm: NULL,// LoadIcon(hInstance, IDI_APPLICATION)
    };

    let class_atom : ATOM = RegisterClassEx(&wndclassex);
    println!("ERROR CODE: {:?}", GetLastError());
    println!("{:?}", wndclassex.style);
    println!("ATOM: {:?}", class_atom);
    let title = CString::new("L-tile").unwrap();
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
                   class.as_ptr() as LPCTSTR,
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
    println!("ERROR CODE: {:?}", GetLastError());
    println!("{:?}", wnd);
    let mut msg: MSG = MSG::default();
    println!("cmdshow: {:?}", nCmdShow);
    ShowWindow(wnd, SW_SHOW);

    while (GetMessage(&mut msg, NULL, 0, 0) > 0) {
      //println!("{:?}", "*");
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //GetMessage(&mut msg, NULL, 0, 0);
}

fn main() {
    let hInstance = GetModuleHandle(NULL);
    let lpCmdLine = GetCommandLine();
    let nCmdShow = GetStartupInfo().wShowWindow as c_int;
    WinMain(hInstance, lpCmdLine, nCmdShow);
    //println!("{:?}", hInstance);
    //println!("{:?}", unsafe{adder(2,2)});
}

 

 //    let class_atom : ATOM = unsafe {RegisterClassExA(&wndclassex)};
 //    println!("hello world {:?}", unsafe {GetModuleHandleA((0 as LPCSTR))});

 //    let mut startupinf: STARTUPINFO = STARTUPINFO::default();
 //    unsafe{GetStartupInfoA(&mut startupinf)};
 //    //println!("{:?}", startupinf);
 //    let cmdln = unsafe{::std::ffi::CStr::from_ptr(GetCommandLineA())};
 //    println!("{:?}", cmdln);
 //    //let title = unsafe{::std::ffi::CStr::from_ptr(startupinf.lpTitle)};
 //    //println!("{:?}", title);
 //    //WinMain(0 as HINSTANCE, "...", 0);