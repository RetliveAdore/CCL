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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#define closesocket(soc) close(soc)

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
	if (!(id = (CCL_SOCKET)CCLLinearTableOut(availableID)))
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
		iRet = select(pInner->soc + 1, &rdfds, NULL, NULL, &tv);
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
	if (pInner->soc == -1)
	{
		server->error = "create socket error!";
		free(pInner);
		return;
	}
	//非阻塞式的
	int socketFlag = fcntl(pInner->soc, F_GETFL, 0) | O_NONBLOCK;
	if (fcntl(pInner->soc, F_SETFL, socketFlag) == -1)
	{
		server->error = "fcntl error!";
		closesocket(pInner->soc);
		free(pInner);
		return;
	}
	struct sockaddr_in addr_in;
	addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(server->port);
	//绑定之后开始监听端口
	if (bind(pInner->soc, (struct sockaddr*)&addr_in, sizeof(struct sockaddr_in)) < 0)
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
		if (!(id = (CCL_SOCKET)CCLLinearTableOut(availableID)))
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

#endif  //Linux

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
	if (iRet == 0)  //表明立刻就连上了（杂鱼~ 一点感觉都没有呢~
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
	closesocket(pInner->soc);
	free(pInner);
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
	if (pInner->soc == -1)
	{
		client->error = "create socket error!";
		free(pInner);
		goto Failed;
	}
	//设置为非阻塞模式
	int socketFlag = fcntl(pInner->soc, F_GETFL, 0) | O_NONBLOCK;
	if (fcntl(pInner->soc, F_SETFL, socketFlag) == -1)
	{
		client->error = "fcntl error!";
		closesocket(pInner->soc);
		free(pInner);
		goto Failed;
	}

	struct sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(client->port);
	addr_in.sin_addr.s_addr = inet_addr(client->addr);
	//使用select来进行超时控制
	int iRet = connect(pInner->soc, (struct sockaddr*)&addr_in, sizeof(struct sockaddr_in));
	if (iRet == 0)  //表明立刻就连上了（哈——啊～！第一次的感觉～
		return pInner;
	else if (iRet < 0)
	{
		if (errno == EINPROGRESS)  //要分情况了（？！要……忍不住了……！！
		{
			fd_set wtfds;
			struct timeval tv;
			FD_ZERO(&wtfds);
			FD_SET(pInner->soc, &wtfds);
			tv.tv_sec = client->timeout;
			tv.tv_usec = 0;
			//Linux有一个特点，使用回环地址127.0.0.1
			//socket会立即发生变化（这种信号表示连接成功），即使没有开启对应端口的服务端
			//只有在使用外网地址时才会有因为连不上而超时的现象
			//这一点很坑（猜测是被系统代管了）
			//
			//好吧，我观察到这一现象的端倪了，当client刚刚运行时立刻启动server,会报bind error
			//这就说明client启动后有什么东西以服务端的名义占用了端口，server应该先于客户
			//端启动，然后就正常了
			iRet = select(pInner->soc + 1, NULL, &wtfds, NULL, &tv);
			if (iRet < 0)  //出错（～～啊～哈啊！～～～～～
				goto Error;
			else if (iRet == 0)  //超时了（果然……还是输给你了呢～
			{
				client->error = "time out";
				goto Failed;
			}
			else if (iRet == 1 && FD_ISSET(pInner->soc, &wtfds))
				return pInner;
		}
	}
Error:
	closesocket(pInner->soc);
	free(pInner);
	client->error = "connect error";
Failed:
	return NULL;
}

#endif  //Linux

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
		if (!(id = (CCLUINT64)CCLLinearTableOut(availableID)))
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
		pStream->buffersize = send(pInner->soc, pStream->buffer, pStream->buffersize, 0);
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
	CCL_SOCKET soc = (CCLUINT64)req;
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
