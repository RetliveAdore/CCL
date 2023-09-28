#include "cclinetinner.h"

static CCLVERSION* pVersion = NULL;
static CCLMODService service = NULL;
static CCLDataStructure flist = NULL;
static CCLUINT64 myuuid = 0;

/*
* 下方为各方的全局变量
*/

//该模块需要用到前置模块CCLstd（多线程）
CCLMODID cclstd;

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

	CCL_MOD_REQUEST_STRUCTURE req;
	req.id = 0;
	req.service_id = CCL_MODS_GETMOD_BYNAME;
	req.name = CCLSTD_MODNAME;
	service(&req);
	if (!req.id)
	{
		printf("Module 'CCLstd' not found!!!\n");
		return NULL;
	}
	cclstd = req.id;
	if (!_inner_inetinit_ccl(service, cclstd))
		goto Failed;

	//填充函数列表
	if (CCLModPutFn_mod(service, flist, client_ccl, CCLSERV_INET_CLIENT, &req))
		printf("enabled CCLSERV_INET_CLIENT:%s\n", CCLINET_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, server_ccl, CCLSERV_INET_SERVER, &req))
		printf("enabled CCLSERV_INET_SERVER:%s\n", CCLINET_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, stream_ccl, CCLSERV_INET_STREAM, &req))
		printf("enabled CCLSERV_INET_STREAM:%s\n", CCLINET_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, close_ccl, CCLSERV_INET_CLOSE, &req))
		printf("enabled CCLSERV_INET_CLOSE:%s\n", CCLINET_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, inetclean_ccl, CCLSERV_INET_IFCLEAN, &req))
		printf("enabled CCLSERV_INET_IFCLEAN:%s\n", CCLINET_MODNAME);
	else goto Failed;

	return CCLINET_MODNAME;
Failed:
	return NULL;
}

CCLMODAPI void CCLEXexit()
{
	_inner_inetclear_ccl();
}
