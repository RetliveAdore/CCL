#include "cclgraphicinner.h"

// 全局变量
CCLModFunction threadserv = nullptr, shortsleep = nullptr,
			   ccllockserv = nullptr;
CCLModFunction ccltimer_create = nullptr, ccltimer_close = nullptr,
			   ccltimer_mark = nullptr, ccltimer_peek = nullptr;
static CCLDataStructure cclwindowpool = nullptr;	  // tree
static CCLDataStructure cclwindowavailable = nullptr; // queue
static CCLUINT64 CurrentID = 1;
static CCLTHREADLOCK lock1, lock2;
static CCLUINT64 window_thread_num1 = 0, window_thread_num2 = 0;

// 什么也不做，仅用于防止空指针
CCLBOOL doNothing(PCCL_MSG msg)
{
	return CCLFALSE;
}

// 试着将主循环也纳入线程托管，用户负责添加内容就好
void *CCLWindowThread(void *data, CCLUINT64 idThis);
void *CCLPaintThread(void *data, CCLUINT64 idThis);

// 用于被继承的主类
class CCL_Window
{
public:
	CCL_Window() {}
	CCL_Window(const char *title,
			   CCLUINT64 w, CCLUINT64 h,
			   CCLUINT64 x, CCLUINT64 y,
			   CCLWINDOWSTYLE style)
	{
	}
	// 禁止拷贝
	CCL_Window &operator=(const CCL_Window &) = delete;
	CCL_Window(const CCL_Window &) = delete;
	~CCL_Window() {}

	/*
	 * 都是一些纯虚函数，要在下面实现多态
	 */

	virtual void quit() = 0;
	virtual void clear() = 0;
	virtual void Text(const char *text) = 0; // 如果要设置窗口标题的话，使用该函数
	virtual void Move(CCLUINT64 x, CCLUINT64 y, CCLUINT64 w, CCLUINT64 h) = 0;

	/*
	 * 下面的方法基本上是平台无关的，无需重写
	 */

public:
	void SetMinmax(CCLUINT64 minx, CCLUINT64 miny, CCLUINT64 maxx, CCLUINT64 maxy);
	CCLUINT64 Get_id();
	void Set_id(CCLUINT64 id);
	// 公开变量，可以迅速设置是否限制大小
	CCLBOOL sizeLimit = CCLFALSE;
	cclWindowCallback funcs[7] =
		{
			doNothing,
			doNothing,
			doNothing,
			doNothing,
			doNothing,
			doNothing,
			doNothing};

protected:
	CCLUINT64 id_this = 0;
	CCLUINT64 _maxx = 0, _maxy = 0;
	CCLUINT64 _minx = 0, _miny = 0;

	CCLTHREADID loop_thread = 0;
	CCLTHREADID paint_thread = 0;
};

void CCL_Window::SetMinmax(CCLUINT64 minx, CCLUINT64 miny, CCLUINT64 maxx, CCLUINT64 maxy)
{
	if (maxx < minx)
		maxx = minx;
	if (maxy < miny)
		maxy = miny;
	_minx = minx, _miny = miny;
	_maxx = maxx, _maxy = maxy;
}

CCLUINT64 CCL_Window::Get_id()
{
	return id_this;
}

void CCL_Window::Set_id(CCLUINT64 id)
{
	id_this = id;
}

void clear_callback(void *data)
{
	if (data)
		delete (CCL_Window *)data;
}

//
////////////////////////////////////////////////////////////
#ifdef CCL_WINDOWS // Windows partition
#include <Windows.h>
#include <windowsx.h>
#include <gl/GLU.h>
#include "cclgl.hpp"

typedef struct ccl_windowthread_inf
{
	void *pThis;
	CCLUINT8 style;
	CCLUINT32 fps;
	const char *title;
	CCLUINT32 w, h, x, y;
	CCLUINT32 barw, barh;
	cclWindowCallback *cbk;
	CCLBOOL onquit;
	HWND hWnd;
	ccl_gl* pgl;  // 负责绘制的类
} CCL_WINDOW_THREAD_INF;

class CCLWindow_windows : public CCL_Window
{
public:
	CCLWindow_windows(const char *title,
					  CCLUINT64 w, CCLUINT64 h,
					  CCLUINT64 x, CCLUINT64 y,
					  CCLWINDOWSTYLE style);
	~CCLWindow_windows();
	// 禁止拷贝
	CCLWindow_windows &operator=(const CCLWindow_windows &) = delete;
	CCLWindow_windows(const CCLWindow_windows &) = delete;

	virtual void quit();
	virtual void clear();
	virtual void Text(const char *text);
	virtual void Move(CCLUINT64 x, CCLUINT64 y, CCLUINT64 w, CCLUINT64 h);

private:
	CCL_WINDOW_THREAD_INF inf;

