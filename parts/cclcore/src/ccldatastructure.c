#include "inner.h"
#include <assert.h>

//限制动态数组最大容量为512MB
#define CCL_DYN_MAXSIZE (1024 * 1024 * 512 / sizeof(void*))

static const char* ErrPool[] = {
	"Fine\0",
	"Invalid structure\0",
	"NULL ptr\0",
	"Not fond\0",
	"Run out of memory\n"
};
#define NO_ERR 0
#define WRONG_STRUCTURE 1
#define NULLPTR_ERR 2
#define NOTFOND_ERR 3
#define OUTOF_MEMORY 4
static const char* CurrentErr = NULL;

CCLAPI CCLDataStructure CCLCreateLinearTable()
{
	PCCLLinearTable pInner = malloc(sizeof(CCLLinearTable));
	assert(pInner);
	pInner->head.total = 0;
	pInner->head.type = CCLDATASTRUCTURE_TYPE_LINEAR;
	pInner->hook = NULL;
	return pInner;
}

CCLAPI CCLDataStructure CCLCreateTreeMap()
{
	PCCLTreeMap pInner = malloc(sizeof(CCLTreeMap));
	assert(pInner);
	pInner->head.total = 0;
	pInner->head.type = CCLDATASTRUCTURE_TYPE_TREE;
	pInner->root = NULL;
	return pInner;
}

CCLAPI CCLDataStructure CCLCreateDynamicArray()
{
	PCCLDynamicArray pInner = malloc(sizeof(CCLDynamicArray));
	assert(pInner);
	pInner->head.total = 0;
	pInner->head.type = CCLDATASTRUCTURE_TYPE_DYN;
	pInner->arr = malloc(1);
	assert(pInner->arr);
	pInner->capacity = 1;
	return pInner;
}

CCLAPI const char* CCLCurrentErr_Datastructure()
{
	if (!CurrentErr)
		CurrentErr = ErrPool[NO_ERR];
	return CurrentErr;
}

CCLAPI CCLUINT64 CCLDataStructureSize(CCLDataStructure structure)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		return 0;
	}
	CCLDataStructurePublic* header = structure;
	return header->total;
}

CCLAPI CCLBOOL CCLLinearTablePut(CCLDataStructure structure, void* data, CCLINT64 offset)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLLinearTable pInner = structure;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_LINEAR)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	if (pInner->hook)
	{
		if (offset < 0)
			offset = -((-offset) % pInner->head.total);
		else
			offset = offset % pInner->head.total;
		PCCLLinearTableNode node = NULL;
		while (!node) node = malloc(sizeof(CCLLinearTableNode));
		node->data = data;
		if (offset < 0)
		{
			node->prior = pInner->hook->prior;
			while (offset < 0)
			{
				offset++;
				node->prior = node->prior->prior;
			}
			node->after = node->prior->after;
			node->prior->after = node;
			node->after->prior = node;
		}
		else
		{
			node->after = pInner->hook;
			while (offset > 0)
			{
				offset--;
				node->after = node->after->after;
			}
			node->prior = node->after->prior;
			node->prior->after = node;
			node->after->prior = node;
		}
	}
	else
	{
		while (!pInner->hook) pInner->hook = malloc(sizeof(CCLLinearTableNode));
		pInner->hook->after = pInner->hook;
		pInner->hook->prior = pInner->hook;
		pInner->hook->data = data;
	}
	pInner->head.total += 1;
	return CCLTRUE;
Failed:
	return CCLFALSE;
}

CCLAPI void* CCLLinearTableGet(CCLDataStructure structure, CCLINT64 offset)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLLinearTable pInner = structure;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_LINEAR)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	if (pInner->hook)
	{
		if (offset < 0)
			offset = -((-offset) % pInner->head.total);
		else
			offset = offset % pInner->head.total;
		PCCLLinearTableNode node = pInner->hook;
		if (offset > 0)
		{
			while (offset > 0)
			{
				offset--;
				node = node->after;
			}
		}
		else
		{
			while (offset < 0)
			{
				offset++;
				node = node->prior;
			}
		}
		void* data = node->data;
		if (pInner->head.total <= 1)
		{
			pInner->hook = NULL;
			free(node);
		}
		else
		{
			if (node == pInner->hook)
				pInner->hook = node->after;
			node->prior->after = node->after;
			node->after->prior = node->prior;
			free(node);
		}
		pInner->head.total -= 1;
		return data;
	}
	CurrentErr = ErrPool[NOTFOND_ERR];
