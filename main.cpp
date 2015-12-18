
#include <windows.h>
#include <windowsx.h>
#include <exception>
#include <map>
#include <deque>
#include <memory>

HINSTANCE hInstance;
HWND      hWnds[2];

enum {
	HWNDS_MAIN_WINDOW,
	HWNDS_OPEN_DIALOG_WINDOW
};

template<class T>
class Just;

template<class T>
class Nothing;

template<class T>
class Maybe {
friend Just<T>;
protected:
	T m_value;
	bool m_is_just;
	Maybe() : m_is_just(false) {};
	Maybe(T value, bool is_just) : m_value(value), m_is_just(is_just) {};
public:
	Maybe(Maybe<T>& maybe) : m_value(maybe.m_value), m_is_just(maybe.m_is_just) {};
	Maybe(Just<T>& just) : Maybe(just.m_value, true) {}
	Maybe(Nothing<T>& nothing) : Maybe() {}
	bool isJust() {
		return m_is_just;
	}
	bool isNothing() {
		return !m_is_just;
	}
};

template<class T>
class Just : public Maybe<T> {
public:
	Just(T value) : Maybe(value, true) {}
	Just(Maybe<T>& maybe):Maybe(maybe) {
		if (!maybe.m_is_just) {
			throw std::range_error("Nothing<T> cannot be converted to Just<T>");
		}
	}
	T GetValue() {
		return m_value;
	}
};

template<class T>
class Nothing : public Maybe<T> {
public:
	Nothing() : Maybe() {}
};

void f() {
	Maybe<int> val = Nothing<int>();// Just<int>(2);
	Just<int> just = Just<int>(val);
	int j = just.GetValue();
}

/* A message handler is a piece of Window behavior */

/* Untyped message handler accepts any message */

class Win32MsgHandler {
public:
	virtual Maybe<LRESULT> HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};

/* Typed message handler accepts only specific message */

template<unsigned int M>
class Win32TypedMsgHandler: public Win32MsgHandler {
public:
	class handler_misuse : public std::exception {
		UINT m_expected_type;
		UINT m_recieved_type;
	public:
		handler_misuse(UINT expected_type, UINT recieved_type) : m_expected_type(expected_type), m_recieved_type(recieved_type) {};
		UINT GetExpectedType() { return m_expected_type; }
		UINT GetRecievedType() { return m_recieved_type; }
	};
	virtual Maybe<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) = 0;
	Maybe<LRESULT> HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		if (uMsg != GetMessageType()) throw handler_misuse(GetMessageType(), uMsg);
		return HandleTypedMessage(hWnd, wParam, lParam);
	};
	UINT GetMessageType() { return M; };
};

class Win32BorderHitTestHandler : public Win32TypedMsgHandler<WM_NCHITTEST> {
protected:
	LONG m_border_width;
public:
	Win32BorderHitTestHandler(LONG border_width) : m_border_width(border_width) { };
	Maybe<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		RECT winrect;
		GetWindowRect(hWnd, &winrect);
		long x = GET_X_LPARAM(lParam);
		long y = GET_Y_LPARAM(lParam);

		//bottom left corner
		if (x >= winrect.left && x < winrect.left + m_border_width &&
			y < winrect.bottom && y >= winrect.bottom - m_border_width)
		{
			return Just<LRESULT>(HTBOTTOMLEFT);
		}
		//bottom right corner
		if (x < winrect.right && x >= winrect.right - m_border_width &&
			y < winrect.bottom && y >= winrect.bottom - m_border_width)
		{
			return Just<LRESULT>(HTBOTTOMRIGHT);
		}
		//top left corner
		if (x >= winrect.left && x < winrect.left + m_border_width &&
			y >= winrect.top && y < winrect.top + m_border_width)
		{
			return Just<LRESULT>(HTTOPLEFT);
		}
		//top right corner
		if (x < winrect.right && x >= winrect.right - m_border_width &&
			y >= winrect.top && y < winrect.top + m_border_width)
		{
			return Just<LRESULT>(HTTOPRIGHT);
		}
		//left border
		if (x >= winrect.left && x < winrect.left + m_border_width)
		{
			return Just<LRESULT>(HTLEFT);
		}
		//right border
		if (x < winrect.right && x >= winrect.right - m_border_width)
		{
			return Just<LRESULT>(HTRIGHT);
		}
		//bottom border
		if (y < winrect.bottom && y >= winrect.bottom - m_border_width)
		{
			return Just<LRESULT>(HTBOTTOM);
		}
		//top border
		if (y >= winrect.top && y < winrect.top + m_border_width)
		{
			return Just<LRESULT>(HTTOP);
		}

		// invoke next handler
		return Nothing<LRESULT>();
	};
};

class Win32ClientHitTestHandler : public Win32TypedMsgHandler<WM_NCHITTEST> {
	Win32ClientHitTestHandler(); // hit tests top level controls that cover the caption and overrides
};                               // some controls need to be masked somehow from this test

class Win32JustCaptionHandler : public Win32TypedMsgHandler<WM_NCHITTEST> {
public:
	Maybe<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		return Just<LRESULT>(HTCAPTION);
	}
};

