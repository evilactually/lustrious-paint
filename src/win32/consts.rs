
#![allow(overflowing_literals)] 

use types::*;
use ctypes::*;

pub const IDI_APPLICATION: LPCSTR = 32512 as LPCSTR;

pub const IDC_ARROW: LPCSTR = 32512 as LPCSTR;
pub const IDC_IBEAM: LPCSTR = 32513 as LPCSTR;
pub const IDC_WAIT: LPCSTR  = 32514 as LPCSTR;
pub const IDC_CROSS: LPCSTR = 32515 as LPCSTR;
pub const IDC_UPARROW: LPCSTR = 32516 as LPCSTR;
pub const IDC_SIZE: LPCSTR    = 32640 as LPCSTR;  /* OBSOLETE: use IDC_SIZEALL */
pub const IDC_ICON: LPCSTR    = 32641 as LPCSTR;  /* OBSOLETE: use IDC_ARROW */
pub const IDC_SIZENWSE: LPCSTR = 32642 as LPCSTR;
pub const IDC_SIZENESW: LPCSTR = 32643 as LPCSTR;
pub const IDC_SIZEWE: LPCSTR   = 32644 as LPCSTR;
pub const IDC_SIZENS: LPCSTR   = 32645 as LPCSTR;
pub const IDC_SIZEALL: LPCSTR  = 32646 as LPCSTR;
pub const IDC_NO: LPCSTR       = 32648 as LPCSTR; /*not in win3.1 */
pub const IDC_HAND: LPCSTR     = 32649 as LPCSTR;
pub const IDC_APPSTARTING: LPCSTR = 32650 as LPCSTR; /*not in win3.1 */
pub const IDC_HELP: LPCSTR        = 32651 as LPCSTR;

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

// #define WM_NULL                         0x0000
// #define WM_CREATE                       0x0001
pub const WM_DESTROY: c_uint =             0x0002;
// #define WM_MOVE                         0x0003
// #define WM_SIZE                         0x0005
// #define WM_SETFOCUS                     0x0007
// #define WM_KILLFOCUS                    0x0008
// #define WM_ENABLE                       0x000A
// #define WM_PAINT                        0x000F
// #define WM_CLOSE                        0x0010
// #define WM_QUIT                         0x0012
// #define WM_NOTIFY                       0x004E
// #define WM_NCCREATE                     0x0081
// #define WM_NCDESTROY                    0x0082
// #define WM_NCCALCSIZE                   0x0083
// #define WM_NCHITTEST                    0x0084
// #define WM_NCPAINT                      0x0085
// #define WM_NCACTIVATE                   0x0086
// #define WM_NCMOUSEMOVE                  0x00A0
// #define WM_NCLBUTTONDOWN                0x00A1
// #define WM_NCLBUTTONUP                  0x00A2
// #define WM_NCLBUTTONDBLCLK              0x00A3
// #define WM_NCRBUTTONDOWN                0x00A4
// #define WM_NCRBUTTONUP                  0x00A5
// #define WM_NCRBUTTONDBLCLK              0x00A6
// #define WM_NCMBUTTONDOWN                0x00A7
// #define WM_NCMBUTTONUP                  0x00A8
// #define WM_NCMBUTTONDBLCLK              0x00A9
// #define WM_NCXBUTTONDOWN                0x00AB
// #define WM_NCXBUTTONUP                  0x00AC
// #define WM_NCXBUTTONDBLCLK              0x00AD
// #define WM_INPUT                        0x00FF
// #define WM_KEYFIRST                     0x0100
// #define WM_KEYDOWN                      0x0100
// #define WM_KEYUP                        0x0101
// #define WM_CHAR                         0x0102
// #define WM_DEADCHAR                     0x0103
// #define WM_SYSKEYDOWN                   0x0104
// #define WM_SYSKEYUP                     0x0105
// #define WM_SYSCHAR                      0x0106
// #define WM_SYSDEADCHAR                  0x0107
// #define WM_UNICHAR                      0x0109
// #define WM_KEYLAST                      0x0109
// #define UNICODE_NOCHAR                  0xFFFF
// #define WM_MOUSEFIRST                   0x0200
// #define WM_MOUSEMOVE                    0x0200
// #define WM_LBUTTONDOWN                  0x0201
// #define WM_LBUTTONUP                    0x0202
// #define WM_LBUTTONDBLCLK                0x0203
// #define WM_RBUTTONDOWN                  0x0204
// #define WM_RBUTTONUP                    0x0205
// #define WM_RBUTTONDBLCLK                0x0206
// #define WM_MBUTTONDOWN                  0x0207
// #define WM_MBUTTONUP                    0x0208
// #define WM_MBUTTONDBLCLK                0x0209
// #define WM_MOUSEWHEEL                   0x020A
// #define WM_MOUSEHWHEEL                  0x020E
// #define WM_SIZING                       0x0214
// #define WM_MOVING                       0x0216

pub const WM_NCHITTEST: c_uint =  0x0084;
pub const WM_NCCALCSIZE: c_uint = 0x0083;

pub const HTNOWHERE: c_int =      0;
pub const HTCLIENT: c_int =       1;
pub const HTCAPTION: c_int =      2;
// pub const HTSYSMENU            3;
// pub const HTGROWBOX            4;
// pub const HTMENU               5;
// pub const HTHSCROLL            6;
// pub const HTVSCROLL            7;
// pub const HTMINBUTTON: c_int =    8;
// pub const HTMAXBUTTON          9;
pub const HTLEFT: c_int =         10;
pub const HTRIGHT: c_int =        11;
pub const HTTOP: c_int =          12;
pub const HTTOPLEFT: c_int =      13;
pub const HTTOPRIGHT: c_int =     14;
pub const HTBOTTOM: c_int =       15;
pub const HTBOTTOMLEFT: c_int =   16;
pub const HTBOTTOMRIGHT: c_int =  17;
//pub const HTBORDER            18

pub const TRUE: BOOL  = 1;
pub const FALSE: BOOL = 0;