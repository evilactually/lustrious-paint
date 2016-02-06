
#![allow(overflowing_literals)] 

use types::*;
use ctypes::*;

pub const IDI_APPLICATION: LPCSTR = 32512 as LPCSTR;

pub const IDC_ARROW: LPCSTR = 32512 as LPCSTR;

pub const CS_VREDRAW: DWORD = 0x0001;
pub const CS_HREDRAW: DWORD = 0x0002;

pub const WS_OVERLAPPED: DWORD = 0x00000000;
pub const WS_OVERLAPPEDWINDOW: DWORD = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
pub const WS_CAPTION: DWORD = 0x00C00000;
pub const WS_POPUP: DWORD = 0x80000000;
pub const WS_CLIPCHILDREN: DWORD = 0x02000000;
pub const WS_CLIPSIBLINGS: DWORD = 0x04000000;
pub const WS_SYSMENU: DWORD = 0x00080000;
pub const WS_THICKFRAME: DWORD = 0x00040000;
pub const WS_GROUP: DWORD = 0x00020000;
pub const WS_BORDER: DWORD = 0x00800000;
pub const WS_MINIMIZEBOX: DWORD = 0x00020000;
pub const WS_MAXIMIZEBOX: DWORD = 0x00010000;

pub const WS_EX_APPWINDOW: DWORD = 0x00040000;

pub const BLACK_BRUSH: c_int = 4;

pub const SW_HIDE: c_int = 0;
pub const SW_MAXIMIZE: c_int = 3;
pub const SW_MINIMIZE: c_int = 6;
pub const SW_RESTORE: c_int = 6;
pub const SW_SHOW: c_int = 5;

pub const WM_NCHITTEST: c_uint = 0x0084;

pub const HTNOWHERE: c_int =      0;
pub const HTCLIENT: c_int =       1;
pub const HTCAPTION: c_int =      2;
// pub const HTSYSMENU               3;
// pub const HTGROWBOX               4;
// pub const HTMENU                  5;
// pub const HTHSCROLL               6;
// pub const HTVSCROLL               7;
// pub const HTMINBUTTON             8;
// pub const HTMAXBUTTON             9;
pub const HTLEFT: c_int =         10;
pub const HTRIGHT: c_int =        11;
pub const HTTOP: c_int =          12;
pub const HTTOPLEFT: c_int =      13;
pub const HTTOPRIGHT: c_int =     14;
pub const HTBOTTOM: c_int =       15;
pub const HTBOTTOMLEFT: c_int =   16;
pub const HTBOTTOMRIGHT: c_int =  17;
//pub const HTBORDER            18
