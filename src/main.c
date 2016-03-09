#include <windows.h>
#include <windowsx.h>

#define BORDER_WIDTH 8
#define CAPTION_HEGHT 40
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lpParam) {
    switch(uMsg) {
        // caption and border
        case WM_NCHITTEST: {
            RECT wnd_rect;
            GetWindowRect(hWnd, &wnd_rect);
            int x = GET_X_LPARAM(lpParam);
            int y = GET_Y_LPARAM(lpParam);
 
            //bottom left corner
            if (x >= wnd_rect.left && x < wnd_rect.left + BORDER_WIDTH &&
                y < wnd_rect.bottom && y >= wnd_rect.bottom - BORDER_WIDTH)
            {
                return HTBOTTOMLEFT;
            }
            //bottom right corner
            if (x < wnd_rect.right && x >= wnd_rect.right - BORDER_WIDTH &&
                y < wnd_rect.bottom && y >= wnd_rect.bottom - BORDER_WIDTH)
            {
                return HTBOTTOMRIGHT;
            }
            //top left corner
            if (x >= wnd_rect.left && x < wnd_rect.left + BORDER_WIDTH &&
                y >= wnd_rect.top && y < wnd_rect.top + BORDER_WIDTH)
            {
                return HTTOPLEFT;
            }
            //top right corner
            if (x < wnd_rect.right && x >= wnd_rect.right - BORDER_WIDTH &&
                y >= wnd_rect.top && y < wnd_rect.top + BORDER_WIDTH)
            {
                return HTTOPRIGHT;
            }
            //left border
            if (x >= wnd_rect.left && x < wnd_rect.left + BORDER_WIDTH)
            {
                return HTLEFT;
            }
            //right border
            if (x < wnd_rect.right && x >= wnd_rect.right - BORDER_WIDTH)
            {
                return HTRIGHT;
            }
            //bottom border
            if (y < wnd_rect.bottom && y >= wnd_rect.bottom - BORDER_WIDTH)
            {
                return HTBOTTOM;
            }
            //top border
            if (y >= wnd_rect.top && y < wnd_rect.top + BORDER_WIDTH)
            {
                return HTTOP;
            }
            //caption
            if (y >= wnd_rect.top && y < wnd_rect.top + CAPTION_HEGHT)
            {
                return HTCAPTION;
            }

            return HTCLIENT;
        }
        // overdraw
        case WM_NCCALCSIZE: {
            if (wParam > 0) {
                return 0;
            }
            return DefWindowProcA(hWnd, uMsg, wParam, lpParam);
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return  DefWindowProcA(hWnd, uMsg, wParam, lpParam);
}



 int WINAPI WinMain(HINSTANCE hInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {

    return 0;
}

//     let class = CString::new("WindowClass").unwrap();
//     let app_ico = CString::new("LUSTRIOUS_PAINT").unwrap();

//     let wndclassex = WNDCLASSEX {
//         cbSize:size_of::<WNDCLASSEX>() as u32,
//         style: (CS_HREDRAW | CS_VREDRAW) as u32,
//         lpfnWndProc: Some(WndProc),
//         cbClsExtra: 0,
//         cbWndExtra: 0,
//         hInstance: hInstance,
//         hIcon: LoadIcon(hInstance, app_ico.as_ptr() as LPCSTR),
//         hCursor: LoadCursor(NULL!(), IDC_CROSS), // Use NULL for built-in icons
//         hbrBackground: GetStockObject(BLACK_BRUSH),
//         lpszMenuName: CNULL!(),
//         lpszClassName: class.as_ptr() as LPCTSTR,
//         hIconSm: LoadIcon(hInstance, app_ico.as_ptr() as LPCVOID)
//     };

//     let class_atom : ATOM = RegisterClassEx(&wndclassex);
//     assert!(class_atom > 0);

//     let title = CString::new("Lustrious Paint").unwrap();
//     let wnd_style = WS_POPUP | 
//                     WS_CLIPCHILDREN | 
//                     WS_CLIPSIBLINGS | 
//                     WS_SYSMENU | 
//                     WS_THICKFRAME | 
//                     WS_GROUP | /*WS_TABSTOP |*/ 
//                     WS_BORDER | 
//                     WS_MINIMIZEBOX | 
//                     WS_MAXIMIZEBOX;

//     let wnd:HWND = CreateWindowEx(WS_EX_APPWINDOW,
//                    class_atom as LPCVOID,
//                    title.as_ptr() as LPCTSTR,
//                    wnd_style,
//                    100,
//                    100,
//                    WINDOW_WIDTH,
//                    WINDOW_HEIGHT,
//                    NULL!(),
//                    NULL!(),
//                    hInstance,
//                    NULL!());

//     assert!(!wnd.is_null());
//     ShowWindow(wnd, if nCmdShow > 0 {nCmdShow} else {SW_SHOW});

//     let mut msg: MSG = MSG::default();
//     while GetMessage(&mut msg, NULL!(), 0, 0) > 0 {
//         TranslateMessage(&msg);
//         DispatchMessage(&msg);
//     }
// }

// macro_rules! three {
//     () => { 3 }
// }


// void main() {
//     // let hInstance = GetModuleHandle(NULL!());
//     // let lpCmdLine = GetCommandLine();
//     // let nCmdShow = GetStartupInfo().wShowWindow as c_int;

//     // //let d = unsafe {dx11::GetDevice()};
//     // //println!("{:?}", unsafe {((*d).square)(2)});
    
//     // WinMain(hInstance, lpCmdLine, nCmdShow);
// }
