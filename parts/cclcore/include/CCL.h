/*
* 一切始于CCL_Core，这是唯一需要链接到可执行文件的库
* 其他的都是模块化的，可以任意“拆卸”
* Adore-2023
*/

#ifndef _INCLUDE_CCLCORE_H_
#define _INCLUDE_CCLCORE_H_

#ifdef WIN32
#  define CCL_WINDOWS
#  ifdef CCL_SHARED_LIB
#    ifdef CCL_BUILD_DLL
#      define CCLAPI __declspec(dllexport)
#    else
#      define CCLAPI __declspec(dllimport)
#    endif
#  else
#    define CCLAPI extern
#  endif

#elif __linux__
#  define CCL_LINUX
#  define CCLAPI extern

#else
#  error unsupported platform.
#endif

#ifdef CCL_WINDOWS
#  ifdef CCLENTRY_CRT
#    pragma comment(linker,"/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#  endif
#  pragma comment(linker, "/align:256")
#endif

#ifndef __cplusplus
#  define CCLBOOL  _Bool
#  define CCLFALSE 0
#  define CCLTRUE  1
#  ifndef NULL
#    define NULL (void*)0
#  endif
#else
#  define CCLBOOL bool
#  define CCLFALSE false
#  define CCLTRUE true
#  ifndef NULL
#    define NULL nullptr
#  endif
#endif

#ifdef CCL_WINDOWS
typedef unsigned char CCLUINT8;
typedef unsigned short CCLUINT16;
typedef unsigned int CCLUINT32;
typedef unsigned long long CCLUINT64;
typedef char CCLINT8;
typedef short CCLINT16;
typedef int CCLINT32;
typedef long CCLINT64;

#elif defined CCL_LINUX
#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ CCLINT8;
#else
typedef char CCLINT8;
#endif
#ifdef __INT16_TYPE__
typedef __INT16_TYPE__ CCLINT16;
#else
typedef short CCLINT16;
#endif
#ifdef __INT32_TYPE__
typedef __INT32_TYPE__ CCLINT32;
#else
typedef int CCLINT32;
#endif
#ifdef __INT64_TYPE__
typedef __INT64_TYPE__ CCLINT64;
#else
typedef long CCLINT64
#endif
#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ CCLUINT8;
#else
typedef unsigned char CCLUINT8;
#endif
#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ CCLUINT16;
#else
typedef unsigned short CCLUINT16;
#endif
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ CCLUINT32;
#else
typedef unsigned int CCLUINT32;
#endif
#ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ CCLUINT64;
#else
typedef unsigned long CCLUINT64;
#endif
#endif

#ifdef CCL_WINDOWS
#  define CCL_ICON     100
#  define CCL_ICONSM   101
#  define CCL_CURSOR_1 500
#  define CCL_CURSOR_2 500
#  define CCL_CURSOR_3 501
#  define CCL_CURSOR_4 502
#  define CCL_CURSOR_5 503
#  define CCL_CURSOR_6 504
#  define CCL_CURSOR_7 505
#  define CCL_CURSOR_8 506
#  define CCLMODAPI _declspec(dllexport)
#else
#  define CCLMODAPI extern
#endif

//第一个版本号继承自GCL，后面就自行发展了
#define CCLVERSION_ALPHA   0x01
#define CCLVERSION_BETA    0x02
#define CCLVERSION_RELEASE 0x03
typedef struct
{
	CCLUINT32 MAJOR;
	CCLUINT32 MINOR;
	CCLUINT32 STEP;
	CCLUINT16 CHECK;
	CCLUINT8  VER;
	CCLUINT8  EX;
}CCLVERSION;

//数据结构就是数字生命基础，所以集成到core中
//一般来讲会用到树和线性表
typedef void* CCLDataStructure;
typedef CCLDataStructure* PCCLDataStructure;

//用于清理data指向的内存（用户自定义）
typedef void (*CCLClearCallback)(void* data);
//用于对遍历到的每个data进行操作（用户自定义）
typedef void (*CCLEnumCallback)(void* data, void* user, CCLUINT64 idThis);

