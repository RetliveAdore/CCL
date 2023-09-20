#include "cclcompressorinner.h"
#include <math.h>

//简单计算一下信息熵
double information_entropy(CCLUINT8* data, CCLUINT64 len)
{
	CCLUINT64 hash[256];
	double entropy = 0;
	for (int t = 0; t < 255; t++)
	{
		hash[t] = 0;
	}
	for (int i = 0; i < len; i++)
	{
		hash[data[i]]++;
	}
	for (int j = 0; j < 255; j++)
	{
		if (hash[j])
		{
			double portability = (double)hash[j] / (double)len;
			entropy -= portability * log(portability) * 1.442695;
			/*
			* 1.0 / log(2.0f) 约等于 1.4426950...
			*/
		}
	}
	return entropy;
}

/*
* Ultility Function: Get several bits` value in a buffer
*（实用函数，也叫工具函数）老外经常这么写
* 
* 直接精确地获取缓冲区中某几位的信息
* 最大获取十六位信息，以从高位到低位的顺序放入返回值中
*/

CCLUINT16 get_bits(const CCLUINT8* buffer, CCLUINT64 seek_pos, CCLUINT8 len)
{
	if (!len)
		return 0;
	if (len > 16)
		len = 16;
	CCLUINT16 ret_val = 0;
	buffer += seek_pos >> 3;
	//在该字节内偏移了多少
	CCLUINT8 offset = seek_pos % 8;
	for (; len > 0; len--)
	{
		ret_val |= (((*buffer & CCL_BIT_MASK_1 >> offset) >> (7 - offset)) << (len - 1));
		offset++;
		//寻址溢出到下一字节的时候
		if (offset > 7)
		{
			offset = 0;
			buffer++;
		}
	}
	return ret_val;
}

/*
* Ultility Function: Put several bits` value to a buffer
*
* 上面那个函数的逆过程，把位数据放进缓存里面
* 同样最大放入十六位信息，从传入变量的高位开始
*/

void put_bits(CCLUINT8* buffer, CCLUINT64 seek_pos, CCLUINT8 len, CCLUINT16 bits_buffer)
{
	if (!len)
		return;
	if (len > 16)
		len = 16;
	buffer += seek_pos >> 3;
	//在该字节内偏移了多少
	CCLUINT8 offset = seek_pos % 8;
	for (; len > 0; len--)
	{
		*buffer &= CCL_BIT_MASK_0 >> offset | CCL_BIT_COVER << (8 - offset);
		*buffer |= (bits_buffer >> (len - 1) & 1) << (7 - offset);
		offset++;
		//寻址溢出到下一字节的时候
		if (offset > 7)
		{
			offset = 0;
			buffer++;
		}
	}
}

//（什么奇形怪状的哈希表？理解不能，所以说还是传统的方法好）
//返回值不可能超过32，所以说只用一个字节返回即可
//offset最大十二位，用16位变量来装空间很富裕
CCLUINT8 compare_win(
	const CCLUINT8* win_begin, const CCLUINT8* win_end,
	const CCLUINT8* buf_begin, const CCLUINT8* buf_end,
	CCLUINT16* token
)
{
	if (win_begin > win_end || buf_begin > buf_end)
		return 0;
	CCLUINT16 token_tmp = win_end - win_begin;
	CCLUINT8 longest = 0, length = 0;
	const CCLUINT8* p_tmp_source = win_begin;
	const CCLUINT8* p_tmp_buffer = buf_begin;
	while (win_begin <= win_end)
	{
		while (p_tmp_buffer <= buf_end && p_tmp_source <= win_end)
		{
			if (*p_tmp_buffer != *p_tmp_source)
				break;
			length++;
			p_tmp_source++;
			p_tmp_buffer++;
		}
		//准备下一轮
		win_begin++;
		p_tmp_source = win_begin;
		p_tmp_buffer = buf_begin;
		if (length > longest)
		{
			longest = length;
			*token = token_tmp + 1;
		}
		token_tmp--;
		length = 0;
	}
	return longest;
}