	friend LRESULT AfterProc(HWND, UINT, WPARAM, LPARAM, CCLWindow_windows *);
	friend void windowserv_ccl(void *req);
	friend LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

CCLWindow_windows::CCLWindow_windows(
	const char *title,
	CCLUINT64 w, CCLUINT64 h,
	CCLUINT64 x, CCLUINT64 y,
	CCLWINDOWSTYLE style)
{
	inf.pThis = this;
	inf.onquit = CCLFALSE;
	inf.fps = 60;
	inf.cbk = funcs;
	inf.title = title;
	inf.w = w;
	inf.h = h;
	inf.x = x;
	inf.y = y;
	inf.pgl = nullptr;
	//inf.pD2d = nullptr;

	CCL_THREAD_REQ thr;
	thr.servid = CCLTHREAD_CREATE;
	thr.func = CCLWindowThread;
	thr.userdata = &inf;
	threadserv(&thr);
	loop_thread = thr.thread;
	thr.func = CCLPaintThread;
	thr.userdata = &inf;
	threadserv(&thr);
	paint_thread = thr.thread;
}

CCLWindow_windows::~CCLWindow_windows()
{
	clear();
}

void CCLWindow_windows::quit()
{
	inf.onquit = CCLTRUE;
}

void CCLWindow_windows::clear()
{
	if (inf.hWnd)
		DestroyWindow(inf.hWnd);
}

void CCLWindow_windows::Text(const char *text)
{
	SetWindowText(inf.hWnd, text);
}

void CCLWindow_windows::Move(CCLUINT64 x, CCLUINT64 y, CCLUINT64 w, CCLUINT64 h)
{
	MoveWindow(inf.hWnd, x, y, w, h, TRUE);
}

static MSG w_msg = {0};

void msgloop()
{
	if (GetMessage(&w_msg, NULL, 0, 0))
	{
		TranslateMessage(&w_msg);
		DispatchMessage(&w_msg);
	}
}

void *CCLWindowThread(void *data, CCLUINT64 idThis)
{
	CCL_THREADLOCK_REQ req_l;
	req_l.servid = CCLLOCK_TRY;
	req_l.lock = lock2;
	req_l.status = CCLFALSE;
	req_l.idThis = idThis;
	while (!req_l.status)
		ccllockserv(&req_l);
	window_thread_num2++;
	req_l.servid = CCLLOCK_UNLOCK;
	ccllockserv(&req_l);
	//////////
	CCL_WINDOW_THREAD_INF *inf = (CCL_WINDOW_THREAD_INF *)data;
	// 创建这个窗口的部分

	inf->hWnd = CreateWindow(TEXT("ccl_wndclass"), inf->title,
							 WS_OVERLAPPEDWINDOW, inf->x, inf->y, inf->w, inf->h,
							 NULL, NULL, GetModuleHandle(NULL),
							 inf->pThis);

	// 校准窗口客户区的大小
	RECT client, win;
	GetClientRect(inf->hWnd, &client);
	GetWindowRect(inf->hWnd, &win);
	inf->barw = (win.right - win.left) - (client.right - client.left);
	inf->barh = (win.bottom - win.top) - (client.bottom - client.top);
	int rx = (win.right - win.left) + inf->barw,
		ry = (win.bottom - win.top) + inf->barh;
	if (inf->w != CCL_WINDOWSIZE_DEFAULT && inf->h != CCL_WINDOWSIZE_DEFAULT && inf->x != CCL_WINDOWSIZE_DEFAULT && inf->y != CCL_WINDOWSIZE_DEFAULT)
		MoveWindow(inf->hWnd, inf->x, inf->y, rx, ry, FALSE);
	UpdateWindow(inf->hWnd);
	ShowWindow(inf->hWnd, SW_SHOWDEFAULT);

	// 维护这个窗口的部分
	while (!inf->onquit)
	{
		if (GetMessage(&w_msg, NULL, 0, 0))
		{
			TranslateMessage(&w_msg);
			DispatchMessage(&w_msg);
		}
	}
	delete inf->pgl;
	delete inf->pThis;
	//////////
	req_l.servid = CCLLOCK_TRY;
	req_l.lock = lock2;
	req_l.status = CCLFALSE;
	req_l.idThis = idThis;
	while (!req_l.status)
	{
		ccllockserv(&req_l);
	}
	window_thread_num2--;
	req_l.servid = CCLLOCK_UNLOCK;
	ccllockserv(&req_l);
	return nullptr;
}

void *CCLPaintThread(void *data, CCLUINT64 idThis)
{
	CCL_THREADLOCK_REQ req_l;
	req_l.servid = CCLLOCK_TRY;
	req_l.lock = lock1;
	req_l.status = CCLFALSE;
	req_l.idThis = idThis;
	while (!req_l.status)
		ccllockserv(&req_l);
	window_thread_num1++;
	req_l.servid = CCLLOCK_UNLOCK;
	ccllockserv(&req_l);
	//////////
	CCL_WINDOW_THREAD_INF *inf = (CCL_WINDOW_THREAD_INF *)data;
	CCL_TIMER_REQ req;
	ccltimer_create(&req);
	ccltimer_mark(&req);
	while (!inf->onquit)
	{
		ccltimer_peek(&req);
		if (req.time >= (float)1 / (float)(inf->fps))
		{
			ccltimer_mark(&req);
			SendMessage(inf->hWnd, WM_PAINT, 0, 0);
		}
		shortsleep((void *)1);
	}
	ccltimer_close(&req);
	//////////
	req_l.servid = CCLLOCK_TRY;
	req_l.lock = lock1;
	req_l.status = CCLFALSE;
	req_l.idThis = idThis;
	while (!req_l.status)
		ccllockserv(&req_l);
	window_thread_num1--;
	req_l.servid = CCLLOCK_UNLOCK;
	ccllockserv(&req_l);
	return nullptr;
}

LRESULT AfterProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, CCLWindow_windows *pThis)
{
	CCL_MSG cbinfo;
	cbinfo.windowID = pThis->Get_id();
	cbinfo.x = GET_X_LPARAM(lParam);
	cbinfo.y = GET_Y_LPARAM(lParam);
	cbinfo.keycode = wParam & 0xff;
	cbinfo.status = CCL_S_OTHER;
	if (msg == WM_CLOSE)
	{
		if (!pThis->funcs[CCL_QUIT_CB](&cbinfo))
		{
			DestroyWindow(hWnd);
			return 0;
		}
	}
	else if (msg == WM_DESTROY)
	{
		pThis->quit();
		CCLLinearTableIn(cclwindowavailable, CCLTreeMapGet(cclwindowpool, pThis->Get_id()));
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	// else
	switch (msg)
	{
	case WM_PAINT:
	{
		// 绘制过程并不直接向外暴露，
		// 用户通过添加实体对象并改其属性来控制画面
		cbinfo.x = pThis->inf.w;
		cbinfo.x = pThis->inf.h;
		LRESULT ret = DefWindowProc(hWnd, msg, wParam, lParam);

		pThis->inf.pgl->PaintAll();
		pThis->funcs[CCL_PAINT_CB](&cbinfo);
		return ret;
	}
	case WM_MOUSEMOVE:
	{
		cbinfo.status = CCL_S_MOVE;
		pThis->funcs[CCL_MOUSE_CB](&cbinfo);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		cbinfo.status = CCL_S_DOWN | CCL_S_LEFT;
		pThis->funcs[CCL_MOUSE_CB](&cbinfo);
		return 0;
	}
	case WM_LBUTTONUP:
	{
		cbinfo.status = CCL_S_UP | CCL_S_LEFT;
		pThis->funcs[CCL_MOUSE_CB](&cbinfo);
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		cbinfo.status = CCL_S_DOWN | CCL_S_RIGHT;
		pThis->funcs[CCL_MOUSE_CB](&cbinfo);
		return 0;
	}
	case WM_RBUTTONUP:
	{
		cbinfo.status = CCL_S_UP | CCL_S_RIGHT;
		pThis->funcs[CCL_MOUSE_CB](&cbinfo);
		return 0;
	}
	case WM_KEYDOWN:
	{
		cbinfo.status = CCL_S_DOWN;
		if (!pThis->funcs[CCL_KEY_CB](&cbinfo))
			return DefWindowProc(hWnd, msg, wParam, lParam);
		return 0;
	}
	case WM_KEYUP:
	{
		cbinfo.status = CCL_S_UP;
		if (!pThis->funcs[CCL_KEY_CB](&cbinfo))
			return DefWindowProc(hWnd, msg, wParam, lParam);
		return 0;
	}
	case WM_CHAR:
	{
		if (!pThis->funcs[CCL_KEY_CB](&cbinfo))
			return DefWindowProc(hWnd, msg, wParam, lParam);
		return 0;
	}
	case WM_SETFOCUS:
	{
		cbinfo.status = CCL_S_DOWN;
		if (!pThis->funcs[CCL_FOCUS_CB](&cbinfo))
			return DefWindowProc(hWnd, msg, wParam, lParam);
		return 0;
	}
	case WM_KILLFOCUS:
	{
		cbinfo.status = CCL_S_UP;
		if (!pThis->funcs[CCL_FOCUS_CB](&cbinfo))
			return DefWindowProc(hWnd, msg, wParam, lParam);
		return 0;
	}
	case WM_MOVE:
	{
		if (!pThis->funcs[CCL_MOVE_CB](&cbinfo))
			return DefWindowProc(hWnd, msg, wParam, lParam);
		return 0;
	}
	case WM_SIZE:
	{
		pThis->inf.w = cbinfo.x;
		pThis->inf.h = cbinfo.y;
		pThis->inf.pgl->Resize(cbinfo.x, cbinfo.y);
		if (!pThis->funcs[CCL_MOVE_CB](&cbinfo))
			return DefWindowProc(hWnd, msg, wParam, lParam);
		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		if (pThis->sizeLimit)
		{
			MINMAXINFO *mminfo = (MINMAXINFO *)lParam;
			mminfo->ptMaxTrackSize.x = pThis->_maxx + pThis->inf.barw;
			mminfo->ptMaxTrackSize.y = pThis->_maxy + pThis->inf.barh;
			mminfo->ptMinTrackSize.x = pThis->_minx + pThis->inf.barw;
			mminfo->ptMinTrackSize.y = pThis->_miny + pThis->inf.barh;
		}
		return 0;
	}
	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
	{
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(lpcs->lpCreateParams));
		CCLWindow_windows* pThis = reinterpret_cast<CCLWindow_windows *>(lpcs->lpCreateParams);
		pThis->inf.pgl = new ccl_gl(GetDC(hWnd));
		return 0;
	}
	else
	{
		CCL_Window *pThis = reinterpret_cast<CCL_Window *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (pThis)
		{
			return AfterProc(hWnd, msg, wParam, lParam, (CCLWindow_windows *)pThis);
		}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void regist_class()
{
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hIcon = NULL;
	wcex.hCursor = NULL;
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = TEXT("ccl_wndclass");
	wcex.hIconSm = NULL;
	RegisterClassEx(&wcex);
}

CCLBOOL _inner_cclgraphic_init(CCLMODService service, CCLMODID id)
{
	CCL_MOD_REQUEST_STRUCTURE req;
	if (CCLModGetFn_mod(service, id, CCLSERV_THREAD, &req))
	{
		threadserv = req.func;
		if (CCLModGetFn_mod(service, id, CCLSERV_TLOCK, &req))
			ccllockserv = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_SHORT_SLEEP, &req))
			shortsleep = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_TIMER_CREATE, &req))
			ccltimer_create = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_TIMER_CLOSE, &req))
			ccltimer_close = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_TIMER_MARK, &req))
			ccltimer_mark = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_TIMER_PEEK, &req))
			ccltimer_peek = req.func;
		else
			goto Failed;
		// 创建线程锁
		CCL_THREADLOCK_REQ req_l;
		req_l.servid = CCLLOCK_CREATE;
		ccllockserv(&req_l);
		lock1 = req_l.lock;
		if (!lock1)
			goto Failed;
		ccllockserv(&req_l);
		lock2 = req_l.lock;
		if (!lock2)
			goto Failed;

		if (!cclwindowpool)
			cclwindowpool = CCLCreateTreeMap();
		if (!cclwindowavailable)
			cclwindowavailable = CCLCreateLinearTable();
		regist_class();

		return CCLTRUE;
	}
