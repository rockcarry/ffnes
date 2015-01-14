#ifndef _NES_SAVE_H_
#define _NES_SAVE_H_

// 包含头文件
#include "nes.h"

// 函数声明
void saver_save_game(NES  *nes , char *file);
void saver_load_game(NES  *nes , char *file);

#endif

