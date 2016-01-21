
extern crate win32;
extern crate ctypes;

use std::mem::{size_of};
use win32::consts::*;
use win32::types::*;
use win32::imports::*;
use ctypes::*;
use std::ffi::{CString};

unsafe extern "system" fn winproc(wnd:HWND, a:UINT, b:WPARAM, c:LPARAM) -> LRESULT {
    0
}

fn WinMain(hInstance : HINSTANCE,
           lpCmdLine : LPTSTR,
           nCmdShow : WORD) {

}

fn main() {
    let hInstance = unsafe {GetModuleHandleA(0 as LPCSTR)} as HINSTANCE;
    let lpCmdLine = unsafe{GetCommandLineA()};
    let nCmdShow = {let mut inf: STARTUPINFO = STARTUPINFO::default();
                    unsafe{GetStartupInfoA(&mut inf)};
                    inf.wShowWindow};
   WinMain(hInstance, lpCmdLine, nCmdShow);
}

 // let class = CString::new("WindowClass").unwrap();
 //    let instance = unsafe {GetModuleHandleA((0 as LPCSTR))};
 //    let wndclassex = WNDCLASSEX {
 //        cbSize:size_of::<WNDCLASSEX>() as u32,
 //        style: (CS_HREDRAW | CS_VREDRAW) as u32,
 //        lpfnWndProc: Some(winproc),
 //        cbClsExtra: 0,
 //        cbWndExtra: 0,
 //        hInstance: unsafe {GetModuleHandleA((0 as LPCSTR))} as HINSTANCE,
 //        hIcon: unsafe {LoadIconA(instance, IDI_APPLICATION)},
 //        hCursor: unsafe {LoadIconA(instance, IDC_ARROW)},
 //        hbrBackground: unsafe { win32::imports::GetStockObject(win32::consts::BLACK_BRUSH) },
 //        lpszMenuName: class.as_ptr(),
 //        lpszClassName: 0 as LPCSTR,
 //        hIconSm: unsafe {LoadIconA(instance, IDI_APPLICATION)}
 //    };

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