Failed:
	return CCLFALSE;
}

void _inner_cclgraphic_clear()
{
	CCLDestroyDataStructure(&cclwindowpool, clear_callback);
	CCLDestroyDataStructure(&cclwindowavailable, NULL);
	CCL_THREADLOCK_REQ req;
	req.servid = CCLLOCK_RELEASE;
	req.lock = lock1;
	ccllockserv(&req);
	req.lock = lock2;
	ccllockserv(&req);
}

void msgbox_ccl(void *req)
{
	CCL_MSGBOX_REQ *msgbox = (CCL_MSGBOX_REQ *)req;
	if (msgbox)
	{
		UINT type = MB_OK;
		if (msgbox->type == CCL_MSGBOXTYPE_OKCANCEL)
			type = MB_OKCANCEL;
		else if (msgbox->type == CCL_MSGBOXTYPE_YNCANCEL)
			type = MB_YESNOCANCEL;
		int back = MessageBox(NULL, msgbox->text, msgbox->title, type);
		if (back == IDOK)
			msgbox->status = CCL_MSGBOX_STAT_OK;
		else if (back == IDNO)
			msgbox->status = CCL_MSGBOX_STAT_NO;
		else if (back == IDYES)
			msgbox->status = CCL_MSGBOX_STAT_YES;
		else if (back == IDCANCEL)
			msgbox->status = CCL_MSGBOX_STAT_CANCEL;
	}
}