Failed:
	return NULL;
}

CCLAPI void* CCLLinearTableSeek(CCLDataStructure structure, CCLINT64 offset)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLLinearTable pInner = structure;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_LINEAR)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	if (pInner->hook)
	{
		if (offset < 0)
			offset = -((-offset) % pInner->head.total);
		else
			offset = offset % pInner->head.total;
		PCCLLinearTableNode node = pInner->hook;
		if (offset > 0)
		{
			while (offset > 0)
			{
				offset--;
				node = node->after;
			}
		}
		else
		{
			while (offset < 0)
			{
				offset++;
				node = node->prior;
			}
		}
		return node->data;
	}
	CurrentErr = ErrPool[NOTFOND_ERR];
Failed:
	return NULL;
}

CCLAPI CCLBOOL CCLLinearTablePush(CCLDataStructure structure, void* data)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLLinearTable pInner = structure;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_LINEAR)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	PCCLLinearTableNode node = NULL;
	while (!node) node = malloc(sizeof(CCLLinearTableNode));
	node->data = data;
	if (pInner->hook)
	{
		node->after = pInner->hook;
		node->prior = pInner->hook->prior;
		node->prior->after = node;
		pInner->hook->prior = node;
		pInner->hook = node;
	}
	else
	{
		node->prior = node;
		node->after = node;
		pInner->hook = node;
	}
	pInner->head.total += 1;
Failed:
	return CCLFALSE;
}

CCLAPI void* CCLLinearTablePop(CCLDataStructure structure)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLLinearTable pInner = structure;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_LINEAR)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	if (pInner->hook)
	{
		if (pInner->head.total <= 1)
		{
			void* data = pInner->hook->data;
			free(pInner->hook);
			pInner->hook = NULL;
			pInner->head.total = 0;
			return data;
		}
		else
		{
			PCCLLinearTableNode node = pInner->hook;
			void* data = node->data;
			pInner->hook = node->after;
			node->prior->after = node->after;
			node->after->prior = node->prior;
			free(node);
			pInner->head.total -= 1;
			return data;
		}
	}
	CurrentErr = ErrPool[NOTFOND_ERR];
Failed:
	return NULL;
}

CCLAPI CCLBOOL CCLLinearTableIn(CCLDataStructure structure, void* data)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLLinearTable pInner = structure;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_LINEAR)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	PCCLLinearTableNode node = NULL;
	while (!node) node = malloc(sizeof(CCLLinearTableNode));
	node->data = data;
	if (pInner->hook)
	{
		node->after = pInner->hook;
		node->prior = pInner->hook->prior;
		node->prior->after = node;
		pInner->hook->prior = node;
		pInner->hook = node;
	}
	else
	{
		node->prior = node;
		node->after = node;
		pInner->hook = node;
	}
	pInner->head.total += 1;
Failed:
	return CCLFALSE;
}

CCLAPI void* CCLLinearTableOut(CCLDataStructure structure)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLLinearTable pInner = structure;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_LINEAR)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	if (pInner->hook)
	{
		if (pInner->head.total <= 1)
		{
			void* data = pInner->hook->data;
			free(pInner->hook);
			pInner->hook = NULL;
			pInner->head.total = 0;
			return data;
		}
		else
		{
			PCCLLinearTableNode node = pInner->hook->prior;
			void* data = node->data;
			node->prior->after = pInner->hook;
			pInner->hook->prior = node->prior;
			free(node);
			pInner->head.total -= 1;
			return data;
		}
	}
	CurrentErr = ErrPool[NOTFOND_ERR];
Failed:
	return NULL;
}

