
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
           cmd_line : LPTSTR,
           nCmdShow : WORD) {

}

fn main() {
    let class = CString::new("Hello, world!").unwrap();
    let wndclassex = WNDCLASSEXW {
        cbSize:size_of::<WNDCLASSEXW>() as u32,
        style: (CS_HREDRAW | CS_VREDRAW) as u32,
        lpfnWndProc: Some(winproc),
        cbClsExtra: 0,
        cbWndExtra: 0,
        hInstance: unsafe {GetModuleHandleA((0 as LPCSTR))} as HINSTANCE,
        hIcon: IDI_APPLICATION,
        hCursor: IDC_ARROW,
        hbrBackground: unsafe { win32::imports::GetStockObject(win32::consts::BLACK_BRUSH) },
        lpszMenuName: class.as_ptr() as *mut ctypes::c_void,
        lpszClassName: 0 as *mut ctypes::c_void,
        hIconSm: IDI_APPLICATION
    };

    let class_atom : ATOM = unsafe { RegisterClassExW(&wndclassex) };
    println!("hello world {:?}", unsafe {GetModuleHandleA((0 as LPCSTR))});

    //WinMain(0 as HINSTANCE, "...", 0);
}