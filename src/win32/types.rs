
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use ctypes::*;
use std::mem::{zeroed};

// #[repr(u8)]
// pub enum c_void_ {
//     // Two dummy variants so the #[repr] attribute can be used.
//     #[doc(hidden)]
//     __variant1,
//     #[doc(hidden)]
//     __variant2,
// }

// macro_rules! DECLARE_HANDLE {
//     ($name:ident, $inner:ident) => {
//         #[repr(C)] #[allow(missing_copy_implementations)] struct $inner { unused: c_void_ }
//         pub type $name = *mut $inner;
//     };
// }

// DECLARE_HANDLE!(HMODULE2, HMODULE2__);

pub type HINSTANCE = *mut c_void;
pub type HANDLE =*mut c_void;
pub type HMODULE = *mut c_void;
pub type HGDIOBJ = *mut c_void;
pub type HICON = *mut c_void;
pub type HCURSOR = *mut c_void;
pub type HBRUSH = *mut c_void;
pub type HWND = *mut c_void;
pub type HMENU = *mut c_void;

pub type UINT = c_uint;
pub type WORD = c_ushort;
pub type DWORD = c_int;
pub type ATOM = WORD;
pub type UINT_PTR = c_uint;
pub type LONG_PTR = c_long;
pub type ULONG_PTR = c_ulong;
pub type WPARAM = UINT_PTR;
pub type LPARAM = LONG_PTR;
pub type LRESULT = LONG_PTR;
pub type CHAR = c_schar;
pub type LONG = c_long;
pub type ULONG = c_ulong;
pub type BOOL = c_int;
pub type LPBYTE = *mut c_schar;
pub type SIZE_T = ULONG_PTR;
pub type LPVOID = *mut c_void;
pub type LPCVOID = *const c_void;

pub type LPCSTR = LPCVOID; //*const CHAR;
pub type LPSTR = LPVOID;   //*mut CHAR;
pub type LPTSTR = LPSTR;
pub type LPCTSTR = LPCSTR;

pub type WNDPROC = Option<unsafe  extern "system" fn(HWND, UINT, WPARAM, LPARAM) -> LRESULT>;

#[repr(C)] #[derive(Copy)]
pub struct WNDCLASSEX {
    pub cbSize: UINT,
    pub style: UINT,
	  pub lpfnWndProc: WNDPROC,
    pub cbClsExtra: c_int,
    pub cbWndExtra: c_int,
    pub hInstance: HINSTANCE,
    pub hIcon: HICON,
    pub hCursor: HCURSOR,
    pub hbrBackground: HBRUSH,
    pub lpszMenuName: LPCTSTR,
    pub lpszClassName: LPCTSTR,
    pub hIconSm: HICON,
}

impl Clone for WNDCLASSEX { fn clone(&self) -> WNDCLASSEX { *self } }

#[derive(Debug)]
pub struct STARTUPINFO {
	pub cd: DWORD,
	pub lpReserved: LPTSTR,
	pub lpDesktop: LPTSTR,
	pub lpTitle: LPTSTR,
	pub dwX: DWORD,
	pub dwY: DWORD,
	pub dwXSize: DWORD,
	pub dwYSize: DWORD,
	pub dwXCountChars: DWORD,
	pub dwYCountChars: DWORD,
	pub dwFillAttribute: DWORD,
	pub dwFlags: DWORD,
	pub wShowWindow: WORD,
	pub cbReserved2: WORD,
	pub lpReserved2: LPBYTE,
	pub hStdInput: HANDLE,
	pub hStdOutput: HANDLE,
	pub hStdError: HANDLE
} 

impl Default for STARTUPINFO {
  fn default () -> STARTUPINFO {
  		unsafe {::std::mem::zeroed::<STARTUPINFO>()}
  }
}

pub type LPSTARTUPINFO = *mut STARTUPINFO;

pub struct MSG {
	pub hwnd: HWND,
    pub message: UINT,
    pub wParam: WPARAM,
    pub lParam: LPARAM,
    pub time: DWORD,
    pub pt: POINT,
    pub lPrivate: DWORD
}

impl Default for MSG {
  fn default () -> MSG {
  		unsafe {::std::mem::zeroed::<MSG>()}
  }
}

pub type LPMSG = *mut MSG;

pub struct POINT {
	pub x: LONG,
	pub y: LONG
}

pub struct RECT
{
	pub left: LONG,
    pub top: LONG,
    pub right: LONG,
    pub bottom: LONG
}