void destroy_lineartable(PCCLLinearTable table, CCLClearCallback callback)
{
	if (table->hook)
	{
		PCCLLinearTableNode p = table->hook, q = NULL;
		p->prior->after = NULL;
		if (!callback)
		{
			while (p->after)
			{
				table->head.total -= 1;
				q = p;
				p = p->after;
				free(q);
			}
			free(p);
		}
		else
		{
			while (p->after)
			{
				table->head.total -= 1;
				q = p;
				p = p->after;
				callback(q->data);
				free(q);
			}
			callback(p->data);
			free(p);
		}
	}
	table->head.total = 0;
	table->hook = NULL;
}

void* clear_node(PCCLTreeMapNode node, CCLClearCallback callback)
{

	if (node->left)
	{
		clear_node(node->left, callback);
		free(node->left);
	}
	if (node->right)
	{
		clear_node(node->right, callback);
		free(node->right);
	}
	if (callback)
		callback(node->data);
}

void destroy_tree(PCCLTreeMap tree, CCLClearCallback callback)
{
	if (tree->root)
		clear_node(tree->root, callback);
}

void destroy_dyn(PCCLDynamicArray dyn, CCLClearCallback callback)
{
	if (!dyn->arr)
		return;
	if (!callback)
	{
		free(dyn->arr);
	}
	else
	{
		while (dyn->head.total)
		{
			dyn->head.total--;
			callback(&(dyn->arr[dyn->head.total]));
		}
		free(dyn->arr);
	}
}

