
use ctypes::*;
use std::mem::{zeroed};

pub type HINSTANCE = *mut c_void;
pub type HANDLE =*mut c_void;
pub type HMODULE = *mut c_void;
pub type HGDIOBJ = *mut c_void;
pub type HICON = *mut c_void;
pub type HCURSOR = *mut c_void;
pub type HBRUSH = *mut c_void;
pub type HWND = *mut c_void;

pub type UINT = c_uint;
pub type WORD = c_ushort;
pub type DWORD = c_int;
pub type ATOM = WORD;
pub type UINT_PTR = c_uint;
pub type LONG_PTR = c_long;
pub type WPARAM = UINT_PTR;
pub type LPARAM = LONG_PTR;
pub type LRESULT = LONG_PTR;
pub type CHAR = c_schar;
pub type LPBYTE = *mut c_schar;
pub type ULONG = c_ulong;
pub type ULONG_PTR = c_ulong;
pub type SIZE_T = ULONG_PTR;

pub type LPCSTR = *const CHAR;
pub type LPSTR = *mut CHAR;
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

//STARTUPINFO, *LPSTARTUPINFO;