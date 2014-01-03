#ifndef _NES_MEM_H_
#define _NES_MEM_H_

// 包含头文件
#include "stdefine.h"

// 类型定义
enum {
    MEM_ROM,
    MEM_RAM,
    MEM_REG,
};

typedef struct {
    int   type;
    int   size;
    BYTE *data;
} MEM;

// 函数声明
void mem_read (MEM *pm, int addr, BYTE *byte);
void mem_write(MEM *pm, int addr, BYTE  byte);

#endif

