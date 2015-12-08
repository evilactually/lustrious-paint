
#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include <sstream>

enum GRAB_STATE;

class Window {
	// [events]
	void SetCursorImage();
	void SetCursorVisibility();
	void SetPosition();
	void SetSize();
	
	// minimize
	// maximize
	// restore
	void Maximize();
	void Minimize();
	void Restore();
	void Update();
	void BeginGrab(GRAB_STATE target);
	void EndGrab();
};

enum GRAB_STATE {
	GRB_NOTHING,
	GRB_WINDOW, 
	GRB_TOP_EDGE, 
	GRB_RIGHT_EDGE, 
	GRB_BOTTOM_EDGE, 
	GRB_LEFT_EDGE, 
	GRB_TOP_LEFT_CORNER,
	GRB_TOP_RIGHT_CORNER,
	GRB_BOTTOM_RIGHT_CORNER,
	GRB_BOTTOM_LEFT_CORNER
};

HWND  m_hwnd;
HINSTANCE	hinst;
POINT m_grab_point = {0,0};
bool  m_is_grabbed = false;
GRAB_STATE m_grab_state;
int m_grab_width;
RECT m_original_rect;

void BeginGrab(GRAB_STATE target) {
	// calculate and save grab point(in window coordinates)
	//RECT r;
	//POINT c;
	//GetCursorPos(&c);
	
	//GetWindowRect(m_hwnd, &r);
	//m_grab_point = { c.x - r.left, c.y - r.top };
	//m_grab_width = r.right - r.left;

	GetCursorPos(&m_grab_point);
	GetWindowRect(m_hwnd, &m_original_rect);

	// allow mouse to get over the edge and still recieve mouse events
	SetCapture(m_hwnd);

	m_grab_state = target;
}

void EndGrab() {
	m_grab_state = GRB_NOTHING;
	ReleaseCapture();
}

void BeginWindowGrab() {
	// calculate and save grab point(in window coordinates)
	RECT r;
	POINT c;
	GetCursorPos(&c);
	GetWindowRect(m_hwnd, &r);
	m_grab_point = { c.x - r.left, c.y - r.top };

	// allow mouse to get over the edge and still recieve mouse events
	SetCapture(m_hwnd);

	m_is_grabbed = true;
};

void EndWindowGrab() {
	ReleaseCapture();
	m_is_grabbed = false;
}

