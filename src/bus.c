// 包含头文件
#include "bus.h"

// 函数实现
static MEM* find_mem_bank(BUS bus, int baddr, int *maddr)
{
    int i;
    for (i=0; bus[i].membank; i++) {
        if (baddr >= bus[i].start && baddr <= bus[i].end)
        {
            if (bus[i].type == BUS_MAP_MIRROR) {
                baddr &= bus[i].mirmask;
                continue;
            }

            if (bus[i].type == BUS_MAP_MEMORY) {
                *maddr = baddr - bus[i].start;
                return bus[i].membank;
            }
        }
    }
    return NULL;
}

BYTE bus_readb(BUS bus, int baddr)
{
    int  maddr = 0;
    MEM *mbank = find_mem_bank(bus, baddr, &maddr);
    if (mbank) {
        return mem_readb(mbank, maddr);
    }
    else return 0;
}

WORD bus_readw(BUS bus, int baddr)
{
    int  maddr = 0;
    MEM *mbank = find_mem_bank(bus, baddr, &maddr);
    if (mbank) {
        return mem_readw(mbank, maddr);
    }
    else return 0;
}

void bus_writeb(BUS bus, int baddr, BYTE byte)
{
    int  maddr = 0;
    MEM *mbank = find_mem_bank(bus, baddr, &maddr);
    if (mbank) {
        mem_writeb(mbank, maddr, byte);
    }
}

void bus_writew(BUS bus, int baddr, WORD word)
{
    int  maddr = 0;
    MEM *mbank = find_mem_bank(bus, baddr, &maddr);
    if (mbank) {
        mem_writew(mbank, maddr, word);
    }
}

void bus_setmem(BUS bus, int i, int start, int end, MEM *membank)
{
    bus[i].type    = BUS_MAP_MEMORY;
    bus[i].start   = start;
    bus[i].end     = end;
    bus[i].membank = membank;
}

void bus_setmir(BUS bus, int i, int start, int end, WORD mirmask)
{
    bus[i].type    = BUS_MAP_MIRROR;
    bus[i].start   = start;
    bus[i].end     = end;
    bus[i].mirmask = mirmask;
}


