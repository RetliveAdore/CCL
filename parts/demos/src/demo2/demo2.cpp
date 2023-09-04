#include <CCL.h>
#include <cclgraphic.h>
#include <cclstd.h>
#include <string.h>
#include <stdio.h>

//���ⲿģ����ص�ȫ�ֱ���
CCLMODID cclstd, cclgraphic;

CCLModFunction
shortsleep = nullptr,
cclmsgbox = nullptr,
cclwindowserv = nullptr,
cclonquit = nullptr;

//gloabal variables
CCLBOOL onquit = CCLFALSE;
CCLWINDOW window1, window2;
CCL_WINDOW_REQ wreq;

//�ص�
CCLBOOL quitcallback(PCCL_MSG msg)
{
	printf("close\n");
	return CCLFALSE;
}

CCLBOOL mousecallback(PCCL_MSG msg)
{
	if (msg->status & CCL_S_LEFT)
		printf("left ");
	else if (msg->status & CCL_S_RIGHT)
		printf("right ");
	if (msg->status & CCL_S_DOWN)
	{
		printf("mouse button down: %d, %d\n", msg->x, msg->y);
	}
	else if (msg->status & CCL_S_UP)
	{
		printf("mouse button up: %d, %d\n", msg->x, msg->y);
	}
	return CCLTRUE;
}

CCLBOOL keycallback(PCCL_MSG msg)
{
	if (msg->status == CCL_S_OTHER)
	{
		printf("key: %c\n", msg->keycode);
		return CCLTRUE;
	}
	else if (msg->status == CCL_S_DOWN && msg->keycode == CCL_ESC)
	{
		wreq.servid = CCLWINDOW_DESTROY;
		wreq.id = msg->windowID;
		cclwindowserv(&wreq);
	}
	else if (msg->status == CCL_S_DOWN)
	{
		printf("keycode: %x\n", msg->keycode);
	}
	return CCLFALSE;
}

int main(int argc, char** argv)
{
	int ret = 0;
	char* path = nullptr;
	CCLUINT8* data = new CCLUINT8[100 * 100 * 4];

	//ʹ�ü�����ǰ�����ȳ�ʼ��
	if (!CCLModuleLoaderInit())
	{
		printf("%s\n", CCLCurrentErr_EXModule());
		ret = -1;
		goto End;
	}

	//�������
	ccl_gluing_path(argv[0], "CCL_std", &path);
	printf("%s\n", path);
	cclstd = CCLEXLoad(path);
	delete path;
	if (!cclstd)
	{
		printf("%s\n", CCLCurrentErr_EXModule());
		ret = 1;
		goto End;
	}
	ccl_gluing_path(argv[0], "CCL_graphic", &path);
	printf("%s\n", path);
	cclgraphic = CCLEXLoad(path);
	delete path;
	if (!cclgraphic)
	{
		printf("%s\n", CCLCurrentErr_EXModule());
		ret = 2;
		goto End;
	}

//��ʼ���������

	CCL_MOD_REQUEST_STRUCTURE req;
	CCLModGetFn(cclstd, CCLSERV_SHORT_SLEEP, &req);
	if (req.func)
		shortsleep = req.func;
	else goto Failed;
	CCLModGetFn(cclgraphic, CCLSERV_MSGBOX, &req);
	if (req.func)
		cclmsgbox = req.func;
	else goto Failed;
	CCLModGetFn(cclgraphic, CCLSERV_WINDOW, &req);
	if (req.func)
		cclwindowserv = req.func;
	else goto Failed;
	CCLModGetFn(cclgraphic, CCLSERV_ONQUIT, &req);
	if (req.func)
		cclonquit = req.func;
	else goto Failed;
	goto Done;
Failed:
	printf("%s\n", req.name);
Done:
	
	wreq.servid = CCLWINDOW_CREATE;
	wreq.title = "Demo2-1";
	wreq.w = 500;
	wreq.h = 500;
	wreq.x = 400;
	wreq.y = 400;
	cclwindowserv(&wreq);
	window1 = wreq.id;
	if (!window1)
		return -1;

	wreq.title = "Demo2-2";
	wreq.w = 500;
	wreq.h = 500;
	wreq.x = 950;
	wreq.y = 500;
	cclwindowserv(&wreq);
	window2 = wreq.id;
	if (!window2)
		return -1;

	wreq.servid = CCLWINDOW_CBK;
	wreq.cbk_id = CCL_QUIT_CB;
	wreq.cbk = quitcallback;
	wreq.id = window1;
	cclwindowserv(&wreq);
	wreq.id = window2;
	cclwindowserv(&wreq);
	/////////////////////
	wreq.cbk_id = CCL_MOUSE_CB;
	wreq.cbk = mousecallback;
	wreq.id = window1;
	cclwindowserv(&wreq);
	wreq.id = window2;
	cclwindowserv(&wreq);
	/////////////////////
	wreq.cbk_id = CCL_KEY_CB;
	wreq.cbk = keycallback;
	wreq.id = window1;
	cclwindowserv(&wreq);
	wreq.id = window2;
	cclwindowserv(&wreq);

	wreq.servid = CCLWINDOW_SIZELMT;
	wreq.w = 500;
	wreq.h = 500;
	wreq.x = 500;
	wreq.y = 500;
	wreq.status = CCLTRUE;
	cclwindowserv(&wreq);

	CCL2D_ITEM line1;
	line1.type = CCL_2D_ELIPSE;// | CCL_2D_FILLED;
	line1.lock = CCLFALSE;
	line1.level = 100;
	line1.color = { 0.75f, 0.2f, 0.55f, 1.0f };
	line1.cr = { 20.0f, 30.0f, 30.0f, 20.0f };
	line1.stroke = 3.0f;
	wreq.id = window1;
	wreq.servid = CCLWINDOW_ITEM;
	wreq.item = &line1;
	cclwindowserv(&wreq);

	CCL2D_ITEM line2;
	line2.type = CCL_2D_RECT;// | CCL_2D_FILLED;
	line2.lock = CCLFALSE;
	line2.level = 90;
	line2.color = { 0.3f, 0.2f, 0.9f, 1.0f };
	line2.pp = { 80.0f, 20.0f, 10.3f, 90.0f };
	line2.stroke = 3.0f;
	wreq.id = window2;
	wreq.servid = CCLWINDOW_ITEM;
	wreq.item = &line2;
	cclwindowserv(&wreq);

	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 100; j++)
		{
			data[(i * 100 + j) * 4] = 255.0 * ((float)i / 100.0);
			data[(i * 100 + j) * 4 + 1] = 255.0 * ((float)j / 100.0);
			data[(i * 100 + j) * 4 + 2] = 255 - (((float)i / 100.0) + ((float)j / 100.0)) / 2;
			data[(i * 100 + j) * 4 + 3] = ((i * 100 + j) % 2) * 255;
		}
	}
	CCL2D_ITEM bmi1;
	bmi1.type = CCL_2D_BITMAP;
	bmi1.lock = CCLFALSE;
	bmi1.level = 0;
	bmi1.pp = { 10.0f, 10.0f, 200.0f, 200.0f };
	bmi1.range = { 0, 0, 100, 100 };
	bmi1.color.a = 1.0f;
	bmi1.data = data;
	wreq.id = window2;
	wreq.item = &bmi1;
	cclwindowserv(&wreq);

	while (!onquit)
	{
		shortsleep((void*)1);
		cclonquit((void*)&onquit);
	}

	delete data;

End:
	printf("return value: %d\n", ret);
	// printf("press enter to exit...\n", ret);
	// getchar();
	return ret;
}