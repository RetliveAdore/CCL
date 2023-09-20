#ifndef _INCLUDE_CCLCODER_H_
#define _INCLUDE_CCLCODER_H_

#include <CCL.h>

#define CCLCOMPRESSOR_MODNAME "CCLCoder"

//计算信息熵
#define CCLSERV_INFENTROPY 10
typedef struct ccl_entropy_req
{
	CCLUINT8* data;
	CCLUINT64 size;
	float entropy;
}CCL_INFENTROPY_REQ, *PCCL_INFENTROPY_REQ;

#define CCL_COMPRESS   0
#define CCL_DECOMPRESS 1
#define CCL_CLEARDYN   2

#define CCLSERV_LZ77 20
typedef struct ccl_lz77_req
{
	//选择压缩或者解压缩
	CCLUINT8 serv;
	CCLUINT8* Src;  //源数据
	CCLDataStructure Dyn; //输出数据
	CCLUINT64 in_size; //源数据大小
	CCLUINT64 out_size; //输出数据大小
}CCL_LZ77SERV_REQ, *PCCL_LZ77SERV_REQ;

#endif //include