CCLAPI CCLBOOL CCLDestroyDataStructure(PCCLDataStructure pStructure, CCLClearCallback callback)
{
	if (!pStructure || !*pStructure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	switch (((CCLDataStructurePublic*)(*pStructure))->type)
	{
	case CCLDATASTRUCTURE_TYPE_LINEAR:
		destroy_lineartable(*pStructure, callback);
		break;
	case CCLDATASTRUCTURE_TYPE_TREE:
		destroy_tree(*pStructure, callback);
		break;
	case CCLDATASTRUCTURE_TYPE_DYN:
		destroy_dyn(*pStructure, callback);
		break;
	default:
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	free(*pStructure);
	*pStructure = NULL;
	return CCLTRUE;
Failed:
	return CCLFALSE;
}

void enum_linear(CCLLinearTable* table, CCLEnumCallback callback, void* user)
{
	if (!table->hook)
		return;
	PCCLLinearTableNode pNode = table->hook;
	for (int i = 0; i < table->head.total; i++)
	{
		callback(pNode->data, user, i);
		pNode = pNode->after;
	}
}

void enum_node(CCLTreeMapNode* node, CCLEnumCallback callback, void* user)
{
	if (node->left)
		enum_node(node->left, callback, user);
	callback(node->data, user, node->id);
	if (node->right)
		enum_node(node->right, callback, user);
}

void enum_tree(CCLTreeMap* tree, CCLEnumCallback callback, void* user)
{
	if (tree->root)
		enum_node(tree->root, callback, user);
}

void enum_dyn(CCLDynamicArray* dyn, CCLEnumCallback callback, void* user)
{
	if (!dyn->arr)
		return;
	for (int i = 0; i < dyn->head.total; i++)
	{
		callback(&(dyn->arr[i]), user, i);
	}
}

CCLAPI CCLBOOL CCLDataStructureEnum(CCLDataStructure structure, CCLEnumCallback callback, void* user)
{
	if (!structure)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	if (!callback)
		goto End;
	
	switch (((CCLDataStructurePublic*)structure)->type)
	{
	case CCLDATASTRUCTURE_TYPE_LINEAR:
		enum_linear(structure, callback, user);
		break;
	case CCLDATASTRUCTURE_TYPE_TREE:
		enum_tree(structure, callback, user);
		break;
	case CCLDATASTRUCTURE_TYPE_DYN:
		enum_dyn(structure, callback, user);
		break;
	default:
		break;
	}
End:
	return CCLTRUE;
Failed:
	return CCLFALSE;
}

//====================---------------

PCCLTreeMapNode create_tree_node(CCLINT64 id, void* data, PCCLTreeMapNode parent)
{
	PCCLTreeMapNode node = NULL;
	while (!node) node = malloc(sizeof(CCLTreeMapNode));
	node->parent = parent;
	node->left = NULL;
	node->right = NULL;
	node->data = data;
	node->id = id;
	node->red = CCLTRUE;
	return node;
}

CCLBOOL if_left(PCCLTreeMapNode node)
{
	if (node->parent)
	{
		if (node->parent->left == node)
			return CCLTRUE;
	}
	return CCLFALSE;
}

PCCLTreeMapNode get_brother(PCCLTreeMapNode node)
{
	if (if_left(node))
	{
		return node->parent->right;
	}
	else
	{
		if (node->parent)
			return node->parent->left;
	}
	return NULL;
}

void left_rotate(PCCLTreeMapNode node)
{
	if (node->right)
	{
		PCCLTreeMapNode top = node, right = node->right;
		PCCLTreeMapNode child = right->left;
		if (if_left(node))
			node->parent->left = right;
		else if (node->parent)
			node->parent->right = right;
		right->parent = node->parent;
		right->left = top;
		top->right = child;
		top->parent = right;
		if (child)
			child->parent = top;
	}
}

void right_rotate(PCCLTreeMapNode node)
{
	if (node->left)
	{
		PCCLTreeMapNode top = node, left = node->left;
		PCCLTreeMapNode child = left->right;
		if (if_left(node))
			node->parent->left = left;
		else if (node->parent)
			node->parent->right = left;
		left->parent = top->parent;
		left->right = top;
		top->left = child;
		top->parent = left;
		if (child)
			child->parent = top;
	}
}

CCLAPI CCLBOOL CCLTreeMapPut(CCLDataStructure tree, void* data, CCLINT64 id)
{
	if (!tree)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLTreeMap pInner = tree;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_TREE)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	if (!pInner->root)
	{
		pInner->root = create_tree_node(id, data, NULL);
		pInner->root->red = CCLFALSE;
	}
	else
	{
		PCCLTreeMapNode node = pInner->root;
	ContinueFind:
		if (node->id == id)
		{
			node->data = data;
			return CCLTRUE;
		}
		if (node->id > id)
		{
			if (node->left)
				node = node->left;
			else
			{
				node->left = create_tree_node(id, data, node);
				node = node->left;
				goto FindOver;
			}
		}
		else
		{
			if (node->right)
				node = node->right;
			else
			{
				node->right = create_tree_node(id, data, node);
				node = node->right;
				goto FindOver;
			}
		}
		goto ContinueFind;
	FindOver:
		if (node->parent->red)
		{
			PCCLTreeMapNode parentbrother = get_brother(node->parent);
			if (parentbrother)
			{
				if (parentbrother->red)
				{
					node->parent->red = CCLFALSE;
					parentbrother->red = CCLFALSE;
					if (node->parent->parent != pInner->root)
					{
						node->parent->parent->red = CCLTRUE;
						node = node->parent->parent;
						goto FindOver;
					}
					goto End;
				}
			}
			if (if_left(node->parent))
			{
				if (if_left(node))
				{
					node->parent->red = CCLFALSE;
					node->parent->parent->red = CCLTRUE;
					if (node->parent->parent == pInner->root)
						pInner->root = node->parent;
					right_rotate(node->parent->parent);
				}
				else
				{
					node->red = CCLFALSE;
					node->parent->parent->red = CCLTRUE;
					if (node->parent->parent == pInner->root)
						pInner->root = node;
					left_rotate(node->parent);
					right_rotate(node->parent);
				}
			}
			else
			{
				if (if_left(node))
				{
					node->red = CCLFALSE;
					node->parent->parent->red = CCLTRUE;
					if (node->parent->parent == pInner->root)
						pInner->root = node;
					right_rotate(node->parent);
					left_rotate(node->parent);
				}
				else
				{
					node->parent->red = CCLFALSE;
					node->parent->parent->red = CCLTRUE;
					if (node->parent->parent == pInner->root)
						pInner->root = node->parent;
					left_rotate(node->parent->parent);
				}
			}
		}
	}
End:
	pInner->head.total += 1;
	return CCLTRUE;
Failed:
	return CCLFALSE;
}

CCLAPI void* CCLTreeMapSeek(CCLDataStructure tree, CCLINT64 id)
{
	if (!tree)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLTreeMap pInner = tree;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_TREE)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	if (pInner->root)
	{
		PCCLTreeMapNode node = pInner->root;
	ContinueFind:
		if (node->id == id)
		{
			return node->data;
		}
		else if (node->id > id)
		{
			if (node->left)
			{
				node = node->left;
				goto ContinueFind;
			}
		}
		else
		{
			if (node->right)
			{
				node = node->right;
				goto ContinueFind;
			}
		}
	}
	CurrentErr = ErrPool[NOTFOND_ERR];
Failed:
	return NULL;
}

