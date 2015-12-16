
#include <windows.h>
#include <Windowsx.h>
#include <exception>

HINSTANCE hInstance;
HWND      hWnds[2];

enum {
	HWNDS_MAIN_WINDOW,
	HWNDS_OPEN_DIALOG_WINDOW
};

struct Win32MsgHandlerResponse {
	BOOL    pass;   // indicate that next handler should be invoked
	LRESULT result; // value passed to next handler or to Windows
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

void test() {
	Maybe<double> v01 = Nothing<double>();
	Maybe<double> v02 = Just<double>(2.0);
	if (v01.isJust()) {
		double value = Just<double>(v01).GetValue();
	}
	Just<double> v03 = v02;
	if (v01.isJust())
	  Just<double> v04 = v01;
}

/* A message handler is a piece of Window behavior */

class Win32MsgHandler {
protected:
	const UINT m_message_type;
	Win32MsgHandler(UINT message_type) : m_message_type(message_type) {};
public:
	virtual Maybe<LRESULT> HandleMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) = 0;
};

class Win32BorderHitTestHandler : public Win32MsgHandler {
protected:
	LONG m_border_width;
public:
	Win32BorderHitTestHandler(LONG border_width) : Win32MsgHandler(WM_NCHITTEST), m_border_width(border_width) { };
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

#include <map>
#include <deque>

class Win32WndProc;

class Win32WndProcDescriptor final {
public:
	friend Win32WndProc;
	void push_handler_back(Maybe<UINT> message_type, Win32MsgHandler* handler) {
		if (message_type.isJust()) {
			m_handlers_map[Just<UINT>(message_type).GetValue()].push_back(handler);
		}
		else {
			m_default_handlers.push_back(handler);
		}
	}
	void push_handler_front(Maybe<UINT> message_type, Win32MsgHandler* handler) {
		if (message_type.isJust()) {
			m_handlers_map[Just<UINT>(message_type).GetValue()].push_front(handler);
		}
		else {
			m_default_handlers.push_front(handler);
		}
	}
private:
	std::map<UINT, std::deque<Win32MsgHandler*>> m_handlers_map;
	std::deque<Win32MsgHandler*>                 m_default_handlers;

	//std::map<UINT, Win32MsgHandler**> ConstructHandlerStackMap() {
	//	std::map<UINT, Win32MsgHandler**> handlers_map_final;
	//	for each (auto pair in m_handlers_map)
	//	{
	//		Win32MsgHandler** handles_array = new Win32MsgHandler*[pair.second.size() + 1];
	//		handlers_map_final[pair.first] = handles_array;              // associate with message
	//		handles_array[pair.second.size()] = nullptr;                 // null terminate
	//		copy(pair.second.begin(), pair.second.end(), handles_array); // copy deque into array
	//	}
	//	return handlers_map_final;
	//}

	//Win32MsgHandler** ConstructDefaultHandlerStack() {
	//	Win32MsgHandler** default_handlers_final = new Win32MsgHandler*[m_default_handlers.size() + 1];
	//	return default_handlers_final;
	//}
};

/* Win32WndProc assembles Win32MsgHandlers into a WndProc that can be used with Win32 Api */

class Win32WndProc final {
private:
	std::map<UINT, Win32MsgHandler**> m_handlers_map;     // message handler stacks for some or all message types
	Win32MsgHandler**                 m_default_handlers; // default message handler stack used for messages not in the handler map

	Win32WndProc(std::map<UINT, Win32MsgHandler**> handlers_map) : m_handlers_map(handlers_map) { }

	~Win32WndProc() 
	{ 
		for each (auto pair in m_handlers_map) 
			delete[] pair.second; 
	}

	/* Invoke handlers in order until one of them returns final result */
	LRESULT invoke_handlers(
		Win32MsgHandler** handlers, 
		HWND	hWnd,
		UINT	uMsg,
		WPARAM	wParam,
		LPARAM	lParam) 
	{
		// iterate null-terminated array
		for (Win32MsgHandler** i = handlers; *i; ++i) {
			Win32MsgHandler* handler = *i;
			Maybe<LRESULT> response = handler->HandleMessage(hWnd, wParam, lParam);
			if (response.isJust()) {
				return Just<LRESULT>(response).GetValue();
			}
		}
	}
public:
	LRESULT CALLBACK WndProc(
		HWND	hWnd,
		UINT	uMsg,
		WPARAM	wParam,
		LPARAM	lParam)
	{
		auto found = m_handlers_map.find(uMsg);
		if (found != m_handlers_map.end()) {
			return invoke_handlers(found->second, hWnd, uMsg, wParam, lParam);
		}
		else {
			return invoke_handlers(m_default_handlers, hWnd, uMsg, wParam, lParam);
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

int WINAPI WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	LPSTR		lpCmdLine,
	int			nCmdShow)
{
	
	Win32BorderHitTestHandler w(5);
	//Win32MsgHandlerMap mm01;
	//Win32MsgHandlerMap mm02;

	//std::map<UINT, std::vector<Win32MsgHandler*>> m_handlers;
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