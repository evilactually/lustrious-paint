
#include <windows.h>
#include <Windowsx.h>
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
	Just(Maybe<T>& maybe) {
		if (maybe.m_is_just) {
			Just(maybe.m_value);
		}
		else {
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

/* A message handler is a piece of Window behavior */

/* Untyped message handler accepts any message */

class Win32MsgHandler {
public:
	virtual Maybe<LRESULT> HandleMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) = 0;
};

/* Typed message handler accepts only specific message */

class Win32TypedMsgHandler: public Win32MsgHandler {
protected:
	const UINT m_message_type;
	Win32TypedMsgHandler(UINT message_type) : m_message_type(message_type) {};
public:
	UINT GetMessageType() { return m_message_type; };
};

class Win32BorderHitTestHandler : public Win32TypedMsgHandler {
protected:
	LONG m_border_width;
public:
	Win32BorderHitTestHandler(LONG border_width) : Win32TypedMsgHandler(WM_NCHITTEST), m_border_width(border_width) { };
	Maybe<LRESULT> HandleMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) override {
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



class Win32WndProc;

//class Win32WndProcDescriptor final {
//public:
//	friend Win32WndProc;
//	void push_handler_back(Maybe<UINT> message_type, std::shared_ptr<Win32TypedMsgHandler> handler) {
//		if (message_type.isJust()) {
//			m_handlers_map[Just<UINT>(message_type).GetValue()].push_back(handler);
//		}
//		else {
//			m_default_handlers.push_back(handler);
//		}
//	}
//	void push_handler_front(Maybe<UINT> message_type, std::shared_ptr<Win32TypedMsgHandler> handler) {
//		if (message_type.isJust()) {
//			m_handlers_map[Just<UINT>(message_type).GetValue()].push_front(handler);
//		}
//		else {
//			m_default_handlers.push_front(handler);
//		}
//	}
//private:
//	std::map<UINT, std::deque<std::shared_ptr<Win32TypedMsgHandler>>> m_handlers_map;
//	std::deque<std::shared_ptr<Win32TypedMsgHandler>>                 m_default_handlers;
//
//	//std::map<UINT, std::shared_ptr<Win32TypedMsgHandler>*> ConstructHandlerStackMap() {
//	//	std::map<UINT, std::shared_ptr<Win32TypedMsgHandler>*> handlers_map_final;
//	//	for each (auto pair in m_handlers_map)
//	//	{
//	//		std::shared_ptr<Win32TypedMsgHandler>* handles_array = new std::shared_ptr<Win32TypedMsgHandler>[pair.second.size() + 1];
//	//		handlers_map_final[pair.first] = handles_array;              // associate with message
//	//		handles_array[pair.second.size()] = nullptr;                 // null terminate
//	//		copy(pair.second.begin(), pair.second.end(), handles_array); // copy deque into array
//	//	}
//	//	return handlers_map_final;
//	//}
//
//	//std::shared_ptr<Win32TypedMsgHandler>* ConstructDefaultHandlerStack() {
//	//	std::shared_ptr<Win32TypedMsgHandler>* default_handlers_final = new std::shared_ptr<Win32TypedMsgHandler>[m_default_handlers.size() + 1];
//	//	return default_handlers_final;
//	//}
//};

/* Win32WndProc assembles Win32TypedMsgHandlers into a WndProc that can be used with Win32 Api */

class Win32WndProc final {

	struct HandlerEntry {
		std::shared_ptr<Win32MsgHandler> handler;
		BOOL                             enabled = true;
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
				Maybe<LRESULT> response = handler_entry.handler->HandleMessage(hWnd, wParam, lParam);
				if (response.isJust()) {
					return Just<LRESULT>(response);
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
	void AttachToWindow(HWND hWnd) {
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)this);
	}
	
	void SetMessageHandler(UINT place, std::shared_ptr<Win32TypedMsgHandler> handler) {
		UINT uMsg = handler->GetMessageType();
		m_main_handlers[uMsg][place].handler = handler;
	}

	void SetDefaultHandler(UINT place, std::shared_ptr<Win32MsgHandler> handler) {
		m_default_handlers[place].handler = handler;
	}

	std::shared_ptr<Win32TypedMsgHandler> GetMessageHandler(UINT uMsg, UINT uPlace) {
		return std::static_pointer_cast<Win32TypedMsgHandler>(get_handler_entry(uMsg, uPlace).handler);
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

/* A redirect is needed because member function pointers are incompatible with regular function pointers */

LRESULT CALLBACK WndProcRedirect(
	HWND	hWnd,
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam)
{
	Win32WndProc* instance = (Win32WndProc*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	return instance->WndProc(hWnd, uMsg, wParam, lParam);
}

typedef LRESULT(CALLBACK WndProcCallback)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


class ATest final {
	/*int a;*/
public:
	ATest(int i) {};
	void f() {
		//a++;
	}
};

int WINAPI WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	LPSTR		lpCmdLine,
	int			nCmdShow)
{
	std::shared_ptr<Win32TypedMsgHandler> hnd01 = std::make_shared<Win32BorderHitTestHandler>(2);
	{
		hnd01->HandleMessage(0, 0, 0);
		Win32WndProc ffff;
		ffff.AttachToWindow((HWND)1);
		ffff.SetMessageHandler(0, hnd01);
		std::shared_ptr<Win32BorderHitTestHandler> ptr05 = std::static_pointer_cast<Win32BorderHitTestHandler>(ffff.GetMessageHandler(WM_ACTIVATE, 0));
		ffff.SetMessageHandler(1, std::make_shared<Win32BorderHitTestHandler>(2));
		ffff.SetMessageHandler(2, hnd01);
		ffff.RemoveDefaultHandler(1);
		ffff.RemoveMessageHandler(WM_ACTIVATE, 0);
		ffff.RemoveMessageHandler(WM_ACTIVATE, 1);
		ffff.RemoveMessageHandler(WM_ACTIVATE, 2);
	}

	hnd01->HandleMessage(0, 0, 0);

	Win32BorderHitTestHandler w(5);
	//Win32TypedMsgHandlerMap mm01;
	//Win32TypedMsgHandlerMap mm02;

	//std::map<UINT, std::vector<std::shared_ptr<Win32TypedMsgHandler>>> m_handlers;
	//m_handlers[1].push_back(nullptr);
	//WndProcCallback* f01 = &mm01.WindowProc;
	//WndProcCallback* f02 = &mm02.WindowProc;
	//if (f01 == f02) {
	//	MessageBeep(MB_ICONASTERISK);
//	}

	//Win32BorderHitTestHandler::instance.HandleMessage(0,0,0,0,0);
	// init everything
	// register classes
	// create all windows
	return 0;
}