PCCLTreeMapNode get_pre(PCCLTreeMapNode node)
{
	node = node->left;
	while (node->right) node = node->right;
	return node;
}

void copy_node(PCCLTreeMapNode node, PCCLTreeMapNode pre)
{
	node->id = pre->id;
	node->data = pre->data;
}

void fix_more(PCCLTreeMap tree, PCCLTreeMapNode node)
{
ContinueFix:
	if (node->parent->parent)
	{
		PCCLTreeMapNode brother = get_brother(node);
		brother->red = CCLTRUE;
		if (node->parent->red)
			node->parent->red = CCLFALSE;
		else
		{
			node = node->parent;
			goto ContinueFix;
		}
	}
	else
	{
		PCCLTreeMapNode brother = get_brother(node);
		tree->root = brother;
		if (if_left(node))
		{
			if (brother->red)
			{
				node->parent->red = CCLTRUE;
				brother->red = CCLFALSE;
				left_rotate(node->parent);
			}
			else if (brother->right->red)
			{
				brother->right->red = CCLFALSE;
				left_rotate(node->parent);
			}
			else if (brother->left->red)
			{
				tree->root = brother->left;
				brother->left->red = CCLFALSE;
				right_rotate(brother);
				left_rotate(node->parent);
			}
			else
			{
				tree->root = node->parent;
				brother->red = CCLTRUE;
			}
		}
		else
		{
			if (brother->red)
			{
				node->parent->red = CCLTRUE;
				brother->red = CCLFALSE;
				right_rotate(node->parent);
			}
			else if (brother->left->red)
			{
				brother->left->red = CCLFALSE;
				right_rotate(node->parent);
			}
			else if (brother->right->red)
			{
				tree->root = brother->right;
				brother->right->red = CCLFALSE;
				left_rotate(brother);
				right_rotate(node->parent);
			}
			else
			{
				tree->root = node->parent;
				brother->red = CCLTRUE;
			}
		}
	}
}

CCLAPI void* CCLTreeMapGet(CCLDataStructure tree, CCLINT64 id)
{
	if (!tree)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}
	PCCLTreeMap pInner = tree;
	if (pInner->head.type != CCLDATASTRUCTURE_TYPE_TREE)
	{
		CurrentErr = ErrPool[WRONG_STRUCTURE];
		goto Failed;
	}
	PCCLTreeMapNode node = pInner->root;
	void* data;
	if (node)
	{
	ContinueFind:
		if (node->id == id)
		{
			goto FindOver;
		}
		else if (node->id > id)
		{
			if (node->left)
			{
				node = node->left;
				goto ContinueFind;
			}
		}
		else if (node->id < id)
		{
			if (node->right)
			{
				node = node->right;
				goto ContinueFind;
			}
		}
	}
	CurrentErr = ErrPool[NOTFOND_ERR];
	goto Failed;
