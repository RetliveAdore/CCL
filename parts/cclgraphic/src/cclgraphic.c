#include "cclgraphicinner.h"

static CCLVERSION* pVersion = NULL;
static CCLMODService service = NULL;
static CCLDataStructure flist = NULL;
static CCLUINT64 myuuid = 0;

/*
* 下方为各方的全局变量
*/

//该模块需要用到前置模块CCLstd
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
	if (!_inner_cclgraphic_init(service, cclstd))
		goto Failed;

	//填充函数列表
	if (CCLModPutFn_mod(service, flist, msgbox_ccl, CCLSERV_MSGBOX, &req))
		printf("enabled CCLSERV_MSGBOX:%s\n", CCLGRAPHIC_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, windowserv_ccl, CCLSERV_WINDOW, &req))
		printf("enabled CCLSERV_WINDOW:%s\n", CCLGRAPHIC_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, onquit_ccl, CCLSERV_ONQUIT, &req))
		printf("enabled CCLSERV_ONQUIT:%s\n", CCLGRAPHIC_MODNAME);
	else goto Failed;

	return CCLGRAPHIC_MODNAME;
Failed:
	return NULL;
}

CCLMODAPI void CCLEXexit()
{
	_inner_cclgraphic_clear();
}