#ifndef _NES_MEM_H_
#define _NES_MEM_H_

#include <stdint.h>

enum {
    MEM_ROM,
    MEM_RAM,
    MEM_REG,
};

typedef struct tagMEM {
    int      type;
    int      size;
    uint8_t *data;
    uint8_t (*r_callback)(struct tagMEM *pm, int addr);
    void    (*w_callback)(struct tagMEM *pm, int addr, uint8_t data);
} MEM;

uint8_t  mem_readb (MEM *pm, int addr);
uint16_t mem_readw (MEM *pm, int addr);
void     mem_writeb(MEM *pm, int addr, uint8_t  byte);
void     mem_writew(MEM *pm, int addr, uint16_t word);

uint8_t  mem_readb_norwcb(MEM *pm, int addr);
uint16_t mem_readw_norwcb(MEM *pm, int addr);

#endif

