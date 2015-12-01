
#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include <sstream>


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
	static bool grab = false;
	static int cl_x = 100;
	static int cl_y = 100;

	// sort through and find what code to run for the message given
	switch (uMsg)
	{
	case WM_LBUTTONDOWN: {
		RECT wr = { 0, 0, 640, 480 };
		InvalidateRect(hWnd, &wr, false);
		UpdateWindow(hWnd);
		SetCapture(hWnd);
		grab = true;
		//cl_x = GET_X_LPARAM(lParam);
		//cl_y = GET_Y_LPARAM(lParam);
		POINT abs_xy;
		GetCursorPos(&abs_xy);
		RECT rct;
		if (!GetWindowRect(hWnd, &rct)) {
			grab = false;
		}
		cl_x = abs_xy.x - rct.left;
		cl_y = abs_xy.y - rct.top;
		count = rct.top;
		break;
	}
	case WM_LBUTTONUP: {
		ReleaseCapture();
		grab = false;
		break;
	}
	case WM_MOUSEMOVE: {
		if (grab) {
			POINT abs_xy;
			GetCursorPos(&abs_xy);
			//count = cl_y;
			RECT rct;
			GetWindowRect(hWnd, &rct);
			MoveWindow(hWnd, abs_xy.x-cl_x, abs_xy.y-cl_y, rct.right-rct.left, rct.bottom-rct.top, false);
			//MoveWindow(hWnd, 100, abs_xy.y - cl_y, 640, 480, false);
			RECT wr = { 0, 0, 640, 480 };
			InvalidateRect(hWnd, &wr, true);
			UpdateWindow(hWnd);
			SetCapture(hWnd);
		}
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
		DrawText(hdc, ss.str().c_str(), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		count++;
		EndPaint(hWnd, &ps);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	LPSTR		lpCmdLine,
	int			nCmdShow)
{
	
	// the handle for the window, filled by a function
	HWND hWnd;
	// this struct holds information for the window class
	WNDCLASSEX wc;

	// Get window size
	RECT wr = { 0, 0, 640, 480 };
	AdjustWindowRect(&wr, WS_POPUP, FALSE);

	// clear out the window class for use
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	// fill in the struct with the needed information
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance; 
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LUSTRIOUS_PAINT));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "WindowClass";

	// register the window class
	RegisterClassEx(&wc);

	// create the window and use the result as the handle
	hWnd = CreateWindowEx(
		NULL,
		"WindowClass",        // class name
		"Lustrious Paint",    // title
		WS_POPUP,             // style
		100,              // x-position
		100,               // y-position
		wr.right-wr.left,     // width
		wr.bottom-wr.top,     // height
		NULL,                 // no parent
		NULL,                 // no menus
		hInstance,
		NULL);                // used with multiple windows, NULL

	ShowWindow(hWnd, nCmdShow);
	
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