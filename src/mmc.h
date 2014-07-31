#ifndef _NES_MMC_H_
#define _NES_MMC_H_

// 包含头文件
#include "stdefine.h"
#include "cartridge.h"
#include "bus.h"

// 类型定义
typedef struct {
    CARTRIDGE *cart;
    BUS        cbus;
    BUS        pbus;
    int      number;
    BYTE      *pram;
} MMC;

// 函数声明
void mmc_init (MMC *mmc, CARTRIDGE *cart, BUS cbus, BUS pbus);
void mmc_free (MMC *mmc);
void mmc_reset(MMC *mmc);

#endif