//用于主进程退出时的收尾处理，一般用来释放服务和内存
typedef void (*TrashBinFunc)(void);

/*
* 下面的宏定义将服务ID和更容易记忆的明文对应起来
* CCL_MODS是标准开头
*/

#define CCL_MODS_ERROR 0
#define CCL_MODS_GETMOD_BYNAME 1
#define CCL_MODS_GETFUN_BYUUID 2
#define CCL_MODS_PUTFUN 3
#define CCL_MODS_REMFUN 4
/*
*/

/*
* Mod Loader是唯一具有平台差异性且包含在core中的组分
* 在GCL->CCL的过程中，即使是多线程也被分离出去。
* 因为需要Mod Loader来加载并使用那些差异性组件。
* 组件可以热拔插，并且需要遵循CCL的规范。
*
* * * * * * * * * * * * * * * * * * * * * * * *
*
* 能够被GCL加载的模块以动态库方式打包，且动态库必须包含一个：
* const char* CCLEXmain(void** argv, unsigned int argc)
* 函数，加载时默认启动该函数。请将组件名称的字符串作为返回值。
* 这个返回的名字将作为其他组件寻找你的依据。不允许重名，所以说遇
* 到重名的情况请换一个名字
*
* * * * * * * * * * * * * * * * * * * * * * * *
*
* GCLEXexit()函数是可选项，当加载的某些步骤失败或者卸载组件时，
* 该函数会被默认执行。假如没有定义该函数就不执行。
*
*/
typedef CCLUINT64 CCLMODID;
#define INVALID_CCL_MOD (CCLUINT64)0

//通常来说这就是模块提供的功能接口格式，
//void* structure指向一个数据块，
//用于在用户和模块之间交换数据，由用户自行创建。
typedef void (*CCLModFunction)(void* structure);

//服务请求报文数据格式
typedef struct request_structure
{
	CCLUINT8 service_id;//请求的服务
	CCLUINT64 id;       //组件的uuid
	const char* name;   //组件名称或者用于返回错误
	CCLDataStructure fl;//函数列表
	CCLUINT32 fun_id;   //请求的函数id
	CCLModFunction func;//返回的函数
} CCL_MOD_REQUEST_STRUCTURE;
typedef CCL_MOD_REQUEST_STRUCTURE* (*CCLMODService)(CCL_MOD_REQUEST_STRUCTURE* request);

