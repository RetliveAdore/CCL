#include "inner.h"

CCLAPI void CCLVersion(CCLVERSION* pversion)
{
	if (pversion)
	{
		pversion->MAJOR = 0;
		pversion->MINOR = 0;
		pversion->STEP  = 5;
		pversion->CHECK = 3;
		pversion->VER   = CCLVERSION_ALPHA;
	}
}

//专用于对文件的散列计算
CCLUINT64 CCL_64HASH(const char* path)
{
	struct stat statbuf;
	int ret = stat(path, &statbuf);
	if (ret == 0)
	{
		CCLUINT64 HASH = 0x000000005555aaaa;
		HASH *= statbuf.st_size;
		FILE* fp = fopen(path, "rb");
		if (fp)
		{
			unsigned char tmp = 0;
			while (fread(&tmp, 1, 1, fp))
			{
				HASH += (HASH * (tmp % 5));
				HASH += HASH >> 62;
			}
			fclose(fp);
			return HASH;
		}
	}
	return 0;
}

//专用于对内存块的散列计算
CCLUINT64 CCL_64HASH_MEM(void* data, CCLUINT64 size)
{
	if (data && size)
	{
		CCLUINT64 HASH = 0x000000005555aaaa * size;
		for (size_t seek = 0; seek < size; seek++)
		{
			HASH += (HASH * (((unsigned char*)data)[seek] % 5));
			HASH += HASH >> 62;
		}
		return HASH;
	}
	return 0;
}

/*
* 便利封装，和手动操作没什么区别
*/

CCLAPI CCLModFunction CCLModGetFn(CCLMODID mod, CCLUINT32 funID, CCL_MOD_REQUEST_STRUCTURE* preq)
{
	preq->id = mod;
	preq->service_id = CCL_MODS_GETFUN_BYUUID;
	preq->fun_id = funID;
	CCLEXService(preq);
	return preq->func;
}

CCLAPI CCLBOOL CCLModPutFn_mod(CCLMODService serv, CCLDataStructure flist, CCLModFunction func, CCLUINT32 fnID, CCL_MOD_REQUEST_STRUCTURE* preq)
{
	preq->service_id = CCL_MODS_PUTFUN;
	preq->fl = flist;
	preq->fun_id = fnID;
	preq->func = func;
	serv(preq);
	if (preq->service_id == CCL_MODS_ERROR)
		return CCLFALSE;
	return CCLTRUE;
}

CCLAPI CCLModFunction CCLModGetFn_mod(CCLMODService serv, CCLMODID uuid, CCLUINT32 fnID, CCL_MOD_REQUEST_STRUCTURE* preq)
{
	preq->service_id = CCL_MODS_GETFUN_BYUUID;
	preq->fun_id = fnID;
	preq->id = uuid;
	preq->func = NULL;
	serv(preq);
	return preq->func;
}

//Windos和Linux库文件以及工作目都有点差别
CCLAPI void ccl_gluing_path(const char* argv, const char* obj, char** result)
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

	char* gluing_name = NULL;
#ifdef CCL_WINDOWS
	len_obj += 4;
	gluing_name = malloc(len_obj);
	gluing_name[len_obj - 4] = '.';
	gluing_name[len_obj - 3] = 'd';
	gluing_name[len_obj - 2] = 'l';
	gluing_name[len_obj - 1] = 'l';
	for (int t = 0; t < len_obj - 4; t++)
	{
		gluing_name[t] = obj[t];
	}
#elif defined CCL_LINUX
	len_obj += 6;
	gluing_name = malloc(len_obj);
	gluing_name[0] = 'l';
	gluing_name[1] = 'i';
	gluing_name[2] = 'b';
	gluing_name[len_obj - 3] = '.';
	gluing_name[len_obj - 2] = 's';
	gluing_name[len_obj - 1] = 'o';
	for (int t = 3; t < len_obj - 3; t++)
	{
		gluing_name[t] = obj[t - 3];
	}
#endif

	*result = malloc(len_source + len_obj + 1);
	int i = 0;
	for (i = 0; i < len_source; i++)
	{
		(*result)[i] = argv[i];
	}
	for (int j = 0; j < len_obj; i++, j++)
	{
		(*result)[i] = gluing_name[j];
	}
	(*result)[i] = '\0';
	free(gluing_name);
}

/*
*/

/*
* 下方将会是平台差异化的，组件加载部分
* Windows将会加载PE文件，Linux将会加载ELF文件
* 至于其他操作系统，目前尚未适配
* 后面是存在手动加载二进制文件的可能性的，但是近期
* 不会有这种可能。
* 
* * * * * * * * * * *
* 
* 手动加载二进制函数数据将会有加密解密功能（预定）
* 而且有从内存块加载的能力。
*/