FindOver:
	data = node->data;
	if (node->red)
	{
		if (if_left(node))
			node->parent->left = NULL;
		else
			node->parent->right = NULL;
		free(node);
	}
	else
	{
		if (node->left)
		{
			if (node->right)
			{
				PCCLTreeMapNode pre = get_pre(node);
				copy_node(node, pre);
				node = pre;
				goto FindOver;
			}
			else
			{
				copy_node(node, node->left);
				free(node->left);
				node->left = NULL;
			}
		}
		else if (node->right)
		{
			copy_node(node, node->right);
			free(node->right);
			node->right = NULL;
		}
		else
		{
			if (node != pInner->root)
			{
				PCCLTreeMapNode brother = get_brother(node);
				if (if_left(node))
				{
					if (brother->red)
					{
						if (node->parent == pInner->root)
							pInner->root = brother;
						brother->red = CCLFALSE;
						node->parent->red = CCLTRUE;
						left_rotate(node->parent);
						brother = node->parent->right;
					}
					if (brother->right)
					{
						if (node->parent == pInner->root)
							pInner->root = brother;
						brother->red = node->parent->red;
						node->parent->red = CCLFALSE;
						brother->right->red = CCLFALSE;
						left_rotate(node->parent);
						node->parent->left = NULL;
						free(node);
					}
					else if (brother->left)
					{
						if (node->parent == pInner->root)
							pInner->root = brother->left;
						node->parent->red = CCLFALSE;
						brother->left->red = node->parent->red;
						right_rotate(brother);
						left_rotate(brother->parent->parent);
						node->parent->left = NULL;
						free(node);
					}
					else
					{
						brother->red = CCLTRUE;
						node->parent->left = NULL;
						free(node);
						if (brother->parent->red)
							brother->parent->red = CCLFALSE;
						else
							if (brother->parent->parent)
								fix_more(pInner, brother->parent);
					}
				}
				else
				{
					if (brother->red)
					{
						if (node->parent == pInner->root)
							pInner->root = brother;
						brother->red = CCLFALSE;
						node->parent->red = CCLTRUE;
						right_rotate(node->parent);
						brother = node->parent->left;
					}
					if (brother->left)
					{
						if (node->parent == pInner->root)
							pInner->root = brother;
						brother->red = node->parent->red;
						node->parent->red = CCLFALSE;
						brother->left->red = CCLFALSE;
						right_rotate(node->parent);
						node->parent->right = NULL;
						free(node);
					}
					else if (brother->right)
					{
						if (node->parent == pInner->root)
							pInner->root = brother->right;
						node->parent->red = CCLFALSE;
						brother->right->red = node->parent->red;
						left_rotate(brother);
						right_rotate(brother->parent->parent);
						node->parent->right = NULL;
						free(node);
					}
					else
					{
						brother->red = CCLTRUE;
						node->parent->right = NULL;
						free(node);
						if (brother->parent->red)
							brother->parent->red = CCLFALSE;
						else
							if (brother->parent->parent)
								fix_more(pInner, brother->parent);
					}
				}
			}
			else
			{
				pInner->root = NULL;
				free(node);
			}
		}
	}
	pInner->head.total -= 1;
	return data;
Failed:
	return NULL;
}
//---居然都没超过1000行---（划掉）
//坏了啦！现在超过1000行了

//====================--------------------

CCLUINT8* dyn_arr_sizeup(CCLUINT8* arr, CCLUINT64 size, CCLUINT64 capacity)
{
	CCLUINT8* tmp = calloc(capacity, 1);
	if (!tmp)
	{
		CurrentErr = ErrPool[OUTOF_MEMORY];
		return NULL;
	}
	memcpy(tmp, arr, size);
	free(arr);
	return tmp;
}

CCLAPI CCLBOOL CCLDynamicArrayPush(CCLDataStructure dyn, CCLUINT8 data)
{
	PCCLDynamicArray pInner = dyn;
	if (!pInner)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}

	if (pInner->head.total < pInner->capacity)
	{
		pInner->arr[pInner->head.total++] = data;
		return CCLTRUE;
	}
	//需要扩容
	if (pInner->capacity >= CCL_DYN_MAXSIZE)
	{
		CurrentErr = ErrPool[OUTOF_MEMORY];
		goto Failed;
	}
	//确定扩容后大小，不得超过容量限制
	if (pInner->capacity << 1 < CCL_DYN_MAXSIZE)
		pInner->capacity <<= 1;
	else
		pInner->capacity = CCL_DYN_MAXSIZE;

	CCLUINT8* tmp = dyn_arr_sizeup(pInner->arr, pInner->head.total, pInner->capacity);
	if (tmp)
		pInner->arr = tmp;
	else goto Failed;

	pInner->arr[pInner->head.total++] = data;
	return CCLTRUE;
