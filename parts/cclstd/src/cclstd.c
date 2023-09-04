#include "cclstdinner.h"

static CCLVERSION* pVersion = NULL;
static CCLMODService service = NULL;
static CCLDataStructure flist = NULL;
static CCLUINT64 myuuid = 0;

/*
* 下方为各方的全局变量
*/


/*
*/

CCLMODAPI const char* CCLEXmain(void** argv, int argc)
{
	/*
	* argv[0] Version
	* argv[1] GCLModService
	* argv[2] function list
	* argv[3] your own uuid
	*/
	pVersion = argv[0];
	service = argv[1];
	flist = argv[2];
	myuuid = (CCLUINT64)argv[3];

	if (!_inner_timersetup_ccl())
		return NULL;

	//开始填充函数列表
	CCL_MOD_REQUEST_STRUCTURE req;
	if (CCLModPutFn_mod(service, flist, createtimer_ccl, CCLSERV_TIMER_CREATE, &req))
		printf("enabled CCLSERV_TIMER_CREATE:%s\n", CCLSTD_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, closetimer_ccl, CCLSERV_TIMER_CLOSE, &req))
		printf("enabled CCLSERV_TIMER_CLOSE:%s\n", CCLSTD_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, peektimer_ccl, CCLSERV_TIMER_PEEK, &req))
		printf("enabled CCLSERV_TIMER_PEEK:%s\n", CCLSTD_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, marktimer_ccl, CCLSERV_TIMER_MARK, &req))
		printf("enabled CCLSERV_TIMER_MARK:%s\n", CCLSTD_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, shortsleep_ccl, CCLSERV_SHORT_SLEEP, &req))
		printf("enabled CCLSERV_SHORT_SLEEP:%s\n", CCLSTD_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, date_ccl, CCLSERV_GET_DATE, &req))
		printf("enabled CCLSERV_GET_DATE:%s\n", CCLSTD_MODNAME);
	else goto Failed;

	_inner_threadinit_ccl();

	if (CCLModPutFn_mod(service, flist, threadserv_ccl, CCLSERV_THREAD, &req))
		printf("enabled CCLSERV_THREAD:%s\n", CCLSTD_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, lockserv_ccl, CCLSERV_TLOCK, &req))
		printf("enabled CCLSERV_TLOCK:%s\n", CCLSTD_MODNAME);
	else goto Failed;

	return CCLSTD_MODNAME;
Failed:
	return NULL;
}

CCLMODAPI void CCLEXexit()
{
	_inner_threadclear_ccl();
}