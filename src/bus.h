#ifndef _NES_BUS_H_
#define _NES_BUS_H_

// 包含头文件
#include "stdefine.h"
#include "mem.h"

// 常量定义
#define NES_MAX_BUS_SIZE  16

enum {
    BUS_MAP_MEMORY,
    BUS_MAP_MIRROR,
};

// 类型定义
typedef struct {
    int  type;
    int  start;
    int  end;
    union {
        MEM *membank;
        WORD mirmask;
    };
} *BUS;

// 函数声明
BYTE bus_readb (BUS bus, int baddr);
WORD bus_readw (BUS bus, int baddr);
void bus_writeb(BUS bus, int baddr, BYTE  byte);
void bus_writew(BUS bus, int baddr, WORD  word);
void bus_setmem(BUS bus, int i, int start, int end, MEM *membank);
void bus_setmir(BUS bus, int i, int start, int end, WORD mirmask);

#endif