Failed:
	return CCLFALSE;
}

CCLAPI CCLBOOL CCLDynamicArraySet(CCLDataStructure dyn, CCLUINT8 data, CCLUINT64 subscript)
{
	PCCLDynamicArray pInner = dyn;
	if (!pInner)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}

	if (subscript >= 0 && subscript < pInner->head.total)
		pInner->arr[subscript] = data;
	else if (subscript == pInner->head.total)
		pInner->arr[pInner->head.total++] = data;
	else if (subscript > pInner->head.total && subscript < pInner->capacity)
	{
		pInner->arr[subscript++] = data;
		pInner->head.total = subscript;
	}
	else if (subscript < CCL_DYN_MAXSIZE)
	{
		while (pInner->capacity <= subscript)
		{
			pInner->capacity <<= 1;
		}
		CCLUINT8* tmp = dyn_arr_sizeup(pInner->arr, pInner->head.total, pInner->capacity);
		if (tmp)
			pInner->arr = tmp;
		else goto Failed;
		pInner->arr[subscript++] = data;
		pInner->head.total = subscript;
	}
	else
	{
		CurrentErr = ErrPool[OUTOF_MEMORY];
		goto Failed;
	}

	return CCLTRUE;
Failed:
	return CCLFALSE;
}

CCLUINT8* dyn_arr_sizedown(CCLUINT8* arr, CCLUINT64 capacity)
{
	CCLUINT8* tmp = calloc(capacity, 1);
	if (!tmp)
	{
		CurrentErr = ErrPool[OUTOF_MEMORY];
		return NULL;
	}
	memcpy(tmp, arr, capacity);
	free(arr);
	return tmp;
}

CCLAPI CCLUINT8 CCLDynamicArrayPop(CCLDataStructure dyn)
{
	PCCLDynamicArray pInner = dyn;
	if (!pInner)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		goto Failed;
	}

	if (pInner->head.total <= 0 || pInner->head.total > CCL_DYN_MAXSIZE)
	{
		CurrentErr = ErrPool[NOTFOND_ERR];
		goto Failed;
	}
	
	CCLUINT8 ret_val = pInner->arr[(pInner->head.total--) - 1];

	if (pInner->head.total >= pInner->capacity >> 1 || pInner->capacity <= 32)
		return ret_val;

	//可以释放一些闲置空间
	pInner->capacity >>= 1;
	CCLUINT8* tmp = dyn_arr_sizedown(pInner->arr, pInner->capacity);
	if (tmp)
		pInner->arr = tmp;
	else goto Failed;

	return ret_val;
Failed:
	return 0;
}

CCLAPI CCLUINT8 CCLDynamicArraySeek(CCLDataStructure dyn, CCLUINT64 subscript)
{
	PCCLDynamicArray pInner = dyn;
	if (!pInner)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		return 0;
	}

	if (subscript >= 0 && subscript < pInner->head.total)
		return pInner->arr[subscript];

	CurrentErr = ErrPool[NOTFOND_ERR];
	return 0;
}

CCLAPI CCLUINT64 CCLDynamicArrayCopy(CCLDataStructure dyn, CCLUINT8** pOut)
{
	PCCLDynamicArray pInner = dyn;
	if (!pInner)
	{
		CurrentErr = ErrPool[NULLPTR_ERR];
		return 0;
	}

	*pOut = NULL;
	if (!pInner->head.total)
		return 0;

	*pOut = malloc(pInner->head.total);
	memcpy(*pOut, pInner->arr, pInner->head.total);
	return pInner->head.total;
}
