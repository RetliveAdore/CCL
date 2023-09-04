#ifndef _INCLUDE_INNER_H_
#define _INCLUDE_INNER_H_

#include <CCL.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/stat.h>

#include <string.h>

#define CCLDATASTRUCTURE_TYPE_LINEAR 0x01
#define CCLDATASTRUCTURE_TYPE_TREE   0x02

typedef struct
{
	CCLUINT16 type;
	CCLUINT64 total;
} CCLDataStructurePublic;

/*
* lineartable bellow
* 线性表
*/

typedef struct linear
{
	void* data;
	struct linear* prior;
	struct linear* after;
} CCLLinearTableNode, * PCCLLinearTableNode;

typedef struct
{
	CCLDataStructurePublic head;
	PCCLLinearTableNode hook;
} CCLLinearTable, * PCCLLinearTable;

/*
* treemap bellow
* 树
*/

typedef struct tree_node
{
	CCLINT64 id;
	void* data;
	CCLBOOL red;
	struct tree_node* parent;
	struct tree_node* left;
	struct tree_node* right;
} CCLTreeMapNode, * PCCLTreeMapNode;

typedef struct
{
	CCLDataStructurePublic head;
	PCCLTreeMapNode root;
} CCLTreeMap, * PCCLTreeMap;

//----------------------------------------

//专用于CCL加载组件的哈希鉴定
CCLUINT64 CCL_64HASH(const char* path);
CCLUINT64 CCL_64HASH_MEM(void* data, CCLUINT64 size);

/*
* moduleloader bellow
*/

typedef struct mod_item
{
	CCLUINT64 uuid_hash;
	CCLUINT64 hash_name;
	const char* name;
	PCCLDataStructure func_list;
} CCL_MODITEM, * PCCL_MODITEM;

const CCL_MOD_REQUEST_STRUCTURE* MService_Inner(CCL_MOD_REQUEST_STRUCTURE* structure);

#endif //include
