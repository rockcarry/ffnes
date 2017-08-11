// 包含头文件
#include "bus.h"

// 函数实现
static MEM* find_mem_bank(BUS bus, int baddr, int *maddr)
{
    do {
        if (baddr >= bus->start /*&& baddr <= bus->end*/) {
            switch (bus->type)
            {
            case BUS_MAP_MEMORY:
                *maddr = baddr - bus->start;
                return bus->membank;
            case BUS_MAP_MIRROR:
                baddr &= bus->mirmask;
                break;
            }
        }
    } while ((++bus)->membank);
    return NULL;
}

BYTE bus_readb(BUS bus, int addr)
{
    MEM *mbank = find_mem_bank(bus, addr, &addr);
    if (mbank) {
        return mem_readb(mbank, addr);
    }
    else return 0;
}

WORD bus_readw(BUS bus, int addr)
{
    MEM *mbank = find_mem_bank(bus, addr, &addr);
    if (mbank) {
        return mem_readw(mbank, addr);
    }
    else return 0;
}

void bus_writeb(BUS bus, int addr, BYTE byte)
{
    MEM *mbank = find_mem_bank(bus, addr, &addr);
    if (mbank) mem_writeb(mbank, addr, byte);
}

void bus_writew(BUS bus, int addr, WORD word)
{
    MEM *mbank = find_mem_bank(bus, addr, &addr);
    if (mbank) mem_writew(mbank, addr, word);
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

// bus read without rw callback
BYTE bus_readb_norwcb(BUS bus, int addr)
{
    MEM *mbank = find_mem_bank(bus, addr, &addr);
    if (mbank) {
        return mem_readb_norwcb(mbank, addr);
    }
    else return 0;
}

WORD bus_readw_norwcb(BUS bus, int addr)
{
    MEM *mbank = find_mem_bank(bus, addr, &addr);
    if (mbank) {
        return mem_readw_norwcb(mbank, addr);
    }
    else return 0;
}

//++ for fast cbus
static MEM* find_mem_bank_fast_cbus(BUS bus, int baddr, int *maddr)
{
    static int map[8] = { 5, 4, 3, 2, 1, 1, 0, 0 };
    if (baddr & 0x8000) {
        *maddr = baddr & 0x7FFF;
    } else {
        *maddr = baddr & 0x3FFF;
    }
    return bus[map[baddr>>13]].membank;
}

BYTE bus_readb_fast_cbus(BUS bus, int addr)
{
    MEM *mbank = find_mem_bank_fast_cbus(bus, addr, &addr);
    return mbank ? mem_readb(mbank, addr) : 0;
}

WORD bus_readw_fast_cbus(BUS bus, int addr)
{
    MEM *mbank = find_mem_bank_fast_cbus(bus, addr, &addr);
    return mbank ? mem_readw(mbank, addr) : 0;
}

void bus_writeb_fast_cbus(BUS bus, int addr, BYTE byte)
{
    MEM *mbank = find_mem_bank_fast_cbus(bus, addr, &addr);
    if (mbank) mem_writeb(mbank, addr, byte);
}

void bus_writew_fast_cbus(BUS bus, int addr, WORD word)
{
    MEM *mbank = find_mem_bank_fast_cbus(bus, addr, &addr);
    if (mbank) mem_writew(mbank, addr, word);
}
//-- for fast cbus

//++ for fast pbus
static MEM* find_mem_bank_fast_pbus(BUS bus, int baddr, int *maddr)
{
    int b;
    if (baddr & 0x2000) {
        b = 5 - ((baddr & 0x0C00) >> 10);
        *maddr = baddr & 0x03FF;
    } else {
        b = (baddr & 0x1000) ? 6 : 7;
        *maddr = baddr & 0x0FFF;
    }
    return bus[b].membank;
}

BYTE bus_readb_fast_pbus(BUS bus, int addr)
{
    MEM *mbank = find_mem_bank_fast_pbus(bus, addr, &addr);
    return mbank ? mem_readb(mbank, addr) : 0;
}

WORD bus_readw_fast_pbus(BUS bus, int addr)
{
    MEM *mbank = find_mem_bank_fast_pbus(bus, addr, &addr);
    return mbank ? mem_readw(mbank, addr) : 0;
}

void bus_writeb_fast_pbus(BUS bus, int addr, BYTE byte)
{
    MEM *mbank = find_mem_bank_fast_pbus(bus, addr, &addr);
    if (mbank) mem_writeb(mbank, addr, byte);
}

void bus_writew_fast_pbus(BUS bus, int addr, WORD word)
{
    MEM *mbank = find_mem_bank_fast_pbus(bus, addr, &addr);
    if (mbank) mem_writew(mbank, addr, word);
}
//-- for fast pbus