class Win32CaptionHitTestHandler : public Win32TypedMsgHandler<WM_NCHITTEST> {
public:
	Win32CaptionHitTestHandler(unsigned int caption_height) {};
	Maybe<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		return Nothing<LRESULT>();
	}
};

class Win32OverdrawHandler : public Win32TypedMsgHandler<WM_NCCALCSIZE> {
	Maybe<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		if (wParam) {
			return Just<LRESULT>(0);
		}
		else {
			return Nothing<LRESULT>();
		}
	}
};

class Win32TerminationHandler: public Win32TypedMsgHandler<WM_DESTROY> {
	Maybe<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		// close the application entirely
		PostQuitMessage(0);
		return Just<LRESULT>(0);
	}
};


//case WM_NCCALCSIZE: {
//	// Take over non-client area
//	return 0;
//	break;

class Win32DefWndProc : public Win32MsgHandler {
public:
	virtual Maybe<LRESULT> HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (uMsg == WM_NCHITTEST) {
			return Just<LRESULT>(DefWindowProc(hWnd, uMsg, wParam, lParam));
		}
		return Just<LRESULT>(DefWindowProc(hWnd, uMsg, wParam, lParam));
	}
};

/* Win32WndProc assembles Win32TypedMsgHandlers into a WndProc that can be used with Win32 Api */

class Win32WndProc final {

	struct HandlerEntry {
		std::shared_ptr<Win32MsgHandler> handler;
		bool                             enabled = true;
	};

	std::map<UINT, std::map<UINT, HandlerEntry>> m_main_handlers;
	std::map<UINT, HandlerEntry>                 m_default_handlers;

	class unhandled_message : public std::exception {
		UINT m_uMsg;
	public:
		unhandled_message(UINT uMsg) : m_uMsg(uMsg) {}
	};

	class default_handler_not_found : public std::exception {
		UINT m_uPlace;
	public:
		default_handler_not_found(UINT uPlace) : m_uPlace(uPlace) {}
		UINT GetPlace() { return m_uPlace; };
	};

	class handler_not_found : public default_handler_not_found {
		UINT m_uMsg;
	public:
		handler_not_found(UINT uMsg, UINT uPlace) : m_uMsg(uMsg), default_handler_not_found(uPlace) {}
		int GetMessage() { return m_uMsg; };
	};
		
	Maybe<LRESULT> invoke_handlers(
		std::map<UINT, HandlerEntry> handlers,
		HWND	hWnd,
		UINT	uMsg,
		WPARAM	wParam,
		LPARAM	lParam)
	{
		for each (auto map_entry in handlers)
		{
			HandlerEntry handler_entry = map_entry.second;
			if (handler_entry.enabled) {
				Maybe<LRESULT> response = handler_entry.handler->HandleMessage(hWnd, uMsg, wParam, lParam);
				if (response.isJust()) {
					return response;
				}
			}
		}
		return Nothing<LRESULT>();
	}

	HandlerEntry& get_handler_entry(UINT uMsg, UINT uPlace) {
		auto found_handlers_it = m_main_handlers.find(uMsg);
		if (found_handlers_it == m_main_handlers.end())
			throw handler_not_found(uMsg, uPlace);
		auto found_handler_it = found_handlers_it->second.find(uPlace);
		if (found_handler_it == m_main_handlers[uMsg].end())
			throw handler_not_found(uMsg, uPlace);
		return found_handler_it->second;
	}

	HandlerEntry& get_default_handler_entry(UINT uPlace) {
		auto found_handler_it = m_default_handlers.find(uPlace);
		if (found_handler_it == m_default_handlers.end())
			throw default_handler_not_found(uPlace);
		return found_handler_it->second;
	}
public:
		
	template<unsigned int M>
	void SetMessageHandler(UINT place, std::shared_ptr<Win32TypedMsgHandler<M>> handler) {
		UINT uMsg = handler->GetMessageType();
		m_main_handlers[uMsg][place].handler = handler; // TODO: throw if already exists
	}

	void SetDefaultHandler(UINT place, std::shared_ptr<Win32MsgHandler> handler) {
		m_default_handlers[place].handler = handler;    // TODO: throw if already exists
	}

	template<unsigned int M>
	std::shared_ptr<Win32TypedMsgHandler<M>> GetMessageHandler(UINT uMsg, UINT uPlace) {
		return std::static_pointer_cast<Win32TypedMsgHandler<M>>(get_handler_entry(uMsg, uPlace).handler);
	}

	std::shared_ptr<Win32MsgHandler> GetDefaultHandler(UINT uPlace) {
		return get_default_handler_entry(uPlace).handler;
	}
	
	void RemoveMessageHandler(UINT uMsg, UINT place) {
		auto found_handlers_it = m_main_handlers.find(uMsg);
		if (found_handlers_it == m_main_handlers.end()) return;
		found_handlers_it->second.erase(place);
		// don't keep empty entries
		if (found_handlers_it->second.empty()) {
			m_main_handlers.erase(found_handlers_it);
		}
	}

