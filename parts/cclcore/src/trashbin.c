#include "inner.h"

#include <stdlib.h>

static CCLBOOL onInit = CCLFALSE;
static CCLDataStructure stack = NULL;

static void clear_callback(void* data)
{
	if (data)
		((TrashBinFunc)data)();
}

static void on_close()
{
	CCLDestroyDataStructure(&stack, clear_callback);
}

CCLAPI void CCLRejisterClearFunc(TrashBinFunc func)
{
	if (!onInit)
	{
		stack = CCLCreateLinearTable();
		atexit(on_close);
		onInit = CCLTRUE;
	}
	CCLLinearTablePush(stack, func);
}