#ifndef _NES_LZW_H_
#define _NES_LZW_H_

// 包含头文件
#include <stdio.h>
#include "stdefine.h"

// 函数声明
void*  lzw_fopen (const char *filename, const char *mode);
int    lzw_fclose(void *stream);
int    lzw_fgetc (void *stream);
int    lzw_fputc (int c, void *stream);
size_t lzw_fread (void *buffer, size_t size, size_t count, void *stream);
size_t lzw_fwrite(void *buffer, size_t size, size_t count, void *stream);
int    lzw_fseek (void *stream, long offset, int origin);
long   lzw_ftell (void *stream);

#endif
