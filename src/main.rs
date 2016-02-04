
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

// extern "C" {
//     pub fn adder(a:c_int,b:c_int) -> c_int;
// }



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
        lpszMenuName: NULL,//class.as_ptr() as LPCVOID,
        lpszClassName: class.as_ptr() as LPCVOID,
        hIconSm: LoadIcon(hInstance, IDI_APPLICATION)
    };

    let class_atom : ATOM = RegisterClassEx(&wndclassex);
    println!("{:?}", wndclassex.style);
    println!("{:?}", class_atom);
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

    CreateWindowEx(WS_EX_APPWINDOW,
                   class.as_ptr() as LPCTSTR,
                   title.as_ptr() as LPCTSTR,
                   wndstyle,
                   100,
                   100,
                   640,
                   480,
                   NULL,
                   NULL,
                   hInstance,
                   NULL);

    let mut msg: MSG = MSG::default();
    println!("{:?}", class_atom);
    //GetMessage(&mut msg, NULL, 0, 0);
}

fn main() {
    let hInstance = GetModuleHandle(NULL);
    let lpCmdLine = GetCommandLine();
    let nCmdShow = GetStartupInfo().wShowWindow;
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