#ifndef _NES_LZW_H_
#define _NES_LZW_H_

// 包含头文件
#include <stdio.h>
#include "stdefine.h"

// 函数声明
void* lzw_fopen (const char *filename, const char *mode);
int   lzw_fclose(void *stream);
int   lzw_fgetc (void *stream);
int   lzw_fputc (int c, void *stream);

#endif
