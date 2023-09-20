#include <CCL.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <cclcompressor.h>

CCLUINT8 buffer0[] = { 0xff, 0xff };
const char* buffer1 = "abaabsbsbabasabsbs";
const char* buffer2 = "aaaaaaaaaaaaaaaaaa";
const char* buffer3 = "asdfhgilaustdfhwliyfagsldfgaldsfwuefjlashdgajshgkdfjgwieualsdfbvgaislexawh,gdfila";
const char* buffer4 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const char* buffer5 = "qowiuwuieuwiueiweuieuwieuiwquowiueiquieoqwieuoqiweuoquweoiqwueoqwieuoqwieuoiqwueo";
const char* buffer6 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

CCLMODID compressor;
CCLModFunction entropy = nullptr, lz77_serv = nullptr;

int main(int argc, char** argv)
{
	char* path = nullptr;

	if (!CCLModuleLoaderInit())
	{
		std::cout << CCLCurrentErr_EXModule() << std::endl;
		return -1;
	}

	ccl_gluing_path(argv[0], "CCL_Compressor", &path);
	std::cout << path << std::endl;

	compressor = CCLEXLoad(path);
	if (!compressor)
	{
		std::cout << CCLCurrentErr_EXModule() << std::endl;
		return 1;
	}
	CCL_MOD_REQUEST_STRUCTURE req;
	CCLModGetFn(compressor, CCLSERV_INFENTROPY, &req);
	if (req.func)
		entropy = req.func;
	else return 2;
	CCLModGetFn(compressor, CCLSERV_LZ77, &req);
	if (req.func)
		lz77_serv = req.func;
	else return 2;

	CCL_INFENTROPY_REQ infent_req;

	const char* buf = buffer6;
	//压缩
	CCL_LZ77SERV_REQ lz77_req;
	lz77_req.serv = CCL_COMPRESS;
	lz77_req.Src = (CCLUINT8*)buf;
	lz77_req.in_size = strlen(buf);
	lz77_serv(&lz77_req);
	
	CCLUINT8* compressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &compressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	//解压缩
	lz77_req.serv = CCL_DECOMPRESS;
	lz77_req.Src = compressed;
	lz77_req.in_size = lz77_req.out_size;
	lz77_serv(&lz77_req);

	CCLUINT8* decompressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &decompressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	std::cout << "source      : " << buf;
	infent_req.data = (CCLUINT8*)buf;
	infent_req.size = strlen(buf);
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;
	std::cout << "decompressed: " << decompressed;
	infent_req.data = compressed;
	infent_req.size = lz77_req.out_size;
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;

	delete compressed;
	delete decompressed;

	//==============================================================
	buf = buffer5;
	//压缩
	lz77_req.serv = CCL_COMPRESS;
	lz77_req.Src = (CCLUINT8*)buf;
	lz77_req.in_size = strlen(buf);
	lz77_serv(&lz77_req);

	compressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &compressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	//解压缩
	lz77_req.serv = CCL_DECOMPRESS;
	lz77_req.Src = compressed;
	lz77_req.in_size = lz77_req.out_size;
	lz77_serv(&lz77_req);

	decompressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &decompressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	std::cout << "source      : " << buf;
	infent_req.data = (CCLUINT8*)buf;
	infent_req.size = strlen(buf);
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;
	std::cout << "decompressed: " << decompressed;
	infent_req.data = compressed;
	infent_req.size = lz77_req.out_size;
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;

	delete compressed;
	delete decompressed;
	//==============================================================
	buf = buffer4;
	//压缩
	lz77_req.serv = CCL_COMPRESS;
	lz77_req.Src = (CCLUINT8*)buf;
	lz77_req.in_size = strlen(buf);
	lz77_serv(&lz77_req);

	compressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &compressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	//解压缩
	lz77_req.serv = CCL_DECOMPRESS;
	lz77_req.Src = compressed;
	lz77_req.in_size = lz77_req.out_size;
	lz77_serv(&lz77_req);

	decompressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &decompressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	std::cout << "source      : " << buf;
	infent_req.data = (CCLUINT8*)buf;
	infent_req.size = strlen(buf);
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;
	std::cout << "decompressed: " << decompressed;
	infent_req.data = compressed;
	infent_req.size = lz77_req.out_size;
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;

	delete compressed;
	delete decompressed;
	//==============================================================
	buf = buffer3;
	//压缩
	lz77_req.serv = CCL_COMPRESS;
	lz77_req.Src = (CCLUINT8*)buf;
	lz77_req.in_size = strlen(buf);
	lz77_serv(&lz77_req);

	compressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &compressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	//解压缩
	lz77_req.serv = CCL_DECOMPRESS;
	lz77_req.Src = compressed;
	lz77_req.in_size = lz77_req.out_size;
	lz77_serv(&lz77_req);

	decompressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &decompressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	std::cout << "source      : " << buf;
	infent_req.data = (CCLUINT8*)buf;
	infent_req.size = strlen(buf);
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;
	std::cout << "decompressed: " << decompressed;
	infent_req.data = compressed;
	infent_req.size = lz77_req.out_size;
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;

	delete compressed;
	delete decompressed;
	//==============================================================
	buf = buffer2;
	//压缩
	lz77_req.serv = CCL_COMPRESS;
	lz77_req.Src = (CCLUINT8*)buf;
	lz77_req.in_size = strlen(buf);
	lz77_serv(&lz77_req);

	compressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &compressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	//解压缩
	lz77_req.serv = CCL_DECOMPRESS;
	lz77_req.Src = compressed;
	lz77_req.in_size = lz77_req.out_size;
	lz77_serv(&lz77_req);

	decompressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &decompressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	std::cout << "source      : " << buf;
	infent_req.data = (CCLUINT8*)buf;
	infent_req.size = strlen(buf);
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;
	std::cout << "decompressed: " << decompressed;
	infent_req.data = compressed;
	infent_req.size = lz77_req.out_size;
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;

	delete compressed;
	delete decompressed;
	//==============================================================
	buf = buffer1;
	//压缩
	lz77_req.serv = CCL_COMPRESS;
	lz77_req.Src = (CCLUINT8*)buf;
	lz77_req.in_size = strlen(buf);
	lz77_serv(&lz77_req);

	compressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &compressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	//解压缩
	lz77_req.serv = CCL_DECOMPRESS;
	lz77_req.Src = compressed;
	lz77_req.in_size = lz77_req.out_size;
	lz77_serv(&lz77_req);

	decompressed = nullptr;
	CCLDynamicArrayCopy(lz77_req.Dyn, &decompressed);
	lz77_req.serv = CCL_CLEARDYN;
	lz77_serv(&lz77_req);

	std::cout << "source      : " << buf;
	infent_req.data = (CCLUINT8*)buf;
	infent_req.size = strlen(buf);
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;
	std::cout << "decompressed: " << decompressed;
	infent_req.data = compressed;
	infent_req.size = lz77_req.out_size;
	entropy(&infent_req);
	std::cout << "\t" << infent_req.entropy << std::endl;

	delete compressed;
	delete decompressed;

	return 0;
}