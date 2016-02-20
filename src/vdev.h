#ifndef _NES_VDEV_H_
#define _NES_VDEV_H_

// 包含头文件
#include "stdefine.h"

// 类型定义
typedef struct {
    void* (*create       )(int w, int h, DWORD extra);
    void  (*destroy      )(void *ctxt);
    void  (*bufrequest   )(void *ctxt, void **buf, int *stride);
    void  (*bufpost      )(void *ctxt);
    void  (*textout      )(void *ctxt, int x, int y, char *text, int time, int priority);
    void  (*setfullscreen)(void *ctxt, int full);
    int   (*getfullscreen)(void *ctxt);
} VDEV;

// 全局变量声明
extern VDEV DEV_GDI;
extern VDEV DEV_D3D;

#endif

