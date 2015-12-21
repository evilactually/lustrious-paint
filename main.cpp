
#include <windows.h>
#include <windowsx.h>
#include <exception>
#include <map>
#include <vector>
#include <deque>
#include <memory>
#include <algorithm>

HINSTANCE hInstance;
HWND      hWnds[2];

enum {
	HWNDS_MAIN_WINDOW,
	HWNDS_OPEN_DIALOG_WINDOW
};

template<class T>
class Value;

template<class T>
class Nothing;

template<class T>
class Optional {
friend Value<T>;
protected:
	T m_value;
	bool m_is_just = false;
	Optional() = default;
	Optional(T value, bool is_just) : m_value(value), m_is_just(is_just) {};
public:
	Optional(Optional<T>& Optional) : m_value(Optional.m_value), m_is_just(Optional.m_is_just) {};
	Optional(Value<T>& Value) : Optional(Value.m_value, true) {}
	Optional(Nothing<T>& nothing) : Optional() {}
	bool isJust() {
		return m_is_just;
	}
	bool isNothing() {
		return !m_is_just;
	}
};

template<class T>
class Value : public Optional<T> {
public:
	Value(T value) : Optional(value, true) {}
	Value(Optional<T>& Optional):Optional(Optional) {
		if (!Optional.m_is_just) {
			throw std::range_error("Nothing<T> cannot be converted to Value<T>");
		}
	}
	T GetValue() {
		return m_value;
	}
};

template<class T>
class Nothing : public Optional<T> {
public:
	Nothing() : Optional() {}
};

void f() {
	Optional<int> val = Nothing<int>();// Value<int>(2);
	Value<int> value = Value<int>(val);
	int j = value.GetValue();
}

//class EventManager
//{
//public:
//	EventManager();
//	virtual ~EventManager();
//
//	bool RegisterEventListener(eEVENT EventID, IEventListener* pListener);
//	bool UnregisterEventListener(eEVENT EventID, IEventListener* pListener);
//
//	bool ProcessEvent(EventPtr pEvent);
//	bool QueueEvent(EventPtr pEvent);
//	bool ProcessEventQueue();
//
//	static EventManager* Get();
//
//protected:
//	std::vector< IEventListener* > m_EventHandlers[NUM_EVENTS];
//	std::vector< EventPtr > m_EventQueue;
//
//	static EventManager* m_spEventManager;
//};

// event-action association
// event-emitter
// event-reciever
// 

class Event {
private:
	unsigned int m_timestamp;
	unsigned int m_type;
public:
	Event(unsigned int type, unsigned int timestamp) : m_type(type), m_timestamp(timestamp) {};
	Event(const Event&) = delete;
	Event(const Event&&) = delete;
	Event& operator=(const Event&) = delete;
	unsigned int GetTimestamp() { return m_timestamp; }
	unsigned int GetType() { return m_type; }
};

class IEventHandler {
public:
	virtual void HandleEvent(Event* e) = 0;
};

class IEventEmitter {
private:
	std::vector<IEventHandler*>* m_event_handlers;
protected:
	IEventEmitter(unsigned int type_count) {
		m_event_handlers = new std::vector<IEventHandler*>[type_count];
	};
	virtual ~IEventEmitter() {
		delete[] m_event_handlers;
	}
	void Emit(Event* e) {
		std::vector<IEventHandler*> handlers = m_event_handlers[e->GetType()];
		for (IEventHandler* h : handlers) { h->HandleEvent(e); }
	}
public:
	class handler_not_found : public std::exception {
		unsigned int m_type;
	public:
		handler_not_found(unsigned int type) : m_type(type) {}
		int GetType() { return m_type; }
	};
	void Subscribe(unsigned int type, IEventHandler* handler) {
		m_event_handlers[type].push_back(handler);
	}
	void Unsubscribe(unsigned int type, IEventHandler* handler) {
		m_event_handlers[type].erase(std::remove(m_event_handlers[type].begin(), m_event_handlers[type].end(), handler), m_event_handlers[type].end());
	}
};

/* A message handler is a piece of Window behavior */

/* Untyped message handler accepts any message */

class IWin32MsgHandler {
protected:
	IWin32MsgHandler() = default;
public:
	IWin32MsgHandler(const IWin32MsgHandler&) = delete;  // NOTE: There isn't enough information to write a copy or move ctors 
	IWin32MsgHandler(const IWin32MsgHandler&&) = delete; //       given only a base class. Fields in derived classes would be lost!
	IWin32MsgHandler& operator=(const IWin32MsgHandler&) = delete;
	virtual Optional<LRESULT> HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};

/* Typed message handler accepts only specific message */

class handler_misuse : public std::exception {
		UINT m_expected_type;
		UINT m_recieved_type;
	public:
		handler_misuse(UINT expected_type, UINT recieved_type) : m_expected_type(expected_type), m_recieved_type(recieved_type) {};
		UINT GetExpectedType() { return m_expected_type; }
		UINT GetRecievedType() { return m_recieved_type; }
};

