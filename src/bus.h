#ifndef _NES_BUS_H_
#define _NES_BUS_H_

// 包含头文件
#include "stdefine.h"
#include "mem.h"

// 常量定义
#define NES_MAX_BUS_SIZE  8

enum {
    BUS_MAP_MEMORY,
    BUS_MAP_MIRROR,
};

// 类型定义
typedef struct {
    int  type;
    int  start;
    int  end;
    MEM *mem;
} BUS[NES_MAX_BUS_SIZE];

// 函数声明
void bus_read  (BUS bus, int addr, BYTE *data);
void bus_write (BUS bus, int addr, BYTE  data);
void bus_setmap(BUS bus, int i, int start, int end, MEM *mem);

#endif