LRESULT CALLBACK WindowProc(
	HWND	hWnd,
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam)
{
	HDC         hdc;
	PAINTSTRUCT ps;
	RECT        rect;
	static int count = 0;

	// sort through and find what code to run for the message given
	switch (uMsg)
	{
	case WM_NCHITTEST: {
		return HTCAPTION;
		break;
	}
	case WM_GETICON: {
		/*RECT wr{ 0,0,640,480 };
		InvalidateRect(hWnd, &wr, true);
		UpdateWindow(hWnd);*/
		
		if (count % 2) {
			return  (LRESULT)LoadIcon(hinst, MAKEINTRESOURCE(IDI_LUSTRIOUS_PAINT));
		}
		else {
			return  (LRESULT)LoadIcon(hinst, MAKEINTRESOURCE(IDI_WARNING));
		}
		break;
	}
	case WM_LBUTTONDOWN: {

		//SetClassLong(hWnd, GCL_HICON, (LONG)LoadIcon(NULL, IDI_APPLICATION));
		//SetClassLong(hWnd, GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_HAND));
		count++;
		//ShowWindow(hWnd, SW_MINIMIZE);

		//BeginGrab(GRB_WINDOW);
		//BeginGrab(GRB_RIGHT_EDGE);
		//BeginGrab(GRB_LEFT_EDGE);
		//BeginGrab(GRB_BOTTOM_EDGE);
		//BeginGrab(GRB_TOP_EDGE);
		//BeginGrab(GRB_BOTTOM_RIGHT_CORNER);
		//BeginGrab(GRB_TOP_RIGHT_CORNER);
		//BeginGrab(GRB_BOTTOM_LEFT_CORNER);
		//BeginGrab(GRB_TOP_LEFT_CORNER);
		//SendMessageA(hWnd, 0, NULL, NULL);
		
		break;
	}
	case WM_LBUTTONUP: {
		EndGrab();
		break;
	}
	case WM_MOUSEMOVE: {
		if (m_grab_state != GRB_NOTHING) {
			POINT final_point;
			GetCursorPos(&final_point);
			POINT drag_delta = { final_point.x - m_grab_point.x, final_point.y - m_grab_point.y };
			int original_width = m_original_rect.right - m_original_rect.left;
			int original_height = m_original_rect.bottom - m_original_rect.top;

			switch (m_grab_state)
			{
			case GRB_WINDOW: {
				MoveWindow(hWnd, m_original_rect.left + drag_delta.x, m_original_rect.top + drag_delta.y, original_width, original_height, false);
				break;
			}
			case GRB_RIGHT_EDGE:
				MoveWindow(hWnd, m_original_rect.left, m_original_rect.top, original_width + drag_delta.x, original_height, false);
				break;
			case GRB_LEFT_EDGE: 
				MoveWindow(hWnd, m_original_rect.left + drag_delta.x, m_original_rect.top, original_width - drag_delta.x, original_height, false);
				break;
			case GRB_BOTTOM_EDGE:
				MoveWindow(hWnd, m_original_rect.left, m_original_rect.top, original_width, original_height + drag_delta.y, false);
				break;
			case GRB_TOP_EDGE:
				MoveWindow(hWnd, m_original_rect.left, m_original_rect.top + drag_delta.y, original_width, original_height - drag_delta.y, false);
				break;
			case GRB_BOTTOM_RIGHT_CORNER:
				MoveWindow(hWnd, m_original_rect.left, m_original_rect.top, original_width + drag_delta.x, original_height + drag_delta.y, false);
				break;
			case GRB_TOP_RIGHT_CORNER:
				MoveWindow(hWnd, m_original_rect.left, m_original_rect.top + drag_delta.y, original_width + drag_delta.x, original_height - drag_delta.y, false);
				break;
			case GRB_BOTTOM_LEFT_CORNER:
				MoveWindow(hWnd, m_original_rect.left + drag_delta.x, m_original_rect.top, original_width - drag_delta.x, original_height + drag_delta.y, false);
				break;
			case GRB_TOP_LEFT_CORNER:
				MoveWindow(hWnd, m_original_rect.left + drag_delta.x, m_original_rect.top + drag_delta.y, original_width - drag_delta.x, original_height - drag_delta.y, false);
				break;
			default:
				break;
			}
		
		}

		//if (m_grab_state == GRB_WINDOW) {
		//	POINT abs_xy;
		//	GetCursorPos(&abs_xy);
		//	RECT rct;
		//	GetWindowRect(hWnd, &rct);
		//	MoveWindow(hWnd, abs_xy.x - m_grab_point.x, abs_xy.y- m_grab_point.y, rct.right-rct.left, rct.bottom-rct.top, false);
		//	//MoveWindow(hWnd, 100, abs_xy.y - cl_y, 640, 480, false);
		//	/*RECT wr = { 0, 0, 640, 480 };
		//	InvalidateRect(hWnd, &wr, true);
		//	UpdateWindow(hWnd);
		//	SetCapture(hWnd);*/
		//}
		//if (m_grab_state == GRB_RIGHT_EDGE) {
		//	POINT abs_xy;
		//	GetCursorPos(&abs_xy);
		//	RECT rct;
		//	GetWindowRect(hWnd, &rct);
		//	MoveWindow(hWnd, rct.left, rct.top, m_grab_width + (abs_xy.x - m_grab_point.x), rct.bottom - rct.top, false);
		//}
		break;
	}
	case WM_DESTROY:
	{
		// close the application entirely
		PostQuitMessage(0);
		return 0;
	} break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &rect);
		std::stringstream ss;
		ss << count;
		//FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
		DrawText(hdc, ss.str().c_str(), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		count++;
		EndPaint(hWnd, &ps);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// #define WS_STYLE 0x860f0000 
//                    820a0000
#define  WS_STYLE WS_POPUP |/* WS_CLIPCHILDREN | WS_CLIPSIBLINGS |*/ WS_SYSMENU | WS_THICKFRAME /*| WS_GROUP  |*/ | WS_TABSTOP
//|  WS_OVERLAPPEDWINDOW
//#define WS_STYLE WS_OVERLAPPEDWINDOW
// #define WS_STYLE /*WS_OVERLAPPEDWINDOW WS_POPUP*/  (WS_OVERLAPPED | WS_CAPTION | /*WS_SYSMENU |*/ WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX) | WS_EX_LAYERED 
//#define WS_STYLE WS_POPUP | WS_CLIPCHILDREN | WS_GROUP | WS_SYSMENU | WS_BORDER | WS_CLIPSIBLINGS
//#define WS_STYLE WS_POPUP | WS_SIZEBOX
//#define WS_STYLE WS_POPUP | WS_OVERLAPPED
//#define WS_STYLE WS_POPUP | WS_BORDER | WS_SYSMENU
//#define WS_STYLE WS_POPUP | WS_SIZEBOX

int WINAPI WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	LPSTR		lpCmdLine,
	int			nCmdShow)
{
	hinst = hInstance;

	// the handle for the window, filled by a function
	//HWND hWnd;
	// this struct holds information for the window class
	WNDCLASSEX wc;

	// Get window size
	RECT wr = { 0, 0, 640, 480 };
	AdjustWindowRect(&wr, WS_STYLE, FALSE);

	// clear out the window class for use
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	// fill in the struct with the needed information
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance; 
	wc.hIcon = NULL; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LUSTRIOUS_PAINT));
	wc.hIconSm = NULL; // LoadIcon(hInstance, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	//wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	//wc.hbrBackground = NULL;
	wc.lpszClassName = "WindowClass";

	// register the window class
	RegisterClassEx(&wc);

	// create the window and use the result as the handle
	m_hwnd = CreateWindowEx(
		NULL,
		"WindowClass",        // class name
		"Lustrious Paint",    // title
		WS_STYLE,             // style
		100,                  // x-position
		100,                  // y-position
		wr.right-wr.left,     // width
		wr.bottom-wr.top,     // height
		NULL,                 // no parent
		NULL,                 // no menus
		hInstance,
		NULL);                // used with multiple windows, NULL

	ShowWindow(m_hwnd, nCmdShow);
	
	SetWindowLong(m_hwnd, GWL_EXSTYLE, GetWindowLong(m_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(m_hwnd, RGB(0,0,0), 0, LWA_COLORKEY);

	// enter the main loop:

	// this struct holds Windows event messages
	MSG msg;

	// wait for the next message in the queue, store the result in 'msg'
	while (GetMessage(&msg, NULL, 0, 0))
	{
		// translate keystroke messages into the right format
		TranslateMessage(&msg);

		// send the message to the WindowProc function
		DispatchMessage(&msg);
	}

	// return this part of the WM_QUIT message to Windows
	return msg.wParam;
}