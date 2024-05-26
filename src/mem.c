#include "mem.h"

uint8_t mem_readb(MEM *pm, int addr)
{
    addr %= pm->size;
    if (pm->r_callback) return pm->r_callback(pm, addr);
    if (pm->data      ) return pm->data[addr];
    return 0;
}

uint16_t mem_readw(MEM *pm, int addr)
{
    return pm->data ? *(uint16_t*)(pm->data + addr % pm->size) : 0;
}

void mem_writeb(MEM *pm, int addr, uint8_t byte)
{
    addr %= pm->size;
    if (pm->w_callback) { pm->w_callback(pm, addr, byte); return; }
    if (pm->type != MEM_ROM && pm->data) pm->data[addr] = byte;
}

void mem_writew(MEM *pm, int addr, uint16_t word)
{
    if (pm->type != MEM_ROM && pm->data) *(uint16_t*)(pm->data + addr % pm->size) = word;
}

uint8_t mem_readb_norwcb(MEM *pm, int addr)
{
    return pm->data ? pm->data[addr % pm->size] : 0;
}

uint16_t mem_readw_norwcb(MEM *pm, int addr)
{
    return pm->data ? *(uint16_t*)(pm->data + addr % pm->size) : 0;
}
