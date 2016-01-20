
use ctypes::*;

pub type HINSTANCE = *mut c_void;
pub type HANDLE =*mut c_void;
pub type HMODULE = *mut c_void;
pub type HGDIOBJ = *mut c_void;
pub type HICON = *mut c_void;
pub type HCURSOR = *mut c_void;
pub type HBRUSH = *mut c_void;
pub type LPCWSTR = *mut c_void;
pub type HWND = *mut c_void;
pub type UINT = c_uint;
pub type WORD = c_ushort;
pub type DWORD = int32_t;
pub type ATOM = WORD;
pub type UINT_PTR = c_uint;
pub type LONG_PTR = c_long;
pub type WPARAM = UINT_PTR;
pub type LPARAM = LONG_PTR;
pub type LRESULT = LONG_PTR;
pub type CHAR = c_schar;
pub type LPCSTR = *mut CHAR;
pub type LPTSTR = LPCSTR;
pub type LPBYTE = *mut c_schar;

pub type WNDPROC = Option<unsafe  extern "system" fn(HWND, UINT, WPARAM, LPARAM) -> LRESULT>;

#[repr(C)] #[derive(Copy)]
pub struct WNDCLASSEXW {
    pub cbSize: UINT,
    pub style: UINT,
	pub lpfnWndProc: WNDPROC,
    pub cbClsExtra: c_int,
    pub cbWndExtra: c_int,
    pub hInstance: HINSTANCE,
    pub hIcon: HICON,
    pub hCursor: HCURSOR,
    pub hbrBackground: HBRUSH,
    pub lpszMenuName: LPCWSTR,
    pub lpszClassName: LPCWSTR,
    pub hIconSm: HICON,
}

impl Clone for WNDCLASSEXW { fn clone(&self) -> WNDCLASSEXW { *self } }

struct STARTUPINFO {
  cd: DWORD,
  lpReserved: LPTSTR,
  lpDesktop: LPTSTR,
  lpTitle: LPTSTR,
  dwX: DWORD,
  dwY: DWORD,
  dwXSize: DWORD,
  dwYSize: DWORD,
  dwXCountChars: DWORD,
  dwYCountChars: DWORD,
  dwFillAttribute: DWORD,
  dwFlags: DWORD,
  wShowWindow: WORD,
  cbReserved2: WORD,
  lpReserved2: LPBYTE,
  hStdInput: HANDLE,
  hStdOutput: HANDLE,
  hStdError: HANDLE
} 

//STARTUPINFO, *LPSTARTUPINFO;