//返回值是压缩后的数据量（字节数）
CCLUINT64 ccl_lz77_compress(CCLUINT8* in, CCLUINT64 in_size, CCLDataStructure* _dyn)
{
	CCLDataStructure dyn = CCLCreateDynamicArray();
	//初末指针所在的位置，刚刚好指向一头一尾两块边缘内存空间
	CCLUINT8 *begin = in;
	CCLUINT8 *end = in + in_size - 1;
	//滑动窗口的位置信息
	CCLUINT8* windowpos = in;
	//输出缓存的位偏移位置
	CCLUINT8 outbuffer_pos = 0;

	CCLUINT8 tmp[5] = { 0 };

	CCLUINT16 token = 0;
	CCLUINT8 length = 0;

	//压缩的第一个字符必然是直接编码的
	put_bits(tmp, outbuffer_pos, LZ77_TYPE_BIT_SIZE, 0);
	outbuffer_pos += LZ77_TYPE_BIT_SIZE;
	put_bits(tmp, outbuffer_pos, LZ77_NEXT_BIT_SIZE, *windowpos);
	outbuffer_pos += LZ77_NEXT_BIT_SIZE;

	//循环里面的就是重点了
	while (windowpos < end)
	{
		length = compare_win(
			(windowpos - begin) > LZ77_WINDOW_SIZE ? windowpos - LZ77_WINDOW_SIZE : begin,
			windowpos,
			windowpos + 1,
			(end - windowpos) > LZ77_BUFFER_SIZE ? windowpos + LZ77_BUFFER_SIZE : end,
			&token
		);
		windowpos++;
		if (length > 1)
		{
			windowpos += length;
			put_bits(tmp, outbuffer_pos, LZ77_TYPE_BIT_SIZE, 1);
			outbuffer_pos += LZ77_TYPE_BIT_SIZE;
			put_bits(tmp, outbuffer_pos, LZ77_OFFSET_BIT_SIZE, token);
			outbuffer_pos += LZ77_OFFSET_BIT_SIZE;
			put_bits(tmp, outbuffer_pos, LZ77_LENGTH_BIT_SIZE, length);
			outbuffer_pos += LZ77_LENGTH_BIT_SIZE;
			if (windowpos <= end)
				put_bits(tmp, outbuffer_pos, LZ77_NEXT_BIT_SIZE, *windowpos);
			else
				put_bits(tmp, outbuffer_pos, LZ77_NEXT_BIT_SIZE, 0);

			outbuffer_pos += LZ77_NEXT_BIT_SIZE;
		}
		else
		{
			put_bits(tmp, outbuffer_pos, LZ77_TYPE_BIT_SIZE, 0);
			outbuffer_pos += LZ77_TYPE_BIT_SIZE;
			put_bits(tmp, outbuffer_pos, LZ77_NEXT_BIT_SIZE, *windowpos);
			outbuffer_pos += LZ77_NEXT_BIT_SIZE;
		}
		//不需要什么循环，就这么几种情况直接枚举
		if (outbuffer_pos > 31)
		{
			outbuffer_pos -= 32;
			CCLDynamicArrayPush(dyn, tmp[0]);
			CCLDynamicArrayPush(dyn, tmp[1]);
			CCLDynamicArrayPush(dyn, tmp[2]);
			CCLDynamicArrayPush(dyn, tmp[3]);
			tmp[0] = tmp[4];
		}
		else if (outbuffer_pos > 23)
		{
			outbuffer_pos -= 24;
			CCLDynamicArrayPush(dyn, tmp[0]);
			CCLDynamicArrayPush(dyn, tmp[1]);
			CCLDynamicArrayPush(dyn, tmp[2]);
			tmp[0] = tmp[3];
		}
		else if (outbuffer_pos > 15)
		{
			outbuffer_pos -= 16;
			CCLDynamicArrayPush(dyn, tmp[0]);
			CCLDynamicArrayPush(dyn, tmp[1]);
			tmp[0] = tmp[2];
		}
		else if (outbuffer_pos > 7)
		{
			outbuffer_pos -= 8;
			CCLDynamicArrayPush(dyn, tmp[0]);
			tmp[0] = tmp[1];
		}
	}
	if (outbuffer_pos)
	{
		CCLDynamicArrayPush(dyn, tmp[0]);
	}
	*_dyn = dyn;
	return CCLDataStructureSize(dyn);
}

//边角料是免不了的，毕竟是亚字节操作，但是解压能够保证完全复原原本的文件，多的部分丢掉就好
CCLUINT64 ccl_lz77_decompress(CCLUINT8* in, CCLUINT64 in_size, CCLDataStructure* _dyn)
{
	CCLDataStructure dyn = CCLCreateDynamicArray();

	CCLUINT64 seek_pos = 0;
	CCLUINT8 buffer_token = 0;

	CCLUINT16 len = 0, token = 0;
	CCLUINT64 window_pos = 0;

	while (seek_pos >> 3 < in_size)
	{
		if (get_bits(in, seek_pos, LZ77_TYPE_BIT_SIZE))
		{
			seek_pos += LZ77_TYPE_BIT_SIZE;
			token = get_bits(in, seek_pos, LZ77_OFFSET_BIT_SIZE);
			seek_pos += LZ77_OFFSET_BIT_SIZE;
			len = get_bits(in, seek_pos, LZ77_LENGTH_BIT_SIZE);
			seek_pos += LZ77_LENGTH_BIT_SIZE;
			window_pos = CCLDataStructureSize(dyn) - token;
			while (len)
			{
				CCLDynamicArrayPush(dyn, CCLDynamicArraySeek(dyn, window_pos));
				len--;
				window_pos++;
			}
			CCLDynamicArrayPush(dyn, get_bits(in, seek_pos, LZ77_NEXT_BIT_SIZE));
		}
		else
		{
			seek_pos += LZ77_TYPE_BIT_SIZE;
			CCLDynamicArrayPush(dyn, get_bits(in, seek_pos, LZ77_NEXT_BIT_SIZE));
		}
		seek_pos += LZ77_NEXT_BIT_SIZE;
	}
	*_dyn = dyn;
	return CCLDataStructureSize(dyn);
}

void informationentropy_ccl(void* req)
{
	PCCL_INFENTROPY_REQ entropy = req;
	if (!entropy)
		return;
	entropy->entropy = information_entropy(entropy->data, entropy->size);
}

void compress_lz77_ccl(void* req)
{
	PCCL_LZ77SERV_REQ lz77 = req;
	if (!lz77)
		return;
	if (lz77->serv == CCL_CLEARDYN)
	{
		CCLDestroyDataStructure(&(lz77->Dyn), NULL);
		return;
	}
	if (!lz77->Src || !lz77->in_size)
		return;
	if (lz77->serv == CCL_DECOMPRESS)
		lz77->out_size = ccl_lz77_decompress(lz77->Src, lz77->in_size, &(lz77->Dyn));
	else if (lz77->serv == CCL_COMPRESS)
		lz77->out_size = ccl_lz77_compress(lz77->Src, lz77->in_size, &(lz77->Dyn));
}
