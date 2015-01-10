#ifndef _NES_LOG_H_
#define _NES_LOG_H_

// 包含头文件
#include "stdefine.h"

/* 函数声明 */
void log_init  (const char *file);
void log_done  (void);
void log_printf(const char *format, ...);

#endif