CCLDataStructure nametree; //tree
CCLDataStructure hashtree; //tree
static CCLBOOL onInit = CCLFALSE;

static CCLVERSION g_version = { 0 };

//loadInf旨在为mod提供一些接口和信息引导
static void* loadInf[4];

/*
* 下方是协议中规定的两种将被主体使用的函数的符号格式。
* 至于加载的过程，是交由用户自定义的，
* 自定义加载函数方法仍应满足协议规范。
*/

//载入时执行，返回空指针将导致加载终止（失败）
typedef const char* (*CCLEXmain)(void** argv, unsigned int argc);
//关闭、加载终止时执行
typedef void (*CCLEXexit)();
/*
*/

//下方为错误处理相关
static const char* ErrPool[] = {
	"Fine\0",
	"Module file not fond\0",
	"Invalid module\0",
	"Repetitive init\0",
	"No module name\0",
	"Repetitive load\0"
};
#define NO_ERR 0
#define FILE_NOT_FOND 1
#define INVALID_MODULE 2
#define ON_INITED 3
#define NO_MODNAME 4
#define ON_LOAD 5
static const char* CurrentErr = NULL;

//--------------------------

static void clear_callback(void* data);

void gcl_mod_auto_clear()
{
	CCLDestroyDataStructure(&hashtree, clear_callback);
	CCLDestroyDataStructure(&nametree, NULL);
}

CCLAPI CCLBOOL CCLModuleLoaderInit()
{
	if (!onInit)
	{
		hashtree = CCLCreateTreeMap();
		nametree = CCLCreateTreeMap();
		CCLRejisterClearFunc(gcl_mod_auto_clear);
		CCLVersion(&g_version);
		loadInf[0] = &g_version; //此结构体用于提供版本信息，尽可能避免版本不兼容
		loadInf[1] = MService_Inner;
		onInit = CCLTRUE;
		CurrentErr = ErrPool[NO_ERR];
		return CCLTRUE;
	}
	CurrentErr = ErrPool[ON_INITED];
	return CCLFALSE;
}

CCLAPI const char* CCLCurrentErr_EXModule()
{
	if (!CurrentErr)
		CurrentErr = ErrPool[NO_ERR];
	return CurrentErr;
}

CCLAPI void CCLModName(CCLMODID id)
{
	PCCL_MODITEM pMod = CCLTreeMapSeek(hashtree, id);
	if (pMod)
	{
		printf("name:%s\n", pMod->name);
		printf("uuid:%x", (CCLUINT32)(pMod->uuid_hash >> 32));
		printf("%x\n", (CCLUINT32)(pMod->uuid_hash & 0x00000000ffffffff));
		printf("nmid:%x", (CCLUINT32)(pMod->hash_name >> 32));
		printf("%x\n", (CCLUINT32)(pMod->hash_name & 0x00000000ffffffff));
	}
}

static const CCL_MOD_REQUEST_STRUCTURE err_ret = {
	CCL_MODS_ERROR, //service_id
	0,              //id
	"nullptr error",//name
	NULL,           //fl
	0,              //fun_id
	NULL            //func
};

//这个函数是给主体使用的，下面那个一模一样的函数将提供给客体
//用以避免释放下面级库文件时与主体的这个函数符号发生冲突
CCLAPI const CCL_MOD_REQUEST_STRUCTURE* CCLEXService(CCL_MOD_REQUEST_STRUCTURE* request)
{
	if (!request)
		return &err_ret;
	switch (request->service_id)
	{
	case CCL_MODS_ERROR:
		break;
	case CCL_MODS_GETMOD_BYNAME:
	{
		PCCL_MODITEM pMod = CCLTreeMapSeek(nametree, CCL_64HASH_MEM((void*)(request->name), strlen(request->name)));
		if (pMod)
		{
			request->id = pMod->uuid_hash;
		}
		else
		{
			request->service_id = CCL_MODS_ERROR;
			request->name = "mod not fond";
		}
		break;
	}
	case CCL_MODS_GETFUN_BYUUID:
	{
		PCCL_MODITEM pMod = CCLTreeMapSeek(hashtree, request->id);
		if (pMod)
		{
			request->func = CCLTreeMapSeek(pMod->func_list, request->fun_id);
			if (!request->func)
			{
				request->service_id = CCL_MODS_ERROR;
				request->name = "function not fond";
			}
		}
		else
		{
			request->service_id = CCL_MODS_ERROR;
			request->name = "mod not fond";
		}
		break;
	}
	case CCL_MODS_PUTFUN:
	{
		if (request->fl)
			CCLTreeMapPut(request->fl, request->func, request->fun_id);
		break;
	}
	case CCL_MODS_REMFUN:
	{
		if (request->fl)
			CCLTreeMapGet(request->fl, request->fun_id);
		break;
	}
	default:
		request->service_id = CCL_MODS_ERROR;
		request->name = "unknown service id";
		break;
	}
	return request;
}

