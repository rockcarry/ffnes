/* 标准头文件 */
#ifndef _STDEFINE_H_
#define _STDEFINE_H_

#if defined(__MINGW32__)
#include <windows.h>
#undef RGB // undef RGB
#else

/* 常量定义 */
#define TRUE   1
#define FALSE  0

/* 标准的类型定义 */
typedef int BOOL;
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef long     int   LONG;

#endif

/* 该宏用于消除变量未使用的警告 */
#define DO_USE_VAR(var)  do { var = var; } while (0)

#endif