class Win32WndProc;

template<unsigned int M>
class Win32TypedMsgHandler: public IWin32MsgHandler {
	friend Win32WndProc;
	virtual Optional<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) = 0;
	Optional<LRESULT> HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		if (uMsg != GetMessageType()) throw handler_misuse(GetMessageType(), uMsg);
		return HandleTypedMessage(hWnd, wParam, lParam);
	};
	UINT GetMessageType() { return M; };
};

class Win32BorderHitTestHandler : public Win32TypedMsgHandler<WM_NCHITTEST> {
	LONG m_border_width;
	Optional<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		RECT winrect;
		GetWindowRect(hWnd, &winrect);
		long x = GET_X_LPARAM(lParam);
		long y = GET_Y_LPARAM(lParam);

		//bottom left corner
		if (x >= winrect.left && x < winrect.left + m_border_width &&
			y < winrect.bottom && y >= winrect.bottom - m_border_width)
		{
			return Value<LRESULT>(HTBOTTOMLEFT);
		}
		//bottom right corner
		if (x < winrect.right && x >= winrect.right - m_border_width &&
			y < winrect.bottom && y >= winrect.bottom - m_border_width)
		{
			return Value<LRESULT>(HTBOTTOMRIGHT);
		}
		//top left corner
		if (x >= winrect.left && x < winrect.left + m_border_width &&
			y >= winrect.top && y < winrect.top + m_border_width)
		{
			return Value<LRESULT>(HTTOPLEFT);
		}
		//top right corner
		if (x < winrect.right && x >= winrect.right - m_border_width &&
			y >= winrect.top && y < winrect.top + m_border_width)
		{
			return Value<LRESULT>(HTTOPRIGHT);
		}
		//left border
		if (x >= winrect.left && x < winrect.left + m_border_width)
		{
			return Value<LRESULT>(HTLEFT);
		}
		//right border
		if (x < winrect.right && x >= winrect.right - m_border_width)
		{
			return Value<LRESULT>(HTRIGHT);
		}
		//bottom border
		if (y < winrect.bottom && y >= winrect.bottom - m_border_width)
		{
			return Value<LRESULT>(HTBOTTOM);
		}
		//top border
		if (y >= winrect.top && y < winrect.top + m_border_width)
		{
			return Value<LRESULT>(HTTOP);
		}

		// invoke next handler
		return Nothing<LRESULT>();
	};
public:
	Win32BorderHitTestHandler(LONG border_width) : m_border_width(border_width) { };
};

class Win32ClientHitTestHandler : public Win32TypedMsgHandler<WM_NCHITTEST> {
	Win32ClientHitTestHandler(); // hit tests top level controls that cover the caption and overrides
};                               // some controls need to be masked somehow from this test

class Win32JustCaptionHandler : public Win32TypedMsgHandler<WM_NCHITTEST> {
public:
	Optional<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		return Value<LRESULT>(HTCAPTION);
	}
};

class Win32CaptionHitTestHandler : public Win32TypedMsgHandler<WM_NCHITTEST> {
	int m_caption_height;
	Optional<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		RECT winrect;
		GetWindowRect(hWnd, &winrect);
		long y = GET_Y_LPARAM(lParam);
		if (y >= winrect.top && y < winrect.top + m_caption_height)
		{
			return Value<LRESULT>(HTCAPTION);
		}
		else {
			return Nothing<LRESULT>();
		}
	}
public:
	Win32CaptionHitTestHandler(unsigned int caption_height) : m_caption_height(caption_height) {};
};

class Win32OverdrawHandler : public Win32TypedMsgHandler<WM_NCCALCSIZE> {
	Optional<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		if (wParam) {
			return Value<LRESULT>(0);
		}
		else {
			return Nothing<LRESULT>();
		}
	}
};

class Win32TerminationHandler: public Win32TypedMsgHandler<WM_DESTROY> {
	Optional<LRESULT> HandleTypedMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
		// close the application entirely
		PostQuitMessage(0);
		return Value<LRESULT>(0);
	}
};

class Win32MouseMessageHandler : public IWin32MsgHandler {
	virtual Optional<LRESULT> HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
	}
public:
	Win32MouseMessageHandler() {

	}
};

class Win32DefWndProc : public IWin32MsgHandler {
public:
	virtual Optional<LRESULT> HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (uMsg == WM_NCHITTEST) {
			return Value<LRESULT>(DefWindowProc(hWnd, uMsg, wParam, lParam));
		}
		return Value<LRESULT>(DefWindowProc(hWnd, uMsg, wParam, lParam));
	}
};

/* Win32WndProc assembles Win32TypedMsgHandlers into a WndProc that can be used with Win32 Api */

class Win32WndProc final {

	struct HandlerEntry {
		std::shared_ptr<IWin32MsgHandler> handler;
		bool                              enabled = true;
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
		