#ifdef __cplusplus
extern "C" {
#endif

CCLAPI void CCLVersion(CCLVERSION* pversion);

CCLAPI CCLDataStructure CCLCreateLinearTable();
CCLAPI CCLDataStructure CCLCreateTreeMap();
CCLAPI CCLDataStructure CCLCreateDynamicArray();
//循环数组，固定大小顺序一圈站，后来的会把先到的挤出去
CCLAPI CCLDataStructure CCLCreateLoopArray(CCLUINT32 size);

CCLAPI CCLBOOL CCLDestroyDataStructure(PCCLDataStructure pStructure, CCLClearCallback callback);
CCLAPI CCLUINT64 CCLDataStructureSize(CCLDataStructure structure);

CCLAPI CCLBOOL CCLLinearTablePut(CCLDataStructure structure, void* data, CCLINT64 offset);
CCLAPI void* CCLLinearTableGet(CCLDataStructure structure, CCLINT64 offset);
CCLAPI void* CCLLinearTableSeek(CCLDataStructure structure, CCLINT64 offset);

CCLAPI CCLBOOL CCLLinearTablePush(CCLDataStructure structure, void* data);
CCLAPI void* CCLLinearTablePop(CCLDataStructure structure);

CCLAPI CCLBOOL CCLLinearTableIn(CCLDataStructure structure, void* data);
CCLAPI void* CCLLinearTableOut(CCLDataStructure structure);

CCLAPI CCLBOOL CCLTreeMapPut(CCLDataStructure tree, void* data, CCLINT64 id);
CCLAPI void* CCLTreeMapGet(CCLDataStructure tree, CCLINT64 id);
CCLAPI void* CCLTreeMapSeek(CCLDataStructure tree, CCLINT64 id);

/*
* 动态数组的最大容量其实是有限制的，为了防止某些脑瘫一上来就把物理内存耗尽。
* 如果达到上限，或者分配内存失败，你将获得错误提示：空间不足，且维持原空间不变
*
* 动态数组不仅能主动扩增空间，也能主动缩减空间，以增加空间使用效率
*/

//用于在动态数组末尾插入数据
CCLAPI CCLBOOL CCLDynamicArrayPush(CCLDataStructure dyn, CCLUINT8 data);
//用于弹出动态数组末尾的数据（成功后数组有效长度减一）
CCLAPI CCLUINT8 CCLDynamicArrayPop(CCLDataStructure dyn);
//用于设置数组内某一下标的值（超过有效长度将扩展容量）
CCLAPI CCLBOOL CCLDynamicArraySet(CCLDataStructure dyn, CCLUINT8 data, CCLUINT64 subscript);
CCLAPI CCLUINT8 CCLDynamicArraySeek(CCLDataStructure dyn, CCLUINT64 subscript);
CCLAPI CCLUINT64 CCLDynamicArrayCopy(CCLDataStructure dyn, CCLUINT8** pOut);

//线性表从前往后顺序遍历，树中序遍历, 动态数组从前往后
CCLAPI CCLBOOL CCLDataStructureEnum(CCLDataStructure structure, CCLEnumCallback callback, void* user);

//收尾（垃圾处理）
CCLAPI void CCLRejisterClearFunc(TrashBinFunc func);

//添加任何组件前都必须先初始化ModuleLoader
CCLAPI CCLBOOL CCLModuleLoaderInit();
//这是CCL最核心的函数之一
CCLAPI CCLMODID CCLEXLoad(const char* path);

//当存在void GCLEXExit()函数时，执行GCLEXExit()，通常用于清理资源
CCLAPI CCLBOOL CCLEXUnload(CCLMODID id);
CCLAPI void CCLModName(CCLMODID id);

//专用于主级（加载方）向次级（被加载方）的一些服务
//比如说用于获取次级的函数列表中的某一服务
CCLAPI const CCL_MOD_REQUEST_STRUCTURE* CCLEXService(CCL_MOD_REQUEST_STRUCTURE* request);

/*
* 一些和函数装载相关的便利封装，纯手动加载或者使用封装都可以
* 如果想要更简洁或者减少行数和体积，建议使用封装（也可以自行封装）
*/

CCLAPI void ccl_gluing_path(const char* argv, const char* obj, char** result);
CCLAPI CCLModFunction CCLModGetFn(CCLMODID mod, CCLUINT32 fnID, CCL_MOD_REQUEST_STRUCTURE* preq);
CCLAPI CCLBOOL CCLModPutFn_mod(CCLMODService serv, CCLDataStructure flist, CCLModFunction func, CCLUINT32 fnID, CCL_MOD_REQUEST_STRUCTURE* preq);
CCLAPI CCLModFunction CCLModGetFn_mod(CCLMODService serv, CCLMODID uuid, CCLUINT32 fnID, CCL_MOD_REQUEST_STRUCTURE* preq);

//每一类的服务都会有一个错误反馈接口
//查询各自最近的错误类型

CCLAPI const char* CCLCurrentErr_Datastructure();
CCLAPI const char* CCLCurrentErr_EXModule();

#ifdef __cplusplus
}
#endif

#define CCLforEach(structure, callback, user) CCLDataStructureEnum(structure, callback, user)

//位操作需要的遮罩 1000_0000
#define CCL_BIT_MASK_1  (CCLUINT16)0x80
//位操作需要的遮罩 0111_1111
#define CCL_BIT_MASK_0  (CCLUINT16)0x7f
#define CCL_BIT_COVER   (CCLUINT16)0xff

#endif //include
