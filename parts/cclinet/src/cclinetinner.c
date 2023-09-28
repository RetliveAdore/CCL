#include "cclinetinner.h"
#include <assert.h>

CCLModFunction threadserv = NULL, shortsleep = NULL;
static CCLDataStructure sockPool = NULL;  //tree
static CCLDataStructure availableID = NULL;  //queue
static CCL_SOCKET CurrentID = 1;

#ifdef CCL_WINDOWS
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
//一些全局变量
WSADATA wsaData;

#elif defined CCL_LINUX
#endif

void inet_clear_callback_ccl(void* data)
{
	PCCL_SOCKET_INNER pInner = data;
	closesocket(pInner->soc);
	free(data);
}

CCLBOOL _init_(CCLMODService service, CCLMODID id)
{
	CCL_MOD_REQUEST_STRUCTURE req;
	if (CCLModGetFn_mod(service, id, CCLSERV_THREAD, &req))
		threadserv = req.func;
	else return CCLFALSE;
	if (CCLModGetFn_mod(service, id, CCLSERV_SHORT_SLEEP, &req))
		shortsleep = req.func;
	else return CCLFALSE;
	return CCLTRUE;
}

#ifdef CCL_WINDOWS

CCLBOOL _inner_inetinit_ccl(CCLMODService service, CCLMODID id)
{
	if (!_init_(service, id))
		goto Failed;

	WORD myVersionRequest = MAKEWORD(2, 2);
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("WSAStartup Failed!\n");
		goto Failed;
	}
	sockPool = CCLCreateTreeMap();
	availableID = CCLCreateLinearTable();

	return CCLTRUE;
Failed:
	return CCLFALSE;
}

void _inner_inetclear_ccl()
{
	CCLDestroyDataStructure(&sockPool, inet_clear_callback_ccl);
	CCLDestroyDataStructure(&availableID, NULL);
	WSACleanup();
}

#elif defined CCL_LINUX

CCLBOOL _inner_inetinit_ccl(CCLMODService service, CCLMODID id)
{
	if (!_init_(service, id))
		goto Failed;

	sockPool = CCLCreateTreeMap();
	availableID = CCLCreateLinearTable();

	return CCLTRUE;
Failed:
	return CCLFALSE;
}

void _inner_inetclear_ccl()
{
	CCLDestroyDataStructure(&sockPool, inet_clear_callback_ccl);
	CCLDestroyDataStructure(&availableID, NULL);
}

#endif

struct process_data
{
	PCCL_SOCKET_INNER soc;
	CCLInetProcess func;
};

//第三棒
void* processThread(void* data, CCLUINT64 idThis)
{
	struct process_data* pProcess = data;

	CCL_SOCKET id;
	if (!(id = CCLLinearTableOut(availableID)))
	{
		id = CurrentID;
		CurrentID++;
	}
	pProcess->soc->id = id;
	CCLTreeMapPut(sockPool, pProcess->soc, id);
	//////////
	pProcess->func(id);

	//////////
	CCLLinearTableIn(availableID, CCLTreeMapGet(sockPool, id));
	free(pProcess->soc);
	free(pProcess);
	return NULL;
}

//第二棒
void* serverSoc(void* data, CCLUINT64 idThis)
{
	struct process_data* pD = data;
	PCCL_SOCKET_INNER pInner = pD->soc;
	fd_set rdfds;
	fd_set rdfds_bk;
	FD_ZERO(&rdfds);
	FD_ZERO(&rdfds_bk);
	FD_SET(pInner->soc, &rdfds_bk);
	struct timeval tv;

	CCL_THREAD_REQ req;
	req.servid = CCLTHREAD_CREATE;
	req.func = processThread;
	//
	int iRet = 0;
	while (pInner->alive)
	{
		rdfds = rdfds_bk;
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		iRet = select(0, &rdfds, NULL, NULL, &tv);
		if (iRet == -1)
		{
			pInner->alive = CCLFALSE;
		}
		else if (iRet > 0)
		{
			struct process_data* pD_thr = malloc(sizeof(struct process_data));
			if (!pD_thr)
				continue;
			pD_thr->soc = malloc(sizeof(CCL_SOCKET_INNER));
			if (!pD_thr->soc)
				continue;
			pD_thr->soc->soc = accept(pInner->soc, NULL, NULL);
			pD_thr->soc->type = 0;
			pD_thr->func = pD->func;
			req.userdata = pD_thr;
			threadserv(&req);
		}
	}
	closesocket(pInner->soc);
	CCLLinearTableIn(availableID, CCLTreeMapGet(sockPool, pInner->id));
	free(pInner);
	free(pD);
	return NULL;
}

