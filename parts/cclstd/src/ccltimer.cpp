#include "cclstdinner.h"

void createtimer_ccl(void* req)
{
	CCL_TIMER_REQ* timer = (CCL_TIMER_REQ*)req;
	if (timer)
	{
		CCL_TIMER* pt = nullptr;
		while (!pt) pt = new CCL_TIMER;
		pt->time = 0.0f;
		pt->magic = CCL_TIMERMAGIC;
		timer->timer = pt;
	}
}

void closetimer_ccl(void* req)
{
	CCL_TIMER_REQ* timer = (CCL_TIMER_REQ*)req;
	if (timer)
	{
		if (timer->timer && reinterpret_cast<CCL_TIMER*>(timer->timer)->magic == CCL_TIMERMAGIC)
			delete (CCL_TIMER*)timer->timer;
	}
}

#ifdef CCL_WINDOWS

#include <Windows.h>
#pragma comment(lib, "winmm.lib")

static LARGE_INTEGER frequency = { 0 };
static LARGE_INTEGER count = { 0 };

CCLBOOL _inner_timersetup_ccl()
{
	//在XP以上的Windows上将始终成功，但还是检查一下
	if (!QueryPerformanceFrequency(&frequency))
	{
		return CCLFALSE;
	}
	return CCLTRUE;
}

void shortsleep_ccl(CCL_SLEEPTIME time)
{
	timeBeginPeriod(1);
	SleepEx((DWORD)time, TRUE);
	timeEndPeriod(1);
}

void peektimer_ccl(void* req)
{
	QueryPerformanceCounter(&count);
	CCL_TIMER_REQ* timer = (CCL_TIMER_REQ*)req;
	if (timer)
	{
		CCL_TIMER* ptimer = (CCL_TIMER*)(timer->timer);
		if (ptimer && ptimer->magic == CCL_TIMERMAGIC)
		{
			timer->time = (double)(count.QuadPart) / (double)(frequency.QuadPart) - ptimer->time;
			return;
		}
		timer->time = 0.0f;
	}
}

void marktimer_ccl(void* req)
{
	QueryPerformanceCounter(&count);
	CCL_TIMER_REQ* timer = (CCL_TIMER_REQ*)req;
	if (timer)
	{
		CCL_TIMER* ptimer = (CCL_TIMER*)(timer->timer);
		if (ptimer && ptimer->magic == CCL_TIMERMAGIC)
		{
			double now = (double)(count.QuadPart) / (double)(frequency.QuadPart);
			timer->time = now - ptimer->time;
			ptimer->time = now;
			return;
		}
		timer->time = 0;
	}
}

void date_ccl(void* date)
{
	CCLDATE_REQ* req = (CCLDATE_REQ*)date;
	if (req)
	{
		GetLocalTime((LPSYSTEMTIME)req);
		if (req->str)
		{
			sprintf_s((char*)(req->str), 31, "%03d-%02d-%02d/%01d %02d:%02d:%02d.%03d",
				req->year, req->month, req->day, req->dweek, req->hour,
				req->min, req->sec, req->ms);
		}
	}
}

#elif defined CCL_LINUX
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

static time_t ccl_time = 0;
static struct tm* ptm = NULL;
static struct timeval ti = {0};
static struct timespec ts = {0};

//Linux上无需执行操作
CCLBOOL _inner_timersetup_ccl()
{
	return CCLTRUE;
}

void shortsleep_ccl(CCL_SLEEPTIME time)
{
	usleep((CCLUINT64)time * 1000);
}

void peektimer_ccl(void* req)
{
	clock_gettime(CLOCK_MONOTONIC, &ts);
	CCL_TIMER_REQ* timer = (CCL_TIMER_REQ*)req;
	if (timer)
	{
		CCL_TIMER* ptimer = (CCL_TIMER*)(timer->timer);
		if (ptimer && ptimer->magic == CCL_TIMERMAGIC)
		{
			timer->time = (double)(ts.tv_sec) + (double)(ts.tv_nsec) / 1000000000 - ptimer->time;
			return;
		}
		timer->time = 0.0f;
	}
}

void marktimer_ccl(void* req)
{
	clock_gettime(CLOCK_MONOTONIC, &ts);
	CCL_TIMER_REQ* timer = (CCL_TIMER_REQ*)req;
	if (timer)
	{
		CCL_TIMER* ptimer = (CCL_TIMER*)(timer->timer);
		if (ptimer && ptimer->magic == CCL_TIMERMAGIC)
		{
			double now = (double)(ts.tv_sec) + (double)(ts.tv_nsec) / 1000000000;
			timer->time = now - ptimer->time;
			ptimer->time = now;
			return;
		}
		timer->time = 0.0f;
	}
}

void date_ccl(void* date)
{
    CCLDATE_REQ* req = (CCLDATE_REQ*)date;
	if (req)
	{
		ccl_time = time(NULL);
		ptm = localtime(&ccl_time);
		req->year = ptm->tm_year + 1900; //在Linux就只能手动搬砖了
		req->month = ptm->tm_mon + 1;
		req->day = ptm->tm_mday;
		req->dweek = ptm->tm_wday;
		req->hour = ptm->tm_hour;
		req->min = ptm->tm_min;
		req->sec = ptm->tm_sec;
		gettimeofday(&ti, NULL);
		req->ms = ti.tv_usec % 1000000 / 1000;
		if (req->str)
		{
			sprintf((char*)(req->str), "%03d-%02d-%02d/%01d %02d:%02d:%02d.%03d",
				req->year, req->month, req->day, req->dweek, req->hour,
				req->min, req->sec, req->ms);
		}
	}
}

#endif