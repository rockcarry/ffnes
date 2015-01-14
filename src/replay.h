#ifndef _NES_REPLAY_H_
#define _NES_REPLAY_H_

// 包含头文件
#include <stdio.h>
#include "stdefine.h"

// 常量定义
enum
{
    NES_REPLAY_RECORD,
    NES_REPLAY_PLAY,
};

// 类型定义
typedef struct {
    char  file[MAX_PATH];
    int   mode;
    void *lzwfp;
} REPLAY;

// 函数声明
void replay_init (REPLAY *rep);
void replay_free (REPLAY *rep);
void replay_reset(REPLAY *rep);
BYTE replay_run  (REPLAY *rep, BYTE data);
void replay_save (REPLAY *rep, char *file);
void replay_load (REPLAY *rep, char *file);

#endif


