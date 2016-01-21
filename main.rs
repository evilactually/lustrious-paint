
extern crate win32;
extern crate ctypes;

use std::mem::{size_of};
use win32::*;
// use win32::consts::*;
// use win32::types::*;
// us*;
use ctypes::*;
use std::ffi::{CString};

static glob: i32 = 100i32;

unsafe extern "system" fn winproc(wnd:HWND, a:UINT, b:WPARAM, c:LPARAM) -> LRESULT {
    0
}

fn WinMain(hInstance : HINSTANCE,
           lpCmdLine : LPTSTR,
           nCmdShow : WORD) {
    let class = CString::new("WindowClass").unwrap();
    let wndclassex = WNDCLASSEX {
        cbSize:size_of::<WNDCLASSEX>() as u32,
        style: (CS_HREDRAW | CS_VREDRAW) as u32,
        lpfnWndProc: Some(winproc),
        cbClsExtra: 0,
        cbWndExtra: 0,
        hInstance: hInstance,
        hIcon: LoadIcon(hInstance, IDI_APPLICATION),
        hCursor: LoadIcon(hInstance, IDC_ARROW),
        hbrBackground: GetStockObject(BLACK_BRUSH),
        lpszMenuName: class.as_ptr() as LPCVOID,
        lpszClassName: NULL,
        hIconSm: LoadIcon(hInstance, IDI_APPLICATION)
    };
    let class_atom : ATOM = RegisterClassEx(&wndclassex);

    let mut msg: MSG = MSG::default();
    //GetMessage(&mut msg, NULL, 0, 0);
}

fn main() {
    let hInstance = GetModuleHandle(NULL);
    let lpCmdLine = GetCommandLine();
    let nCmdShow = GetStartupInfo().wShowWindow;
    WinMain(hInstance, lpCmdLine, nCmdShow);
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