	void RemoveDefaultHandler(UINT place) {
		m_default_handlers.erase(place);
	}

	void EnableMessageHandler(UINT uMsg, UINT uPlace, bool enable) {
		get_handler_entry(uMsg, uPlace).enabled = enable;
	}

	void EnableDefaultHandler(UINT uPlace, bool enable) {
		get_default_handler_entry(uPlace).enabled = enable;
	}

	bool IsMessageHandlerEnabled(UINT uMsg, UINT uPlace) {
		return get_handler_entry(uMsg, uPlace).enabled;
	}

	bool IsDefaultHandlerEnabled(UINT uPlace) {
		return get_default_handler_entry(uPlace).enabled;
	}
	
	LRESULT CALLBACK WndProc(
		HWND	hWnd,
		UINT	uMsg,
		WPARAM	wParam,
		LPARAM	lParam)
	{
		// try to find message handler stack
		auto found_handlers_it = m_main_handlers.find(uMsg);
		if (found_handlers_it != m_main_handlers.end()) {
			// try to get response from handler stack
			Maybe<LRESULT> response = invoke_handlers(found_handlers_it->second, hWnd, uMsg, wParam, lParam);
			if (response.isJust()) {
				return Just<LRESULT>(response).GetValue();
			}
			// if non of the handlers return a result, try default handler stack
			else {
				// try to get response from default handler stack
				response = invoke_handlers(m_default_handlers, hWnd, uMsg, wParam, lParam);
				if (response.isJust()) {
					return Just<LRESULT>(response).GetValue();
				}
				// if even default handler stack doesn't respond, throw error
				else {
					throw unhandled_message(uMsg);
				}
			}
		}
		// if not found use default handlers
		else {
			// try to get response from default handler stack
			Maybe<LRESULT> response = invoke_handlers(m_default_handlers, hWnd, uMsg, wParam, lParam);
			if (response.isJust()) {
				return Just<LRESULT>(response).GetValue();
			}
			// if even default handler stack doesn't respond, throw error
			else {
				throw unhandled_message(uMsg);
			}
		}
	}
};

class Win32WndProcRegistrar final {
private:
	Win32WndProcRegistrar();
	static std::map<ATOM, Win32WndProc*> m_wndproc_map;
public:
	static void RegisterClassProcedure(ATOM class_atom, Win32WndProc* procedure) {
		m_wndproc_map[class_atom] = procedure;
	}

	static void UnregisterClassProcedure(ATOM class_atom) {
		m_wndproc_map.erase(class_atom);
	}

	static LRESULT CALLBACK WndProcRedirect(
		HWND	hWnd,
		UINT	uMsg,
		WPARAM	wParam,
		LPARAM	lParam)
	{
		Win32WndProc* procedure = m_wndproc_map[(ATOM)GetClassLong(hWnd, GCW_ATOM)];
		return procedure->WndProc(hWnd, uMsg, wParam, lParam);
	}
};

std::map<ATOM, Win32WndProc*> Win32WndProcRegistrar::m_wndproc_map;

RECT CalculateWindowRect(RECT client_rect, UINT style) {
	AdjustWindowRect(&client_rect, style, FALSE);
	return client_rect;
}

#include "resource.h"

int WINAPI WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	LPSTR		lpCmdLine,
	int			nCmdShow)
{
	

	const UINT WS_STYLE = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU | WS_THICKFRAME | WS_GROUP | WS_TABSTOP | WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		
	RECT wr = CalculateWindowRect({ 0, 0, 640, 480 }, WS_POPUP);
		
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = &Win32WndProcRegistrar::WndProcRedirect;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LUSTRIOUS_PAINT));
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LUSTRIOUS_PAINT));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = "WindowClass";
	ATOM ca = RegisterClassEx(&wc);
	
	Win32WndProc basic_behavior;
	basic_behavior.SetMessageHandler<WM_NCHITTEST>(0, std::make_shared<Win32BorderHitTestHandler>(8));
	basic_behavior.SetMessageHandler<WM_NCHITTEST>(1, std::make_shared<Win32JustCaptionHandler>());
	basic_behavior.SetMessageHandler<WM_NCCALCSIZE>(0, std::make_shared<Win32OverdrawHandler>());
	basic_behavior.SetMessageHandler<WM_DESTROY>(0, std::make_shared<Win32TerminationHandler>());
	//basic_behavior.EnableMessageHandler(WM_NCHITTEST,1,false);
	basic_behavior.SetDefaultHandler(0, std::make_shared<Win32DefWndProc>());
	Win32WndProcRegistrar::RegisterClassProcedure(ca, &basic_behavior);

	HWND hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		"WindowClass",        // class name
		"Lustrious Paint",    // title
		WS_STYLE,             // style
		100,                  // x-position
		100,                  // y-position
		wr.right - wr.left,   // width
		wr.bottom - wr.top,   // height
		NULL,                 // no parent
		NULL,                 // no menus
		hInstance,
		NULL);

	ShowWindow(hwnd, nCmdShow);

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