
use types::*;
use ctypes::*;

#[link(name = "user32")]
extern "system" {
    pub fn RegisterClassExW(lpWndClass: *const WNDCLASSEXW) -> ATOM;
    pub fn LoadIconA(hInstance: HINSTANCE, lpIconName: LPCSTR);
}

#[link(name = "kernel32")]
extern "system" {
	pub fn GetModuleHandleA(lpModuleName: LPCSTR) -> HMODULE;
	pub fn GetStartupInfo(lpStartupInfo: LPSTARTUPINFO);
}


#[link(name = "gdi32")]
extern "system" {
	pub fn GetStockObject(i : c_int) -> HGDIOBJ;
}