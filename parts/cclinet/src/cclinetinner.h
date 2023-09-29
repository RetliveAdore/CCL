#ifndef _INCLUDE_CCLINET_INNER_
#define _INCLUDE_CCLINET_INNER_

#include <cclinet.h>
#include <stdio.h>
#include <cclstd.h>
#include <malloc.h>

#define CCL_TYPE_SERVER 1
#define CCL_TYPE_CLIENT 2

typedef struct ccl_socket
{
	CCLUINT32 soc;
	CCLUINT8 type;
	CCLUINT64 id;
	CCLBOOL alive;
}CCL_SOCKET_INNER, *PCCL_SOCKET_INNER;

void server_ccl(void* req);
void client_ccl(void* req);
void close_ccl(void* req);

void stream_ccl(void* req);
void inetclean_ccl(void* req);

CCLBOOL _inner_inetinit_ccl(CCLMODService service, CCLMODID id);
void _inner_inetclear_ccl();

#endif