//如果不分开写，释放符号的时候计算机就会找不着北。
const CCL_MOD_REQUEST_STRUCTURE* MService_Inner(CCL_MOD_REQUEST_STRUCTURE* request)
{
	if (!request)
		return &err_ret;
	switch (request->service_id)
	{
	case CCL_MODS_ERROR:
		break;
	case CCL_MODS_GETMOD_BYNAME:
	{
		PCCL_MODITEM pMod = CCLTreeMapSeek(nametree, CCL_64HASH_MEM((void*)(request->name), strlen(request->name)));
		if (pMod)
		{
			request->id = pMod->uuid_hash;
		}
		else
		{
			request->service_id = CCL_MODS_ERROR;
			request->name = "mod not fond";
		}
		break;
	}
	case CCL_MODS_GETFUN_BYUUID:
	{
		PCCL_MODITEM pMod = CCLTreeMapSeek(hashtree, request->id);
		if (pMod)
		{
			request->func = CCLTreeMapSeek(pMod->func_list, request->fun_id);
			if (!request->func)
			{
				request->service_id = CCL_MODS_ERROR;
				request->name = "function not fond";
			}
		}
		else
		{
			request->service_id = CCL_MODS_ERROR;
			request->name = "mod not fond";
		}
		break;
	}
	case CCL_MODS_PUTFUN:
	{
		if (request->fl)
			CCLTreeMapPut(request->fl, request->func, request->fun_id);
		break;
	}
	case CCL_MODS_REMFUN:
	{
		if (request->fl)
			CCLTreeMapGet(request->fl, request->fun_id);
		break;
	}
	default:
		request->service_id = CCL_MODS_ERROR;
		request->name = "unknown service id";
		break;
	}
	return request;
}

#ifdef CCL_WINDOWS
#include <Windows.h>

typedef struct windows_dll
{
	CCL_MODITEM public_item;
	HINSTANCE hDll;
	CCLEXmain exMain;
} CCL_MOD_WINDOWS, * PCCL_MOD_WINDOWS;

//在main退出后执行，无法打印字符串（应该是标准库抢先一步被释放了），其他操作正常执行
//尽可能使用GCL自带的函数。
//然而在使用静态链接编译时就没有这个问题
static void clear_callback(void* data)
{
	PCCL_MOD_WINDOWS pMod = data;

	CCLEXexit exitfunc = GetProcAddress(pMod->hDll, "CCLEXexit");
	if (exitfunc)
		exitfunc();
	FreeLibrary(pMod->hDll);
	CCLTreeMapGet(nametree, pMod->public_item.hash_name);
	CCLDestroyDataStructure(&(pMod->public_item.func_list), NULL);
	free(pMod);
}

CCLAPI CCLMODID CCLEXLoad(const char* path)
{
	PCCL_MOD_WINDOWS pWin = NULL;
	while (!pWin) pWin = malloc(sizeof(CCL_MOD_WINDOWS));
	pWin->public_item.uuid_hash = CCL_64HASH(path);
	if (CCLTreeMapSeek(hashtree, pWin->public_item.uuid_hash))
	{
		CurrentErr = ErrPool[ON_LOAD];
		free(pWin);
		goto Failed;
	}
	pWin->hDll = LoadLibrary(path);
	if (pWin->hDll == NULL)
	{
		CurrentErr = ErrPool[FILE_NOT_FOND];
		free(pWin);
		goto Failed;
	}
	pWin->exMain = (CCLEXmain)GetProcAddress(pWin->hDll, "CCLEXmain");
	if (pWin->exMain == NULL)
	{
		CurrentErr = ErrPool[INVALID_MODULE];
		FreeLibrary(pWin->hDll);
		free(pWin);
		goto Failed;
	}
	////////////////////////////////////////
	pWin->public_item.func_list = CCLCreateTreeMap();
	loadInf[2] = pWin->public_item.func_list;
	loadInf[3] = pWin->public_item.uuid_hash;
	pWin->public_item.name = pWin->exMain(loadInf, 4);
	if (!pWin->public_item.name)
	{
		CurrentErr = ErrPool[NO_MODNAME];
		CCLEXexit exitfunc = GetProcAddress(pWin->hDll, "CCLEXexit");
		if (exitfunc)
			exitfunc();
		FreeLibrary(pWin->hDll);
		CCLDestroyDataStructure(&(pWin->public_item.func_list), NULL);
		free(pWin);
		goto Failed;
	}
	pWin->public_item.hash_name = CCL_64HASH_MEM(pWin->public_item.name, strlen(pWin->public_item.name));
	CCLTreeMapPut(hashtree, pWin, pWin->public_item.uuid_hash);
	CCLTreeMapPut(nametree, pWin, pWin->public_item.hash_name);
	return pWin->public_item.uuid_hash;
Failed:
	return 0;
}