void windowserv_ccl(void *req)
{
	CCL_WINDOW_REQ *window_serv = (CCL_WINDOW_REQ *)req;
	if (!window_serv)
		return;
	// 遇到了麻烦，只能由一个线程掌管从创建到窗口循环再到销毁窗口的所有直接操作
	// 因为一个线程无法获取另一个线程的消息。
	// 所以说每一个窗口都将对应一条线程。
	CCLWindow_windows *pWindow = (CCLWindow_windows *)CCLTreeMapSeek(cclwindowpool, window_serv->id);
	if (window_serv->servid == CCLWINDOW_CREATE)
	{
		CCLUINT64 id;
		if (!(id = (CCLUINT64)CCLLinearTableOut(cclwindowavailable)))
		{
			id = CurrentID;
			CurrentID++;
		}
		pWindow = new CCLWindow_windows(
			window_serv->title, window_serv->w, window_serv->h,
			window_serv->x, window_serv->y, window_serv->style);
		pWindow->Set_id(id);
		CCLTreeMapPut(cclwindowpool, pWindow, id);
		window_serv->id = id;
	}
	// else
	if (!pWindow)
		return;
	switch (window_serv->servid)
	{
	case CCLWINDOW_ITEM:
		//while (!pWindow->inf.pD2d)
		//	shortsleep((void *)1);
		//pWindow->inf.pD2d->AddItem((PCCL2DITEM)(window_serv->item));
		while (!pWindow->inf.pgl)
			shortsleep((void *)1);
		pWindow->inf.pgl->AddItem((PCCL2DITEM)(window_serv->item));
		break;
	case CCLWINDOW_CBK:
		pWindow->funcs[window_serv->cbk_id] = window_serv->cbk;
		break;
	case CCLWINDOW_DESTROY:
		CCLLinearTableIn(cclwindowavailable, (void *)(pWindow->Get_id()));
		pWindow->clear();
		break;
	case CCLWINDOW_MOVE:
		pWindow->Move(window_serv->x, window_serv->y, window_serv->w, window_serv->h);
		break;
	case CCLWINDOW_TITLE:
		pWindow->Text(window_serv->title);
		break;
	case CCLWINDOW_SIZELMT:
		pWindow->sizeLimit = window_serv->status;
		pWindow->SetMinmax(window_serv->x, window_serv->y, window_serv->w, window_serv->h);
		break;
	default:
		break;
	}
}

