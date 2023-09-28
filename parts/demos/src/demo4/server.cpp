#include <CCL.h>
#include <cclinet.h>
#include <iostream>

CCLMODID cclstd, cclinet;
CCLModFunction inetserver, streaminet, closesoc, ifclean;

CCLBOOL run = CCLTRUE;

CCLBOOL compare(const char* str1, const char* str2)
{
	if (strlen(str1) != strlen(str2))
		return CCLFALSE;
	for (int i = 0; i < strlen(str1); i++)
	{
		if (str1[i] != str2[i])
			return CCLFALSE;
	}
	return CCLTRUE;
}

void process(CCL_SOCKET soc)
{
	std::cout << "connected" << std::endl;
	CCLUINT8 ch[10];
	CCL_INETSTREAM_REQ stream;
	stream.serv = STREAM_IN;
	stream.buffer = ch;
	stream.soc = soc;
	while (run)
	{
		stream.buffersize = 10;
		streaminet(&stream);
		if (stream.error)
		{
			std::cout << stream.error << std::endl;
			break;
		}
		if (!stream.effective) break;
		else if (stream.buffersize < 0) _sleep(1);
		else printf("%s\n", ch);
	}
	printf("disconnect\n");
}

int main(int argc, char** argv)
{
	char* path = nullptr;

	if (!CCLModuleLoaderInit())
	{
		std::cout << CCLCurrentErr_EXModule() << std::endl;
		return -1;
	}

	ccl_gluing_path(argv[0], "CCL_std", &path);
	std::cout << path << std::endl;
	cclstd = CCLEXLoad(path);
	if (!cclstd)
	{
		std::cout << CCLCurrentErr_EXModule() << std::endl;
		return 1;
	}
	delete path;

	ccl_gluing_path(argv[0], "CCL_inet", &path);
	std::cout << path << std::endl;
	cclinet = CCLEXLoad(path);
	if (!cclinet)
	{
		std::cout << CCLCurrentErr_EXModule() << std::endl;
		return 1;
	}

	CCL_MOD_REQUEST_STRUCTURE req;
	CCLModGetFn(cclinet, CCLSERV_INET_SERVER, &req);
	if (req.func)
		inetserver = req.func;
	else return 1;
	CCLModGetFn(cclinet, CCLSERV_INET_STREAM, &req);
	if (req.func)
		streaminet = req.func;
	else return 1;
	CCLModGetFn(cclinet, CCLSERV_INET_CLOSE, &req);
	if (req.func)
		closesoc = req.func;
	else return 1;
	CCLModGetFn(cclinet, CCLSERV_INET_IFCLEAN, &req);
	if (req.func)
		ifclean = req.func;
	else return 1;

	CCL_SERVER_REQ server_req;
	server_req.maxAccepts = 10;
	server_req.port = 2567;
	server_req.processFn = process;
	inetserver(&server_req);
	if (!server_req.soc)
		std::cout << server_req.error << std::endl;

	char cmd[32];
	char tmp;
	int subscript = 0;
	while (1)
	{
		subscript = 0;
		for (int i = 0; i < 32; i++) cmd[i] = '\0';
		while (subscript < 31 && (tmp = getchar()) != '\n')
		{
			cmd[subscript] = tmp;
			subscript++;
		}
		if (compare(cmd, "close"))
			break;
		else
			std::cout << "unknown command: " << cmd << std::endl;
	}

	run = CCLFALSE;
	//一定要记得释放
	closesoc((void*)server_req.soc);

	CCLBOOL clean = CCLFALSE;
	while (!clean)
	{
		ifclean(&clean);
		_sleep(10);
	}

	return 0;
}