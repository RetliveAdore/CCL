#include <CCL.h>
#include <cclinet.h>
#include <iostream>

CCLMODID cclstd, cclinet;
CCLModFunction inetclient, streaminet;

CCLUINT8 buffer[] = "123321";

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
	CCLModGetFn(cclinet, CCLSERV_INET_CLIENT, &req);
	if (req.func)
		inetclient = req.func;
	else return 1;
	CCLModGetFn(cclinet, CCLSERV_INET_STREAM, &req);
	if (req.func)
		streaminet = req.func;
	else return 1;

	CCL_CLIENT_REQ client_req;
	client_req.addr = "127.0.0.1";
	client_req.port = 2567;
	client_req.timeout = 5;
	inetclient(&client_req);
	if (!client_req.soc)
		std::cout << client_req.error << std::endl;
	else
		std::cout << "connected" << std::endl;

	std::cout << "buffer: " << buffer << std::endl;
	CCL_INETSTREAM_REQ stream;
	stream.serv = STREAM_OUT;
	stream.buffer = buffer;
	stream.buffersize = strlen((char*)buffer);
	stream.soc = client_req.soc;
	streaminet(&stream);
	if (stream.error)
		std::cout << stream.error << std::endl;
	else
		std::cout << "buffer: " << buffer << std::endl;
	_sleep(1000);
	stream.buffer = buffer;
	stream.buffersize = strlen((char*)buffer);
	streaminet(&stream);
	if (stream.error)
		std::cout << stream.error << std::endl;

	char t = getchar();

	return 0;
}