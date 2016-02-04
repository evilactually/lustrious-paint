
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


