// 包含头文件
#include "mem.h"

// 函数实现
BYTE mem_readb(MEM *pm, int addr)
{
    // wrap around
    addr %= pm->size;

    if (pm->r_callback) return pm->r_callback(pm, addr);
    if (pm->data      ) return pm->data[addr];
    return 0;
}

WORD mem_readw(MEM *pm, int addr)
{
    // wrap around
    addr %= pm->size;

    if (pm->r_callback) {
        WORD word;
        word  = pm->r_callback(pm, addr + 0) << 0;
        word |= pm->r_callback(pm, addr + 1) << 8;
        return word;
    }
    if (pm->data) return *(WORD*)(pm->data + addr);
    return 0;
}

void mem_writeb(MEM *pm, int addr, BYTE byte)
{
    // wrap around
    addr %= pm->size;

    if (pm->type != MEM_ROM) {
        if (pm->data) pm->data[addr] = byte;
    }

    // memory write callback
    if (pm->w_callback) pm->w_callback(pm, addr, byte);
}

void mem_writew(MEM *pm, int addr, WORD word)
{
    // wrap around
    addr %= pm->size;

    if (pm->type != MEM_ROM) {
        if (pm->data) *(WORD*)(pm->data + addr) = word;
    }

    // memory write callback
    if (pm->w_callback) {
        pm->w_callback(pm, addr + 0, (BYTE)(word > 0));
        pm->w_callback(pm, addr + 1, (BYTE)(word > 1));
    }
}