CCLAPI CCLBOOL CCLEXUnload(CCLMODID id)
{
	PCCL_MOD_WINDOWS pWin = CCLTreeMapGet(hashtree, id);
	if (!pWin)
	{
		CurrentErr = ErrPool[INVALID_MODULE];
		goto Failed;
	}
	CCLEXexit exitfunc = GetProcAddress(pWin->hDll, "GCLEXexit");
	if (exitfunc)
		exitfunc();
	FreeLibrary(pWin->hDll);
	CCLTreeMapGet(nametree, pWin->public_item.hash_name);
	CCLDestroyDataStructure(&(pWin->public_item.func_list), NULL);
	free(pWin);
	return CCLTRUE;
Failed:
	return CCLFALSE;
}

#elif defined CCL_LINUX
#include <dlfcn.h>
#include <string.h>

CCLUINT64 tmp = 0;

typedef struct linux_so
{
	CCL_MODITEM public_item;
	void* handle;
	CCLEXmain exMain;
} CCL_MOD_LINUX, * PCCL_MOD_LINUX;

//然而在linux上并没有出现Windows中“不打印”的问题
//无论静态还是动态编译
//这表明：Linux才算得上健全
static void clear_callback(void* data)
{
	PCCL_MOD_LINUX pMod = data;
	CCLEXexit exitfunc = dlsym(pMod->handle, "CCLEXexit");
	if (exitfunc)
		exitfunc();
	dlclose(pMod->handle);
	CCLDestroyDataStructure((PCCLDataStructure) & (pMod->public_item.func_list), NULL);
}

CCLAPI CCLMODID CCLEXLoad(const char* path)
{
	PCCL_MOD_LINUX pLin = NULL;
	while (!pLin) pLin = malloc(sizeof(CCL_MOD_LINUX));
	pLin->public_item.uuid_hash = CCL_64HASH(path);
	if (CCLTreeMapSeek(hashtree, pLin->public_item.uuid_hash))
	{
		CurrentErr = ErrPool[ON_LOAD];
		free(pLin);
		goto Failed;
	}
	pLin->handle = dlopen(path, RTLD_LAZY);
	if (!pLin->handle)
	{
		printf("dlopen - %s\n", dlerror());
		CurrentErr = ErrPool[FILE_NOT_FOND];
		free(pLin);
		goto Failed;
	}
	pLin->exMain = (CCLEXmain)dlsym(pLin->handle, "CCLEXmain");
	if (!pLin->exMain)
	{
		CurrentErr = ErrPool[INVALID_MODULE];
		dlclose(pLin->handle);
		free(pLin);
		goto Failed;
	}
	////////////////////////////////////////
	pLin->public_item.func_list = CCLCreateTreeMap();
	loadInf[2] = (void*)(pLin->public_item.func_list);
	loadInf[3] = (void*)(pLin->public_item.uuid_hash);
	pLin->public_item.name = pLin->exMain(loadInf, 4);
	if (!pLin->public_item.name)
	{
		CurrentErr = ErrPool[NO_MODNAME];
		CCLEXexit exitfunc = dlsym(pLin->handle, "CCLEXexit");
		if (exitfunc)
			exitfunc();
		dlclose(pLin->handle);
		tmp = (CCLUINT64)(pLin->public_item.func_list);
		CCLDestroyDataStructure((PCCLDataStructure) & (pLin->public_item.func_list), NULL);
		free(pLin);
		goto Failed;
	}
	pLin->public_item.hash_name = CCL_64HASH_MEM((void*)(pLin->public_item.name), strlen(pLin->public_item.name));
	CCLTreeMapPut(hashtree, pLin, pLin->public_item.uuid_hash);
	CCLTreeMapPut(nametree, pLin, pLin->public_item.hash_name);
	return pLin->public_item.uuid_hash;
Failed:
	return 0;
}

CCLAPI CCLBOOL CCLEXUnload(CCLMODID id)
{
	PCCL_MOD_LINUX pLin = CCLTreeMapGet(hashtree, id);
	if (!pLin)
	{
		CurrentErr = ErrPool[INVALID_MODULE];
		goto Failed;
	}
	CCLEXexit exitfunc = dlsym(pLin->handle, "CCLEXexit");
	if (exitfunc)
		exitfunc();
	dlclose(pLin->handle);
	CCLTreeMapGet(nametree, pLin->public_item.hash_name);
	CCLDestroyDataStructure((PCCLDataStructure) & (pLin->public_item.func_list), NULL);
	free(pLin);
	return CCLTRUE;
Failed:
	return CCLFALSE;
}

#endif