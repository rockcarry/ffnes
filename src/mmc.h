#ifndef _NES_MMC_H_
#define _NES_MMC_H_

#include "bus.h"
#include "cartridge.h"

typedef struct {
    CARTRIDGE *cart;
    BUS       *cbus;
    BUS       *pbus;
    int        number; // mapper number
    uint8_t    regs[8];// mmc internel regs

    // for bank switch
    int        pbanksize;
    int        pbank8000;
    int        pbankc000;
    int        cbanksize;
    int        cbank0000;
    int        cbank1000;
} MMC;

void mmc_init (MMC *mmc, CARTRIDGE *cart, BUS *cbus, BUS *pbus);
void mmc_free (MMC *mmc);
void mmc_reset(MMC *mmc);

// internal used by save.c
void mmc_switch_pbank16k0(MMC *mmc, int bank);
void mmc_switch_pbank16k1(MMC *mmc, int bank);
void mmc_switch_pbank32k (MMC *mmc, int bank);
void mmc_switch_cbank4k0 (MMC *mmc, int bank);
void mmc_switch_cbank4k1 (MMC *mmc, int bank);
void mmc_switch_cbank8k  (MMC *mmc, int bank);

#endif

