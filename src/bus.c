// 包含头文件
#include "bus.h"

// 函数实现
static int find_mem_bank(BUS bus, int addr)
{
    int i;
    for (i=0; i<NES_MAX_BUS_SIZE; i++) {
        if (!bus[i].mem || (addr >= bus[i].start && addr <= bus[i].end))
        {
            return i;
        }
    }
    return -1;
}

void bus_read(BUS bus, int addr, BYTE *data)
{
    int bank = find_mem_bank(bus, addr);
    if (bank > 0) {
        mem_read(bus[bank].mem, addr - bus[bank].start, data);
    }
}

void bus_write(BUS bus, int addr, BYTE data)
{
    int bank = find_mem_bank(bus, addr);
    if (bank > 0) {
        mem_write(bus[bank].mem, addr - bus[bank].start, data);
    }
}

void bus_setmap(BUS bus, int i, int start, int end, MEM *mem)
{
    bus[i].start = start;
    bus[i].end   = end;
    bus[i].mem   = mem;
}