void onquit_ccl(void *ifquit)
{
	if (CCLDataStructureSize(cclwindowpool))
		*(CCLBOOL *)ifquit = CCLFALSE;
	else if (window_thread_num1)
		*(CCLBOOL *)ifquit = CCLFALSE;
	else if (window_thread_num2)
		*(CCLBOOL *)ifquit = CCLFALSE;
	else
		*(CCLBOOL *)ifquit = CCLTRUE;
}

#elif defined CCL_LINUX
#include <X11/Xlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "cclgl.hpp"

Display *pDisplay;

typedef struct ccl_windowthread_inf
{
	void *pThis;
	CCLUINT8 style;
	CCLUINT32 fps;
	const char *title;
	CCLUINT32 w, h, x, y;
	cclWindowCallback *cbk;
	CCLBOOL onquit;
	XVisualInfo *vi;
	Window win;
	ccl_gl *pgl;
	CCLBOOL paint;
	Atom protocols_quit;
} CCL_WINDOW_THREAD_INF;

class CCLWindow_linux : public CCL_Window
{
public:
	CCLWindow_linux(const char *title,
					CCLUINT64 w, CCLUINT64 h,
					CCLUINT64 x, CCLUINT64 y,
					CCLWINDOWSTYLE style);
	~CCLWindow_linux();
	// 禁止拷贝
	CCLWindow_linux &operator=(const CCLWindow_linux &) = delete;
	CCLWindow_linux(const CCLWindow_linux &) = delete;

	virtual void quit();
	virtual void clear();
	virtual void Text(const char *text);
	virtual void Move(CCLUINT64 x, CCLUINT64 y, CCLUINT64 w, CCLUINT64 h);

private:
	CCL_WINDOW_THREAD_INF inf;

	friend void windowserv_ccl(void *req);
};

