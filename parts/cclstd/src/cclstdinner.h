#ifndef _INCLUDE_CCLSTD_INNER_H_
#define _INCLUDE_CCLSTD_INNER_H_

#include <cclstd.h>
#include <stdio.h>

//应用了一个魔数来验证计时器，防止错误地释放不是
//计时器的内存块
typedef struct ccl_timer
{
	double time;
	CCLUINT32 magic;
}CCL_TIMER;
#define CCL_TIMERMAGIC 0x5060ffaa

typedef struct ccl_thread
{
	CCLThreadFunc func;
	CCLTHREADID id;
	void* userdata;
	CCLBOOL detach;
	CCLUINT32 magic;
} CCLThreadInner, * PCCLThreadInner;
#define CCL_THREADMAGIC 0x660ffaa5

typedef struct ccl_threadlock
{
	CCLBOOL locked;
	CCLTHREADID id;
	CCLThreadFunc func;
	CCLUINT32 magic;
} CCLThreadLockInner, * PCCLThreadLockInner;
#define CCL_LOCKMAGIC 0xff65678a

#ifdef __cplusplus
extern "C" {
#endif

void createtimer_ccl(void* req);
void closetimer_ccl(void* req);
void peektimer_ccl(void* req);
void marktimer_ccl(void* req);
void date_ccl(void* date);
void shortsleep_ccl(CCL_SLEEPTIME time);

void threadserv_ccl(void* req);
void lockserv_ccl(void* req);

/*
* 内部使用方法均带有 _inner_ 前缀
*/

//仅内部使用，用于在加载时初始化

CCLBOOL _inner_timersetup_ccl();

void _inner_threadinit_ccl();
void _inner_threadclear_ccl();

#ifdef __cplusplus
}
#endif

#endif //include
