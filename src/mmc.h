#ifndef _NES_MMC_H_
#define _NES_MMC_H_

// 包含头文件
#include "stdefine.h"
#include "cartridge.h"
#include "bus.h"

// 类型定义
typedef struct {
    CARTRIDGE     *cart;
    BUS            cbus;
    BUS            pbus;
    int          number; // mapper number
    BYTE chrram[0x2000]; // some cartridge don't have CHR-ROM
                         // but use 8KB CHR-RAM instead of it
    int        bank8000;
    int        bankc000;
    int        bankchrrom;
} MMC;

// 函数声明
void mmc_init (MMC *mmc, CARTRIDGE *cart, BUS cbus, BUS pbus);
void mmc_free (MMC *mmc);
void mmc_reset(MMC *mmc);

#endif