//这东西就像接力赛一样，一定要注意下一棒的内存释放操作，不进行释放将导致内存溢出
//第一棒
void server_ccl(void* req)
{
	PCCL_SERVER_REQ server = req;
	if (!server || !server->processFn)
		return;
	//
	server->error = NULL;
	server->soc = 0;
	//非抢占式系统中除非内存耗尽，否则不会出现NULL
	PCCL_SOCKET_INNER pInner = malloc(sizeof(CCL_SOCKET_INNER));
	assert(pInner);
	pInner->alive = CCLTRUE;
	pInner->type = CCL_TYPE_SERVER;
	pInner->soc = socket(AF_INET, SOCK_STREAM, 0);
	if (pInner->soc == INVALID_SOCKET)
	{
		server->error = "create socket error!";
		free(pInner);
		return;
	}
	//非阻塞式的
	int imod = 1;
	if (ioctlsocket(pInner->soc, FIONBIO, (u_long*)&imod) == SOCKET_ERROR)
	{
		server->error = "ioctlsocket error!";
		closesocket(pInner->soc);
		free(pInner);
		return;
	}
	SOCKADDR_IN addr_in;
	addr_in.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(server->port);
	//绑定之后开始监听端口
	if (bind(pInner->soc, (SOCKADDR*)&addr_in, sizeof(SOCKADDR_IN)) < 0)
	{
		server->error = "bind error";
		closesocket(pInner->soc);
		free(pInner);
		return;
	}
	if (listen(pInner->soc, server->maxAccepts) < 0)
	{
		server->error = " listen error";
		closesocket(pInner->soc);
		free(pInner);
	}
	else
	{
		CCL_SOCKET id;
		if (!(id = CCLLinearTableOut(availableID)))
		{
			id = CurrentID;
			CurrentID++;
		}
		pInner->id = id;
		CCLTreeMapPut(sockPool, pInner, id);
		//剩下的工作就是开始监听了
		server->soc = id;
		CCL_THREAD_REQ thread_req;
		thread_req.servid = CCLTHREAD_CREATE;
		thread_req.func = serverSoc;
		struct process_data* pD = malloc(sizeof(CCL_SOCKET_INNER));
		if (pD)
		{
			pD->soc = pInner;
			pD->func = server->processFn;
			thread_req.userdata = pD;
			threadserv(&thread_req);
		}
		else
		{
			closesocket(pInner->soc);
			free(pInner);
		}
	}
}

#ifdef CCL_WINDOWS
PCCL_SOCKET_INNER client_(PCCL_CLIENT_REQ client)
{
	//非抢占式系统中除非内存耗尽，否则不会出现NULL
	PCCL_SOCKET_INNER pInner = malloc(sizeof(CCL_SOCKET_INNER));
	assert(pInner);
	pInner->type = CCL_TYPE_CLIENT;
	//create
	pInner->soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (pInner->soc == INVALID_SOCKET)
	{
		client->error = "create socket error!";
		free(pInner);
		goto Failed;
	}
	int imod = 1;
	//设置为非阻塞模式，以下引自MSDN：
	/*
	* // If iMode = 0, blocking is enabled;
	* // If iMode != 0, non-blocking mode is enabled.
	*/
	if (ioctlsocket(pInner->soc, FIONBIO, (u_long*)&imod) == SOCKET_ERROR)
	{
		client->error = "ioctlsocket error!";
		closesocket(pInner->soc);
		free(pInner);
		goto Failed;
	}
	struct sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(client->port);
	addr_in.sin_addr.S_un.S_addr = inet_addr(client->addr);

	//使用select来进行超时控制
	int iRet = connect(pInner->soc, (struct sockaddr*)&addr_in, sizeof(struct sockaddr_in));
	if (!iRet)  //表明立刻就连上了（杂鱼~ 一点感觉都没有呢~
		return pInner;
	else if (iRet < 0)  //要分情况了（？！什……什么……这种……不可能啊~~💕！！
	{
		iRet = WSAGetLastError();
		if (iRet == WSAEWOULDBLOCK || iRet == WSAEINVAL)  //服务器还没准备好（这么突然就进来……不要~~!💕
		{
			fd_set wtfds;
			struct timeval tv;
			FD_ZERO(&wtfds);
			FD_SET(pInner->soc, &wtfds);
			tv.tv_sec = client->timeout;
			tv.tv_usec = 0;
			iRet = select(0, NULL, &wtfds, NULL, &tv);
			if (iRet < 0)  //出错（嗯~~已经……已经坏掉了💕……
			{
				closesocket(pInner->soc);
				free(pInner);
				goto Error;
			}
			else if (iRet == 0)  //超时了（不要~~不要停下来~~！
			{
				client->error = "time out";
				goto Failed;
			}
			else if (FD_ISSET(pInner->soc, &wtfds))
				return pInner;
		}
		else if (iRet == WSAEISCONN)
			return pInner;
	}
Error:
	client->error = "connect error";
Failed:
	return NULL;
}