	Optional<LRESULT> invoke_handlers(
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
				Optional<LRESULT> response = handler_entry.handler->HandleMessage(hWnd, uMsg, wParam, lParam);
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
	Win32WndProc() = default;

	// Allow move, since is not a class hierarchy
	Win32WndProc(const Win32WndProc&& p) {
		this->m_default_handlers = p.m_default_handlers;
		this->m_main_handlers = p.m_main_handlers;
	}

	// Disallow copy. Otherwise the copied Win32WndProc would refer to same handler objects!
	Win32WndProc(const Win32WndProc& p) = delete;
	Win32WndProc& operator=(const Win32WndProc&) = delete;

	template<typename T> // NOTE: Type of handler is generalized std::shared_ptr<T> to allow automatic template deduction
	void SetMessageHandler(UINT uMsg, UINT place, std::shared_ptr<T> handler) {
		if (uMsg != handler->GetMessageType()) 
			throw handler_misuse(handler->GetMessageType(), uMsg);
		m_main_handlers[uMsg][place].handler = handler; // TODO: throw if already exists
	}

	void SetDefaultHandler(UINT place, std::shared_ptr<IWin32MsgHandler> handler) {
		m_default_handlers[place].handler = handler;    // TODO: throw if already exists
	}

	std::shared_ptr<IWin32MsgHandler> GetMessageHandler(UINT uMsg, UINT uPlace) {
		return get_handler_entry(uMsg, uPlace).handler;
	}

	std::shared_ptr<IWin32MsgHandler> GetDefaultHandler(UINT uPlace) {
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
			Optional<LRESULT> response = invoke_handlers(found_handlers_it->second, hWnd, uMsg, wParam, lParam);
			if (response.isJust()) {
				return Value<LRESULT>(response).GetValue();
			}
			// if non of the handlers return a result, try default handler stack
			else {
				// try to get response from default handler stack
				response = invoke_handlers(m_default_handlers, hWnd, uMsg, wParam, lParam);
				if (response.isJust()) {
					return Value<LRESULT>(response).GetValue();
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
			Optional<LRESULT> response = invoke_handlers(m_default_handlers, hWnd, uMsg, wParam, lParam);
			if (response.isJust()) {
				return Value<LRESULT>(response).GetValue();
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
	static std::map<ATOM, Win32WndProc*> m_wndproc_map;
public:
	Win32WndProcRegistrar() = delete;
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


Win32WndProc testtt() {
	Win32WndProc proc;
	return proc;
}

template<unsigned int T>
void f02(std::shared_ptr<Win32TypedMsgHandler<T>> h) {

}

class CCC {
public:
	template<unsigned int T>
	void SetMessageHandler(UINT place, std::shared_ptr<Win32TypedMsgHandler<T>> handler) {
		UINT uMsg = handler->GetMessageType();
		//m_main_handlers[M][place].handler = handler; // TODO: throw if already exists
	}
};

int WINAPI WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	LPSTR		lpCmdLine,
	int			nCmdShow)
{
	const UINT WS_STYLE = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU | WS_THICKFRAME | WS_GROUP | /*WS_TABSTOP |*/ WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		
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
	basic_behavior.SetMessageHandler(WM_NCHITTEST, 0, std::make_shared<Win32BorderHitTestHandler>(8));
	basic_behavior.SetMessageHandler(WM_NCHITTEST, 1, std::make_shared<Win32CaptionHitTestHandler>(40));
	basic_behavior.SetMessageHandler(WM_NCCALCSIZE, 0, std::make_shared<Win32OverdrawHandler>());
	basic_behavior.SetMessageHandler(WM_DESTROY, 0, std::make_shared<Win32TerminationHandler>());
	basic_behavior.SetDefaultHandler(0, std::make_shared<Win32DefWndProc>());
	Win32WndProcRegistrar::RegisterClassProcedure(ca, &basic_behavior);

	std::shared_ptr<IWin32MsgHandler> fdd = basic_behavior.GetMessageHandler(WM_NCCALCSIZE, 0);
	std::shared_ptr<Win32OverdrawHandler> fdo = std::static_pointer_cast<Win32OverdrawHandler>(fdd);

	/*std::shared_ptr<Win32TypedMsgHandler<WM_DESTROY>> dest_hdl = std::make_shared<Win32TerminationHandler>();
	std::shared_ptr<Win32TerminationHandler> dest_hdl2 = std::make_shared<Win32TerminationHandler>();
	dest_hdl = std::make_shared<Win32TerminationHandler>();
	f02(dest_hdl);
	f02(std::static_pointer_cast<Win32TypedMsgHandler<WM_DESTROY>>(dest_hdl2));
	f02<WM_DESTROY>(dest_hdl2);

	CCC c;
	c.SetMessageHandler(1, dest_hdl);*/
	//c.SetMessageHandler(1, std::make_shared<Win32TerminationHandler>());
	//c.SetMessageHandler(1, dest_hdl2);


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