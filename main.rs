
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
    let class = CString::new("WindowClass").unwrap();
    let instance = unsafe {GetModuleHandleA((0 as LPCSTR))};
    let wndclassex = WNDCLASSEX {
        cbSize:size_of::<WNDCLASSEX>() as u32,
        style: (CS_HREDRAW | CS_VREDRAW) as u32,
        lpfnWndProc: Some(winproc),
        cbClsExtra: 0,
        cbWndExtra: 0,
        hInstance: unsafe {GetModuleHandleA((0 as LPCSTR))} as HINSTANCE,
        hIcon: unsafe {LoadIconA(instance, IDI_APPLICATION)},
        hCursor: unsafe {LoadIconA(instance, IDC_ARROW)},
        hbrBackground: unsafe { win32::imports::GetStockObject(win32::consts::BLACK_BRUSH) },
        lpszMenuName: class.as_ptr(),
        lpszClassName: 0 as LPCSTR,
        hIconSm: unsafe {LoadIconA(instance, IDI_APPLICATION)}
    };

    let class_atom : ATOM = unsafe { RegisterClassExA(&wndclassex) };
    println!("hello world {:?}", unsafe {GetModuleHandleA((0 as LPCSTR))});

    //WinMain(0 as HINSTANCE, "...", 0);
}