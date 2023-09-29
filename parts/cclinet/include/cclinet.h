#ifndef _INCLUDE_CCLINET_H_
#define _INCLUDE_CCLINET_H_

#include <CCL.h>
#include <cclstd.h>

#define CCLINET_MODNAME "CCLinet"

//socket也是内部托管的，用户拿到的是对应的ID
typedef CCLUINT64 CCL_SOCKET;

typedef void (*CCLInetProcess)(CCL_SOCKET soc);

//直接传入一个CCLBOOL的指针
//一定要确保退出的时候socket池子干干净净
#define CCLSERV_INET_IFCLEAN 1

#define CCLSERV_INET_SERVER 10
typedef struct ccl_server_req
{
	CCL_SOCKET soc;
	CCLUINT16 port;

	//未完成队列容量上限
	CCLUINT32 maxAccepts;
	//成功建立链接后用于处理链接的函数（多线程）
	CCLInetProcess processFn;
	char* error;
}CCL_SERVER_REQ, *PCCL_SERVER_REQ;

#define CCLSERV_INET_CLIENT 20
typedef struct ccl_client_req
{
	CCL_SOCKET soc;
	char* addr;
	CCLUINT16 port;
	CCLUINT8 timeout;  //seconds
	char* error;
}CCL_CLIENT_REQ, *PCCL_CLIENT_REQ;

#define CCLSERV_INET_CLOSE 30

/*
* 读写结构都是差不多的
* buffersize会作为实际读写的字节数返回
*/

#define CCLSERV_INET_STREAM 40
#define STREAM_IN 1
#define STREAM_OUT 2
typedef struct ccl_inetread_req
{
	CCLUINT8 serv; //STREAM_IN or STREAM_OUT
	CCL_SOCKET soc;
	CCLUINT8* buffer;
	CCLINT32 buffersize;
	char* error;
	CCLBOOL effective;  //判断连接的有效性
}CCL_INETSTREAM_REQ, *PCCL_INETSTREAM_REQ;

#endif