#include "cclstdinner.h"

CCLDataStructure cclthreadpool = nullptr;
CCLDataStructure cclthreadavailable = nullptr;
static CCLTHREADID currentID = 1;

static const char* ErrPool[] = {
	"Fine\0",
	"On inited\0",
	"Error on thread\0",
	"Invalid thread func\0",
	"Already detached\0",
	"Invalid thread id\0"
};
#define NO_ERROR 0
#define ON_INITED 1
#define ERR_THREAD 2
#define INVALID_THREADFUNC 3
#define REPETITIVE_DETACH 4
#define INVALID_ID 5
static const char* CurrentErr = NULL;

void _inner_threadinit_ccl()
{
	if (!cclthreadpool)
		cclthreadpool = CCLCreateTreeMap();
	if (!cclthreadavailable)
		cclthreadavailable = CCLCreateLinearTable();
	CurrentErr = NO_ERROR;
}

CCLTHREADLOCK CCLCreateLock()
{
	PCCLThreadLockInner pInner = NULL;
	while (!pInner) pInner = new CCLThreadLockInner;
	pInner->id = 0;
	pInner->locked = CCLFALSE;
	pInner->func = nullptr;
	pInner->magic = CCL_LOCKMAGIC;
	return pInner;
}

CCLBOOL CCLReleaseLock(CCLTHREADLOCK lock)
{
	if (lock)
	{
		PCCLThreadLockInner pInner = (PCCLThreadLockInner)lock;
		if (pInner->magic == CCL_LOCKMAGIC)
		{
			delete pInner;
			return CCLTRUE;
		}
	}
	return CCLFALSE;
}

PCCLThreadInner _INNER_CCLSeekThreadInner(CCLTHREADID thread)
{
	return (PCCLThreadInner)CCLTreeMapSeek(cclthreadpool, thread);
}

CCLBOOL CCLTryLock(CCLTHREADID idThis, CCLTHREADLOCK lock)
{
	if (lock)
	{
		PCCLThreadLockInner pInner = (PCCLThreadLockInner)lock;
		if (pInner->magic == CCL_LOCKMAGIC)
		{
			PCCLThreadInner pThread = _INNER_CCLSeekThreadInner(idThis);
			PCCLThreadInner pTh = _INNER_CCLSeekThreadInner(pInner->id);
			if (pThread)
			{
				if (!pInner->locked)
				{
					pInner->locked = CCLTRUE;
					pInner->id = idThis;
					pInner->func = pThread->func;
					return CCLTRUE;
				}
				else
				{
					if (pInner->id == idThis && pInner->func == pThread->func)
					{
						return CCLTRUE;
					}
					else
					{
						if (!pTh)
						{
							pInner->id = idThis;
							return CCLTRUE;
						}
					}
				}
			}
		}
	}
	return CCLFALSE;
}

CCLBOOL CCLUnlock(CCLTHREADID idThis, CCLTHREADLOCK lock)
{
	if (lock)
	{
		PCCLThreadLockInner pInner = (PCCLThreadLockInner)lock;
		if (pInner->magic == CCL_LOCKMAGIC)
		{
			PCCLThreadInner pThread = _INNER_CCLSeekThreadInner(idThis);
			PCCLThreadInner pTh = _INNER_CCLSeekThreadInner(pInner->id);
			if (pInner->locked)
			{
				if (pThread)
				{
					if (pInner->id == idThis)
					{
						pInner->locked = CCLFALSE;
						return CCLTRUE;
					}
					else
					{
						if (!pTh)
						{
							pInner->locked = CCLFALSE;
							return CCLTRUE;
						}
					}
				}
			}
			else
			{
				return CCLTRUE;
			}
		}
	}
	return CCLFALSE;
}

//锁是自己设计的，和平台无关
void lockserv_ccl(void* req)
{
	CCL_THREADLOCK_REQ* lock = (CCL_THREADLOCK_REQ*)req;
	if (lock)
	{
		lock->status = CCLTRUE;
		switch (lock->servid)
		{
		case CCLLOCK_CREATE:
		{
			lock->lock = CCLCreateLock();
			break;
		}
		case CCLLOCK_RELEASE:
		{
			if (!CCLReleaseLock(lock->lock))
				lock->status = CCLFALSE;
			break;
		}
		case CCLLOCK_TRY:
		{
			if (!CCLTryLock(lock->idThis, lock->lock))
				lock->status = CCLFALSE;
			break;
		}
		case CCLLOCK_UNLOCK:
		{
			if (!CCLUnlock(lock->idThis, lock->lock))
				lock->status = CCLFALSE;
			break;
		}
		default:
			break;
		}
	}
}

