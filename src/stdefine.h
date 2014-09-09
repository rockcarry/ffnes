/* 标准头文件 */
#ifndef _STDEFINE_H_
#define _STDEFINE_H_

#if defined(WIN32)
#include <windows.h>

#ifdef _MSC_VER
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4996)
#endif
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

#ifndef offsetof
#define offsetof(type, member)          ((size_t)&((type*)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member) (type*)((char*)(ptr) - offsetof(type, member))
#endif

/* 该宏用于消除变量未使用的警告 */
#define DO_USE_VAR(var)  do { var = var; } while (0)

#endif