CCLWindow_linux::CCLWindow_linux(const char *title,
								 CCLUINT64 w, CCLUINT64 h,
								 CCLUINT64 x, CCLUINT64 y,
								 CCLWINDOWSTYLE style)
{
	inf.onquit = CCLFALSE;
	inf.fps = 30;
	inf.cbk = funcs;
	inf.title = title;
	inf.w = w;
	inf.h = h;
	inf.x = x;
	inf.y = y;
	inf.win = 0;
	inf.vi = nullptr;
	inf.pThis = this;
	inf.pgl = nullptr;
	inf.paint = CCLFALSE;
	// XCreateWindow();
	CCL_THREAD_REQ req;
	req.servid = CCLTHREAD_CREATE;
	req.func = CCLWindowThread;
	req.userdata = &inf;
	threadserv(&req);
	loop_thread = req.thread;
	req.func = CCLPaintThread;
	req.userdata = &inf;
	threadserv(&req);
	paint_thread = req.thread;
}

CCLWindow_linux::~CCLWindow_linux()
{}

void CCLWindow_linux::quit()
{
	inf.onquit = CCLTRUE;
}

void CCLWindow_linux::clear()
{
	//发消息利用原本的方法来自然销毁是最完美的解法
	XEvent event;
	event.type = ClientMessage;
	event.xany.window = inf.win;
	event.xclient.data.l[0] = inf.protocols_quit;
	XPutBackEvent(pDisplay, &event);
}

void CCLWindow_linux::Text(const char *text)
{
}

void CCLWindow_linux::Move(CCLUINT64 x, CCLUINT64 y, CCLUINT64 w, CCLUINT64 h)
{
}

void ProcessMsg(CCL_WINDOW_THREAD_INF *inf)
{
	CCL_MSG cbinfo;
	XEvent event;
	CCLWindow_linux *pWindow = (CCLWindow_linux *)(inf->pThis);
	cbinfo.status = CCL_S_OTHER;
	cbinfo.windowID = pWindow->Get_id();
	while (!inf->onquit)
	{
		XNextEvent(pDisplay, &event);
		if (event.xany.window != inf->win)
		{
			XPutBackEvent(pDisplay, &event);
			shortsleep((void *)1);
			continue;
		}
		switch (event.type)
		{
		case Expose: // “显示”事件
		{
			if (event.xexpose.count == 0)
			{
				inf->paint = CCLTRUE;
			}
			break;
		}
		case ConfigureNotify: // 窗口改变大小
		{
			inf->w = event.xconfigure.width;
			inf->h = event.xconfigure.height;
			cbinfo.x = event.xconfigure.width;
			cbinfo.y = event.xconfigure.height;
			inf->paint = CCLTRUE;
			break;
		}
		case MotionNotify:
		{
			cbinfo.status = CCL_S_MOVE;
			cbinfo.x = event.xbutton.x;
			cbinfo.y = event.xbutton.y;
			inf->cbk[CCL_MOUSE_CB](&cbinfo);
			break;
		}
		case ButtonPress:
		{
			cbinfo.status = CCL_S_DOWN;
			if (event.xbutton.button == 1)
				cbinfo.status |= CCL_S_LEFT;
			else if (event.xbutton.button == 3)
				cbinfo.status |= CCL_S_RIGHT;
			cbinfo.x = event.xbutton.x;
			cbinfo.y = event.xbutton.y;
			inf->cbk[CCL_MOUSE_CB](&cbinfo);
			break;
		}
		case ButtonRelease:
		{
			cbinfo.status = CCL_S_UP;
			if (event.xbutton.button == 1)
				cbinfo.status |= CCL_S_LEFT;
			else if (event.xbutton.button == 3)
				cbinfo.status |= CCL_S_RIGHT;
			cbinfo.x = event.xbutton.x;
			cbinfo.y = event.xbutton.y;
			inf->cbk[CCL_MOUSE_CB](&cbinfo);
			break;
		}
		case KeyPress:
		{
			cbinfo.status = CCL_S_DOWN;
			cbinfo.keycode = event.xkey.keycode;
			inf->cbk[CCL_KEY_CB](&cbinfo);
			break;
		}
		case KeyRelease:
		{
			cbinfo.status = CCL_S_UP;
			cbinfo.keycode = event.xkey.keycode;
			inf->cbk[CCL_KEY_CB](&cbinfo);
			break;
		}
		case DestroyNotify:
			break;
		case ClientMessage:
		{
			if (event.xclient.data.l[0] == inf->protocols_quit)
			{
				if (!inf->cbk[CCL_QUIT_CB](&cbinfo))
				{
					// 释放
					XSelectInput(pDisplay, inf->win, NoEventMask);
					XDestroyWindow(pDisplay, inf->win);
					pWindow->quit();
				}
			}
			break;
		}
		default:
		{
			if (inf->onquit)
			{
				// 释放
				XSelectInput(pDisplay, inf->win, NoEventMask);
				XDestroyWindow(pDisplay, inf->win);
				break;
			}
			shortsleep((void *)1);
			break;
		}
		}
	}
	XFlush(pDisplay);
}

