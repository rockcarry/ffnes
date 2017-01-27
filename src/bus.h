#ifndef _NES_BUS_H_
#define _NES_BUS_H_

// 包含头文件
#include "stdefine.h"
#include "mem.h"

// 预编译开关
#define ENABLE_FAST_CBUS

// 常量定义
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
} *BUS, BUSITEM;

// 函数声明
BYTE bus_readb (BUS bus, int addr);
WORD bus_readw (BUS bus, int addr);
void bus_writeb(BUS bus, int addr, BYTE byte);
void bus_writew(BUS bus, int addr, WORD word);
void bus_setmem(BUS bus, int i, int start, int end, MEM *membank);
void bus_setmir(BUS bus, int i, int start, int end, WORD mirmask);

// bus read without rw callback
BYTE bus_readb_norwcb(BUS bus, int addr);
WORD bus_readw_norwcb(BUS bus, int addr);

BYTE bus_readb_fast_cbus (BUS bus, int addr);
WORD bus_readw_fast_cbus (BUS bus, int addr);
void bus_writeb_fast_cbus(BUS bus, int addr, BYTE byte);
void bus_writew_fast_cbus(BUS bus, int addr, WORD word);

#endif



