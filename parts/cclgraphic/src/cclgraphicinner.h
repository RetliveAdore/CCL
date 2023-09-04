#ifndef _INCLUDE_CCLGRAPHICINNER_H_
#define _INCLUDE_CCLGRAPHICINNER_H_

#include <cclgraphic.h>
#include <stdio.h>
#include <cclstd.h>

#ifdef __cplusplus
extern "C" {
#endif

void msgbox_ccl(void* req);
void windowserv_ccl(void* req);
void onquit_ccl(void* ifquit);

/*
* 内部使用函数
*/

CCLBOOL _inner_cclgraphic_init(CCLMODService service, CCLMODID id);
void _inner_cclgraphic_clear();

#ifdef __cplusplus
}
#endif

#endif