#ifdef CCL_WINDOWS
#include <Windows.h>

typedef struct windows_thread
{
	CCLThreadInner public_head;
	HANDLE hThread;
	DWORD threadID_w;
} CCLWindowsThread, * PCCLWindowsThread;

//通用的线程函数，用于中转执行用户的函数，抹平平台差异
DWORD WINAPI thread_func_inner(LPVOID lp)
{
	PCCLWindowsThread pInner = (PCCLWindowsThread)lp;
	if (!lp || !pInner->public_head.func)
		goto Failed;
	void* back = pInner->public_head.func(pInner->public_head.userdata, pInner->public_head.id);
	if (pInner->public_head.detach)
	{
		CCLLinearTableIn(cclthreadavailable, (void*)(pInner->public_head.id));
		CCLTreeMapGet(cclthreadpool, pInner->public_head.id);
		free(pInner);
	}
	return (DWORD)back;
Failed:
	CurrentErr = ErrPool[ERR_THREAD];
	return (DWORD)NULL;
}

CCLTHREADID CCLCreateThread(CCLThreadFunc func, void* userData)
{
	if (!func)
	{
		CurrentErr = ErrPool[INVALID_THREADFUNC];
		goto Failed;
	}
	CCLTHREADID id = 0;
	if (!(id = (CCLTHREADID)CCLLinearTableOut(cclthreadavailable)))
	{
		id = currentID;
		currentID++;
	}
	PCCLWindowsThread pInner = NULL;
	while (!pInner) pInner = new CCLWindowsThread;
	pInner->public_head.detach = CCLFALSE;
	pInner->public_head.id = id;
	pInner->public_head.func = func;
	pInner->public_head.userdata = userData;
	pInner->public_head.magic = CCL_THREADMAGIC;
	pInner->hThread = CreateThread(NULL, 0, thread_func_inner, pInner, 0, &(pInner->threadID_w));
	CCLTreeMapPut(cclthreadpool, pInner, id);
	return id;
Failed:
	return 0;
}

void CCLWaitforThread(CCLTHREADID thread)
{
	PCCLWindowsThread pInner = (PCCLWindowsThread)CCLTreeMapSeek(cclthreadpool, thread);
	if (pInner && !pInner->public_head.detach)
	{
		WaitForSingleObject(pInner->hThread, INFINITE);
		CCLLinearTableIn(cclthreadavailable, (void*)thread);
		CCLTreeMapGet(cclthreadpool, thread);
		delete pInner;
	}
}

CCLBOOL CCLDetachThread(CCLTHREADID thread)
{
	PCCLWindowsThread pInner = (PCCLWindowsThread)CCLTreeMapSeek(cclthreadpool, thread);
	if (!pInner)
	{
		CurrentErr = ErrPool[INVALID_ID];
		goto Failed;
	}
	if (pInner->public_head.detach)
	{
		CurrentErr = ErrPool[REPETITIVE_DETACH];
		goto Failed;
	}
	if (!CloseHandle(pInner->hThread))
	{
		CurrentErr = ErrPool[ERR_THREAD];
		goto Failed;
	}
	pInner->public_head.detach = CCLTRUE;
	return CCLTRUE;
Failed:
	return CCLFALSE;
}

CCLBOOL CCLThreadAlive(CCLTHREADID thread)
{
	PCCLWindowsThread pInner = (PCCLWindowsThread)CCLTreeMapSeek(cclthreadpool, thread);
	if (pInner)
	{
		LPWORD exitcode = 0;
		if (GetExitCodeThread(pInner->hThread, (LPDWORD)&exitcode))
		{
			if (exitcode == (LPWORD)STILL_ACTIVE)
				return CCLTRUE;
		}
	}
	return CCLFALSE;
}

static void clear_callback(void* data)
{
	PCCLWindowsThread pInner = (PCCLWindowsThread)data;
	if (pInner && !pInner->public_head.detach)
	{
		CloseHandle(pInner->hThread);
	}
}

void _inner_threadclear_ccl()
{
	CCLDestroyDataStructure(&cclthreadpool, clear_callback);
	CCLDestroyDataStructure(&cclthreadavailable, NULL);
}

#elif defined CCL_LINUX
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

typedef struct linux_thread
{
    CCLThreadInner public_head;
    pthread_t thread;
} CCLLinuxThread, *PCCLLinuxThread;

