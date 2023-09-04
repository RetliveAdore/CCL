#include <CCL.h>
#include <iostream>
#include <cclstd.h>
#include <string.h>

//和外部模块相关的全局变量
CCLMODID cclstd;
CCLModFunction shortsleep = NULL, ccldate = NULL, createtimer = NULL, closetimer = NULL;
CCLModFunction timermark = NULL, timerpeek = NULL;
CCLModFunction threadserv = NULL, lockserv = NULL;

/*
* 全局变量，可以多线程共享
*/

CCLTHREADLOCK lock1;

void trash(void)
{
	std::cout << "on exit" << std::endl;
}

void Version()
{
	CCLVERSION cclver = { 0 };
	CCLVersion(&cclver);
	std::cout
		<< "MAJOR-" << cclver.MAJOR << "-"
		<< "MINOR-" << cclver.MINOR << "-"
		<< "STEP-" << cclver.STEP << "-"
		<< "CHECK-" << cclver.CHECK
		<< std::endl;
}

//Windos和Linux库文件以及工作目都有点差别
void gluing_path(const char* argv, const char* obj, char** result)
{
	int len_source = strlen(argv);
	int len_obj = strlen(obj);
	int subscript = 0;
	for (int i = 0; i < len_source; i++)
	{
#ifdef CCL_WINDOWS
		if (argv[i] == '\\')
			subscript = i;
#elif defined CCL_LINUX
		if (argv[i] == '/')
			subscript = i;
#endif
	}
	len_source = subscript + 1;
	*result = new char[len_source + len_obj + 1];
	int i = 0;
	for (i = 0; i < len_source; i++)
	{
		(*result)[i] = argv[i];
	}
	for (int j = 0; j < len_obj; i++, j++)
	{
		(*result)[i] = obj[j];
	}
	(*result)[i] = '\0';
}

CCLBOOL Init(char* argv)
{
	//使用加载器前必须先初始化
	if (!CCLModuleLoaderInit())
	{
		printf("%s\n", CCLCurrentErr_EXModule());
		return CCLFALSE;
	}
	char* path = nullptr;
#ifdef CCL_WINDOWS
	gluing_path(argv, "CCL_std.dll", &path);
#elif defined CCL_LINUX
	gluing_path(argv, "libCCL_std.so", &path);
#endif
	cclstd = CCLEXLoad(path);
	if (path)
		delete path;
	if (!cclstd)
	{
		printf("load module failed\n");
		return CCLFALSE;
	}
	CCL_MOD_REQUEST_STRUCTURE req;

	CCLModGetFn(cclstd, CCLSERV_SHORT_SLEEP, &req);
	if (req.func) shortsleep = req.func;
	else goto Failed;

	CCLModGetFn(cclstd, CCLSERV_GET_DATE, &req);
	if (req.func) ccldate = req.func;
	else goto Failed;

	CCLModGetFn(cclstd, CCLSERV_TIMER_CREATE, &req);
	if (req.func) createtimer = req.func;
	else goto Failed;

	CCLModGetFn(cclstd, CCLSERV_TIMER_CLOSE, &req);
	if (req.func) closetimer = req.func;
	else goto Failed;

	CCLModGetFn(cclstd, CCLSERV_TIMER_MARK, &req);
	if (req.func) timermark = req.func;
	else goto Failed;

	CCLModGetFn(cclstd, CCLSERV_TIMER_PEEK, &req);
	if (req.func) timerpeek = req.func;
	else goto Failed;

	CCLModGetFn(cclstd, CCLSERV_THREAD, &req);
	if (req.func) threadserv = req.func;
	else goto Failed;

	CCLModGetFn(cclstd, CCLSERV_TLOCK, &req);
	if (req.func) lockserv = req.func;
	else goto Failed;

	//初始化一些全局变量
	CCL_THREADLOCK_REQ lockr;
	lockr.servid = CCLLOCK_CREATE;
	lockserv(&lockr);
	if (lockr.lock)
		lock1 = lockr.lock;
	else goto Failed;

	return CCLTRUE;
Failed:
	printf("%s\n", req.name);
	return CCLFALSE;
}

static int counter = 0;

void* func1(void* data, CCLTHREADID idThis)
{
	//加锁
	CCL_THREADLOCK_REQ lockr;
	lockr.servid = CCLLOCK_TRY;
	lockr.lock = lock1;
	lockr.idThis = idThis;
	lockr.status = CCLFALSE;
	while (!lockr.status) lockserv(&lockr);
	//
	printf("idThis: %d\n", (int)idThis);
	for (int i = 0; i < 100; i++)
	{
		std::cout << "func1:" << i << std::endl;
		counter++;
	}
	//解锁
	lockr.servid = CCLLOCK_UNLOCK;
	lockserv(&lockr);
	//
	return NULL;
};

void* func2(void* data, CCLTHREADID idThis)
{
	//加锁
	CCL_THREADLOCK_REQ lockr;
	lockr.servid = CCLLOCK_TRY;
	lockr.lock = lock1;
	lockr.idThis = idThis;
	lockr.status = CCLFALSE;
	while (!lockr.status) lockserv(&lockr);
	//
	printf("idThis: %d\n", (int)idThis);
	for (int i = 0; i < 100; i++)
	{
		std::cout << "func2:" << i << std::endl;
		counter++;
	}
	//解锁
	lockr.servid = CCLLOCK_UNLOCK;
	lockserv(&lockr);
	//
	return NULL;
}

int main(int argc, char** argv)
{
	std::cout << argv[0] << std::endl;
	CCLRejisterClearFunc(trash);

	Version();

	if (!Init(argv[0]))
		return 1;
	
	//获取日期和系统时间
	CCLDATE_REQ date;
	CCLDateStr str;
	date.str = &str;
	ccldate((void*)&date);
	printf("%s\n", (char*)&str);

	//创建计时器
	CCLTIMER timer = NULL;
	CCL_TIMER_REQ timer_r;
	createtimer(&timer_r);
	if (timer_r.timer)
		timer = timer_r.timer;
	else
		return 1;

	//使用计时器测试
	timermark(&timer_r);
	shortsleep((void*)1500);
	timerpeek(&timer_r);
	printf("%f\n", timer_r.time);

	closetimer(&timer_r);

	//线程测试
	CCLTHREADID thread1 = 0, thread2 = 0;
	CCL_THREAD_REQ thr;
	thr.servid = CCLTHREAD_CREATE;
	thr.func = func1;
	thr.userdata = NULL;
	threadserv(&thr);
	thread1 = thr.thread;
	thr.func = func2;
	threadserv(&thr);
	thread2 = thr.thread;
	if (thread1 || thread2)
	{
		thr.servid = CCLTHREAD_WAIT;
		thr.thread = thread1;
		threadserv(&thr);
		thr.thread = thread2;
		threadserv(&thr);
	}

	printf("counter: %d/2000\n", counter);

	return 0;
}