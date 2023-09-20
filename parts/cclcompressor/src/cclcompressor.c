#include "cclcompressorinner.h"

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

	CCL_MOD_REQUEST_STRUCTURE req;
	//填充函数列表
	if (CCLModPutFn_mod(service, flist, informationentropy_ccl, CCLSERV_INFENTROPY, &req))
		printf("enabled CCLSERV_INFENTROPY:%s\n", CCLCOMPRESSOR_MODNAME);
	else goto Failed;
	if (CCLModPutFn_mod(service, flist, compress_lz77_ccl, CCLSERV_LZ77, &req))
		printf("enabled CCLSERV_LZ77:%s\n", CCLCOMPRESSOR_MODNAME);
	else goto Failed;

	return CCLCOMPRESSOR_MODNAME;
Failed:
	return NULL;
}

CCLMODAPI void CCLEXexit()
{}