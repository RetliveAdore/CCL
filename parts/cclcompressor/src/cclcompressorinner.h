#ifndef _INCLUDE_CCLCODERINNER_H_
#define _INCLUDE_CCLCODERINNER_H_

#include "../include/cclcompressor.h"
#include <stdio.h>

// （三元组）总共26比特
// +-----------+-------------------------+--------------+-----------------+
// | type( 1 ) | offset in window(12bit) | length(5bit) | next bits(8bit) |
// +-----------+-------------------------+--------------+-----------------+
// （普通字节）总共9比特
// +-----------+-----------------+
// | type( 0 ) | next bits(8bit) |
// +-----------+-----------------+
// 需要很多位操作

//1:三元组
//0:普通字符
#define LZ77_TYPE_BIT_SIZE   1   //类型，表明是普通字符还是三元组的标志位
#define LZ77_OFFSET_BIT_SIZE 12  //偏移量，在编码窗口中的偏移量
#define LZ77_LENGTH_BIT_SIZE 5   //长度，匹配到的最大长度
#define LZ77_NEXT_BIT_SIZE   8   //下一个普通字符的编码

#define LZ77_WINDOW_SIZE     4095  //滑动窗口的大小限制（1111_1111_1111B）
#define LZ77_BUFFER_SIZE     31    //超前缓冲区大小限制（1_1111B）

void informationentropy_ccl(void* req);
void compress_lz77_ccl(void* req);

#endif //include