void *CCLWindowThread(void *data, CCLUINT64 idThis)
{
	Window root;
	GLint att[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 24,
		GLX_SAMPLE_BUFFERS,
		//GLX_SAMPLES, 4,
		None
		};
	//XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;
	GLXContext glc;
	XWindowAttributes gwa;
	XEvent xev;

	//////////
	CCL_THREADLOCK_REQ req_l;
	req_l.servid = CCLLOCK_TRY;
	req_l.lock = lock2;
	req_l.status = CCLFALSE;
	req_l.idThis = idThis;
	while (!req_l.status) ccllockserv(&req_l);
	window_thread_num2++;
	req_l.servid = CCLLOCK_UNLOCK;
	ccllockserv(&req_l);
	//////////
	CCL_WINDOW_THREAD_INF* inf = (CCL_WINDOW_THREAD_INF*)data;

	root = DefaultRootWindow(pDisplay);
	inf->vi = glXChooseVisual(pDisplay, 0, att);
	if (!inf->vi)
		printf("attr error\n");
	cmap = XCreateColormap(pDisplay, root, inf->vi->visual, AllocNone);

	swa.colormap = cmap;
	//选择关心的事件
	swa.event_mask = ExposureMask
	| KeyPressMask | ButtonPressMask
	| KeyReleaseMask | ButtonReleaseMask
	| PointerMotionMask
	| StructureNotifyMask;

	inf->win = XCreateWindow(pDisplay, root, inf->x, inf->y, inf->w, inf->h, 0, inf->vi->depth, InputOutput, inf->vi->visual, CWColormap | CWEventMask, &swa);
	XStoreName(pDisplay, inf->win, inf->title);

	XMapWindow(pDisplay, inf->win);

	//捕获退出事件
	inf->protocols_quit = XInternAtom(pDisplay, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(pDisplay, inf->win, &(inf->protocols_quit), 1);

	// inf->pgl = new ccl_gl(pDisplay, vi, inf->win);

	/**/
	ProcessMsg(inf);
	/**/

	//////////
	req_l.servid = CCLLOCK_TRY;
	req_l.lock = lock2;
	req_l.status = CCLFALSE;
	req_l.idThis = idThis;
	while (!req_l.status) ccllockserv(&req_l);
	window_thread_num2--;
	req_l.servid = CCLLOCK_UNLOCK;
	ccllockserv(&req_l);
	return nullptr;
}

void *CCLPaintThread(void *data, CCLUINT64 idThis)
{
	const char *CCLstr = "CCL XWindow";
	char size[32];
	//////////
	CCL_THREADLOCK_REQ req_l;
	req_l.servid = CCLLOCK_TRY;
	req_l.lock = lock1;
	req_l.status = CCLFALSE;
	req_l.idThis = idThis;
	while (!req_l.status)
		ccllockserv(&req_l);
	window_thread_num1++;
	req_l.servid = CCLLOCK_UNLOCK;
	ccllockserv(&req_l);
	//////////
	CCL_WINDOW_THREAD_INF *inf = (CCL_WINDOW_THREAD_INF *)data;
	while (!inf->win)
		shortsleep((void *)1);

	//一定要确保创建和绘制都在同一个线程
	inf->pgl = new ccl_gl(pDisplay, inf->vi, inf->win);

	CCL_TIMER_REQ req;
	ccltimer_create(&req);
	ccltimer_mark(&req);
	while (!inf->onquit)
	{
		ccltimer_peek(&req);
		if (req.time >= (float)1 / (float)(inf->fps) || inf->paint)
		{
			ccltimer_mark(&req);
			inf->paint = CCLFALSE;
			inf->pgl->Resize(inf->w, inf->h);
			inf->pgl->PaintAll();
			XFlush(pDisplay);
		}
		shortsleep((void *)1);
	}
	ccltimer_close(&req);
	CCLWindow_linux* pWindow = (CCLWindow_linux*)(inf->pThis);
	delete inf->pgl;
	CCLLinearTableIn(cclwindowavailable, CCLTreeMapGet(cclwindowpool, pWindow->Get_id()));
	delete (CCLWindow_linux*)(inf->pThis);
	//////////
	req_l.servid = CCLLOCK_TRY;
	req_l.lock = lock1;
	req_l.status = CCLFALSE;
	req_l.idThis = idThis;
	while (!req_l.status) ccllockserv(&req_l);
	window_thread_num1--;
	req_l.servid = CCLLOCK_UNLOCK;
	ccllockserv(&req_l);
	return nullptr;
}

CCLBOOL _inner_cclgraphic_init(CCLMODService service, CCLMODID id)
{
	CCL_MOD_REQUEST_STRUCTURE req;
	if (CCLModGetFn_mod(service, id, CCLSERV_THREAD, &req))
	{
		threadserv = req.func;
		if (CCLModGetFn_mod(service, id, CCLSERV_TLOCK, &req))
			ccllockserv = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_SHORT_SLEEP, &req))
			shortsleep = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_TIMER_CREATE, &req))
			ccltimer_create = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_TIMER_CLOSE, &req))
			ccltimer_close = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_TIMER_MARK, &req))
			ccltimer_mark = req.func;
		else
			goto Failed;
		if (CCLModGetFn_mod(service, id, CCLSERV_TIMER_PEEK, &req))
			ccltimer_peek = req.func;
		else
			goto Failed;
		if (!(pDisplay = XOpenDisplay(NULL)))
			goto Failed;
		// �����߳���
		CCL_THREADLOCK_REQ req_l;
		req_l.servid = CCLLOCK_CREATE;
		ccllockserv(&req_l);
		lock1 = req_l.lock;
		if (!lock1)
			goto Failed;
		ccllockserv(&req_l);
		lock2 = req_l.lock;
		if (!lock2)
			goto Failed;

		if (!cclwindowpool)
			cclwindowpool = CCLCreateTreeMap();
		if (!cclwindowavailable)
			cclwindowavailable = CCLCreateLinearTable();

		return CCLTRUE;
	}