#elif defined CCL_LINUX
PCCL_SOCKET_INNER client_(PCCL_CLIENT_REQ client)
{
	//非抢占式系统中除非内存耗尽，否则不会出现NULL
	PCCL_SOCKET_INNER pInner = malloc(sizeof(CCL_SOCKET_INNER));
	assert(pInner);
	pInner->type = CCL_TYPE_CLIENT;
	//create
	pInner->soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (pInner->soc == INVALID_SOCKET)
	{
		client->error = "create socket error!";
		free(pInner);
		goto Failed;
	}
Error:
	client->error = "connect error";
Failed:
	return NULL;
}

#endif

void client_ccl(void* req)
{
	PCCL_CLIENT_REQ client = req;
	if (!client)
		return;
	//
	client->error = NULL;
	client->soc = 0;

	//非阻塞式的
	PCCL_SOCKET_INNER pInner = client_(client);
	if (pInner)
	{
		CCLUINT64 id;
		if (!(id = CCLLinearTableOut(availableID)))
		{
			id = CurrentID;
			CurrentID++;
		}
		pInner->id = id;
		CCLTreeMapPut(sockPool, pInner, id);
		client->soc = id;
	}
}

void stream_ccl(void* req)
{
	PCCL_INETSTREAM_REQ pStream = req;
	if (!pStream)
		return;
	pStream->error = NULL;
	PCCL_SOCKET_INNER pInner = CCLTreeMapSeek(sockPool, pStream->soc);
	if (!pInner)
	{
		pStream->error = "invalid socket";
		pStream->effective = CCLFALSE;
		return;
	}
	if (pStream->serv == STREAM_IN)
	{
		pStream->buffersize = recv(pInner->soc, pStream->buffer, pStream->buffersize, 0);
	}
	else if (pStream->serv == STREAM_OUT)
	{
		pStream->buffer = send(pInner->soc, pStream->buffer, pStream->buffersize, 0);
	}
	else
	{
		pStream->error = "unknown serv";
		return;
	}
	//如果网络断开，就会返回0
	if (pStream->buffersize == 0)
	{
		pStream->effective = CCLFALSE;
		return;
	}
	pStream->effective = CCLTRUE;
}

void close_ccl(void* req)
{
	CCL_SOCKET soc = req;
	if (!soc)
		return;
	PCCL_SOCKET_INNER pInner = CCLTreeMapSeek(sockPool, soc);
	if (!pInner)
		return;
	if (pInner->type == CCL_TYPE_SERVER)
		pInner->alive = CCLFALSE;
	else if (pInner->type == CCL_TYPE_CLIENT)
	{
		closesocket(pInner->soc);
		CCLLinearTableIn(availableID, CCLTreeMapSeek(sockPool, soc));
	}
}

void inetclean_ccl(void* req)
{
	CCLBOOL* cl = req;
	if (!req)
		return;
	if (CCLDataStructureSize(sockPool))
		*cl = CCLFALSE;
	else
		*cl = CCLTRUE;
}