void* thread_func_inner(void* user)
{
    PCCLLinuxThread pInner = (PCCLLinuxThread)user;
    if (!pInner || !pInner->public_head.func)
        return NULL;
    void* back = NULL;
    back = pInner->public_head.func(pInner->public_head.userdata, pInner->public_head.id);
    if (pInner->public_head.detach)
    {
        CCLLinearTableIn(cclthreadavailable, (void*)(pInner->public_head.id));
        CCLTreeMapGet(cclthreadpool, pInner->public_head.id);
        free(pInner);
    }
    return back;
}

static void clear_callback(void* data)
{
    PCCLLinuxThread pInner = (PCCLLinuxThread)data;
    if (pInner && !pInner->public_head.detach)
    {
        pthread_kill(pInner->thread, SIGKILL);
    }
}

void _inner_threadclear_ccl()
{
	CCLDestroyDataStructure(&cclthreadpool, clear_callback);
	CCLDestroyDataStructure(&cclthreadavailable, NULL);
}

//Linux-gcc的goto比较苛刻，必须保证栈中的变量的安全，需要在所有变量声明之后
//才能goto
CCLAPI CCLTHREADID CCLCreateThread(CCLThreadFunc func, void* userData)
{
    CCLTHREADID id = (CCLTHREADID)CCLLinearTableOut(cclthreadavailable);
    PCCLLinuxThread pInner = NULL;
    if (!func)
    {
        CurrentErr = ErrPool[INVALID_THREADFUNC];
		goto Failed;
    }
    if (!id)
    {
        id = currentID;
        currentID++;
    }
    while (!pInner) pInner = new CCLLinuxThread;
    pInner->public_head.detach = CCLFALSE;
	pInner->public_head.id = id;
	pInner->public_head.func = func;
	pInner->public_head.userdata = userData;
	pInner->public_head.magic = CCL_THREADMAGIC;
    pthread_create(&(pInner->thread), NULL, thread_func_inner, pInner);
    CCLTreeMapPut(cclthreadpool, pInner, id);
    return id;
Failed:
    return 0;
}

void CCLWaitforThread(CCLTHREADID thread)
{
    PCCLLinuxThread pInner = (PCCLLinuxThread)CCLTreeMapSeek(cclthreadpool, thread);
    if (pInner && !pInner->public_head.detach)
    {
        pthread_join(pInner->thread, NULL);
        CCLLinearTableIn(cclthreadavailable, (void*)thread);
		CCLTreeMapGet(cclthreadpool, thread);
		delete pInner;
    }
}

CCLBOOL CCLDetachThread(CCLTHREADID thread)
{
    PCCLLinuxThread pInner = (PCCLLinuxThread)CCLTreeMapSeek(cclthreadpool, thread);
    if (!pInner)
    {
        CurrentErr = ErrPool[INVALID_ID];
        goto Failed;
    }
    if (pInner->public_head.detach)
    {
        CurrentErr = ErrPool[REPETITIVE_DETACH];
        goto Failed;
    }
    if (0 != pthread_detach(pInner->thread))
    {
        CurrentErr = ErrPool[ERR_THREAD];
        goto Failed;
    }
    pInner->public_head.detach = CCLTRUE;
Failed:
    return CCLFALSE;
}

CCLBOOL CCLThreadAlive(CCLTHREADID thread)
{
    PCCLLinuxThread pInner = (PCCLLinuxThread)CCLTreeMapSeek(cclthreadpool, thread);
    if (pInner)
    {
        if (0 == pthread_kill(pInner->thread, 0))
            return CCLTRUE;
    }
    return CCLFALSE;
}

#endif

//这个部分更像是代理执行，没有体现出平台差异，所以说放到了外面
void threadserv_ccl(void* req)
{
	CCL_THREAD_REQ* thread = (CCL_THREAD_REQ*)req;
	if (thread)
	{
		thread->status = CCLTRUE;
		switch (thread->servid)
		{
		case CCLTHREAD_CREATE:
		{
			thread->thread = CCLCreateThread(thread->func, thread->userdata);
			break;
		}
		case CCLTHREAD_WAIT:
		{
			CCLWaitforThread(thread->thread);
			break;
		}
		case CCLTHREAD_DETACH:
		{
			thread->status = CCLDetachThread(thread->thread);
			break;
		}
		case CCLTHREAD_ALIVE:
		{
			thread->status = CCLThreadAlive(thread->thread);
			break;
		}
		default:
			break;
		}
		thread->err = CurrentErr;
	}
}