Failed:
	return CCLFALSE;
}

void clear_linux(void* data)
{
	CCLWindow_linux* pWin = (CCLWindow_linux*)data;
	pWin->clear();
}

void _inner_cclgraphic_clear()
{
	CCLDestroyDataStructure(&cclwindowpool, clear_linux);
	CCLDestroyDataStructure(&cclwindowavailable, NULL);
	XCloseDisplay(pDisplay);
	CCL_THREADLOCK_REQ req;
	req.servid = CCLLOCK_RELEASE;
	req.lock = lock1;
	ccllockserv(&req);
	req.lock = lock2;
	ccllockserv(&req);
}

void msgbox_ccl(void *req)
{
	CCL_MSGBOX_REQ *msgbox = (CCL_MSGBOX_REQ *)req;
	if (msgbox)
	{
	}
}

void windowserv_ccl(void *req)
{
	CCL_WINDOW_REQ *window_serv = (CCL_WINDOW_REQ *)req;
	if (!window_serv)
		return;
	//
	CCLWindow_linux *pWindow = (CCLWindow_linux *)CCLTreeMapSeek(cclwindowpool, window_serv->id);
	if (window_serv->servid == CCLWINDOW_CREATE)
	{
		CCLUINT64 id;
		if (!(id = (CCLUINT64)CCLLinearTableOut(cclwindowavailable)))
		{
			id = CurrentID;
			CurrentID++;
		}
		pWindow = new CCLWindow_linux(
			window_serv->title, window_serv->w, window_serv->h,
			window_serv->x, window_serv->y, window_serv->style);
		pWindow->Set_id(id);
		CCLTreeMapPut(cclwindowpool, pWindow, id);
		window_serv->id = id;
	}
	// else
	if (!pWindow)
		return;
	switch (window_serv->servid)
	{
	case CCLWINDOW_ITEM:
		while (!pWindow->inf.pgl)
			shortsleep((void *)1);
		pWindow->inf.pgl->AddItem((PCCL2DITEM)window_serv->item);
		break;
	case CCLWINDOW_CBK:
		pWindow->funcs[window_serv->cbk_id] = window_serv->cbk;
		break;
	case CCLWINDOW_DESTROY:
		CCLLinearTableIn(cclwindowavailable, (void *)(pWindow->Get_id()));
		pWindow->clear();
		break;
	case CCLWINDOW_MOVE:
		pWindow->Move(window_serv->x, window_serv->y, window_serv->w, window_serv->h);
		break;
	case CCLWINDOW_TITLE:
		pWindow->Text(window_serv->title);
		break;
	case CCLWINDOW_SIZELMT:
		pWindow->sizeLimit = window_serv->status;
		pWindow->SetMinmax(window_serv->x, window_serv->y, window_serv->w, window_serv->h);
		break;
	default:
		break;
	}
}

void onquit_ccl(void *ifquit)
{
	if (CCLDataStructureSize(cclwindowpool))
		*(CCLBOOL *)ifquit = CCLFALSE;
	else if (window_thread_num1)
		*(CCLBOOL *)ifquit = CCLFALSE;
	else if (window_thread_num2)
		*(CCLBOOL *)ifquit = CCLFALSE;
	else
		*(CCLBOOL *)ifquit = CCLTRUE;
}

#endif