#ifndef _NES_BUS_H_
#define _NES_BUS_H_

#include "mem.h"

#define ENABLE_FAST_CBUS
#define ENABLE_FAST_PBUS

enum {
    BUS_MAP_MEMORY,
    BUS_MAP_MIRROR,
};

typedef struct {
    int  type;
    int  start;
    int  end;
    union {
        MEM     *membank;
        uint16_t mirmask;
    };
} BUS;

uint8_t  bus_readb (BUS *bus, int addr);
uint16_t bus_readw (BUS *bus, int addr);
void     bus_writeb(BUS *bus, int addr, uint8_t  byte);
void     bus_writew(BUS *bus, int addr, uint16_t word);
void     bus_setmem(BUS *bus, int i, int start, int end, MEM     *membank);
void     bus_setmir(BUS *bus, int i, int start, int end, uint16_t mirmask);

// bus read without rw callback
uint8_t  bus_readb_norwcb(BUS *bus, int addr);
uint16_t bus_readw_norwcb(BUS *bus, int addr);

uint8_t  bus_readb_fast_cbus (BUS *bus, int addr);
uint16_t bus_readw_fast_cbus (BUS *bus, int addr);
void     bus_writeb_fast_cbus(BUS *bus, int addr, uint8_t  byte);
void     bus_writew_fast_cbus(BUS *bus, int addr, uint16_t word);

uint8_t  bus_readb_fast_pbus (BUS *bus, int addr);
uint16_t bus_readw_fast_pbus (BUS *bus, int addr);
void     bus_writeb_fast_pbus(BUS *bus, int addr, uint8_t  byte);
void     bus_writew_fast_pbus(BUS *bus, int addr, uint16_t word);

#endif



