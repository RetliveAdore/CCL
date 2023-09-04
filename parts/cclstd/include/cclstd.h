#ifndef _INCLUDE_CCLSTD_H_
#define _INCLUDE_CCLSTD_H_

#include <CCL.h>

#define CCLSTD_MODNAME "CCLstd"

typedef struct date_str
{
	char date[32];
} CCLDateStr;

typedef void* CCLTIMER;

//创建的计时器由用户自行保管，不会有自动释放，
//请务必手动调用 CCLSERV_TIMER_CLOSE 服务
#define CCLSERV_TIMER_CREATE 10
//用于关闭并释放计时器（释放内存空间）
#define CCLSERV_TIMER_CLOSE 11
#define CCLSERV_TIMER_PEEK 12
#define CCLSERV_TIMER_MARK 13
typedef struct ccl_timer_req
{
	CCLTIMER timer; //这个就是指向计时器本体的指针了，需要用户妥善保管
	double time;
}CCL_TIMER_REQ;

#define CCLSERV_GET_DATE 20
typedef struct ccl_date_req
{
	CCLINT16 year;
	CCLINT16 month;
	CCLINT16 dweek;
	CCLINT16 day;
	CCLINT16 hour;
	CCLINT16 min;
	CCLINT16 sec;
	CCLINT16 ms;
	CCLDateStr* str; //可选项，传入有效指针将在里面装填格式化的时间字符串
}CCLDATE_REQ;

//只需要将立即数或者一个整数传入服务接口即可
#define CCLSERV_SHORT_SLEEP 30
typedef void* CCL_SLEEPTIME;

/*
* 多线程部分
*/

//多线程由用户持有的是内部分配的ID，根据ID找到线程进行操作。
//0是用于报错的故障ID
typedef CCLUINT64 CCLTHREADID;

//多线程的函数格式
typedef void* (*CCLThreadFunc)(void* data, CCLTHREADID idThis);

#define CCLSERV_THREAD 40

#define CCLTHREAD_ERROR  0x00
#define CCLTHREAD_CREATE 0x01
#define CCLTHREAD_WAIT   0x02
#define CCLTHREAD_DETACH 0x03
#define CCLTHREAD_ALIVE  0x05

//任何情况下kill都不见得是个好选择
//#define CCLTHREAD_KILL   0x04

typedef struct ccl_thread_req
{
	CCLUINT8 servid;   //CCLTHREAD_XX
	CCLTHREADID thread;
	CCLThreadFunc func;
	void* userdata;
	CCLBOOL status;    //用于指示“成功/失败”或“存活/结束”的线程状态，默认CCLTRUE
	const char* err;   //停留在上一个错误,不一定反映实时状态
}CCL_THREAD_REQ;

//仅适用于线程主动加锁。锁的管理方式和计时器相同
typedef void* CCLTHREADLOCK;

//锁
#define CCLSERV_TLOCK 41

#define CCLLOCK_ERROR   0x00
#define CCLLOCK_CREATE  0x01
#define CCLLOCK_RELEASE 0x02
#define CCLLOCK_TRY     0x03
#define CCLLOCK_UNLOCK  0x04

typedef struct ccl_threadlock_req
{
	CCLUINT8 servid;
	CCLTHREADLOCK lock;
	CCLTHREADID idThis;
	CCLBOOL status;    //用于指明状态
}CCL_THREADLOCK_REQ;

#endif //include
