// 包含头文件
#include "nes.h"

// 内部函数实现
static void mmc_switch_pbank0(MMC *mmc, int bank)
{
    //++ if bank switched, set ndb.banksw 1 to notify ndb ++//
    NES *nes = container_of(mmc, NES, mmc);
    if (mmc->bank8000 != -1 && mmc->bank8000 != bank) nes->ndb.banksw = 0x8000;
    mmc->bank8000 = bank;
    //-- if bank switched, set ndb.banksw 1 to notify ndb --//

    bank = (bank == -1) ? (mmc->cart->prom_count - 1) : bank; // -1 is special, means the last bank
    mmc->cbus[6].membank->data = mmc->cart->buf_prom + 0x4000 * (bank % mmc->cart->prom_count);
}

static void mmc_switch_pbank1(MMC *mmc, int bank)
{
    //++ if bank switched, set ndb.banksw 1 to notify ndb ++//
    NES *nes = container_of(mmc, NES, mmc);
    if (mmc->bankc000 != -1 && mmc->bankc000 != bank) nes->ndb.banksw = 0xc000;
    mmc->bankc000 = bank;
    //-- if bank switched, set ndb.banksw 1 to notify ndb --//

    bank = (bank == -1) ? (mmc->cart->prom_count - 1) : bank; // -1 is special, means the last bank
    mmc->cbus[7].membank->data = mmc->cart->buf_prom + 0x4000 * (bank % mmc->cart->prom_count);
}

static void mmc_switch_cbank(MMC *mmc, int bank)
{
    mmc->bankchrrom = bank;
    bank = (bank == -1) ? (mmc->cart->crom_count - 1) : bank; // -1 is special, means the last bank
    mmc->pbus[2].membank->data = mmc->cart->buf_crom + 0x2000 * (bank % mmc->cart->crom_count);
}

// 内部类型定义
typedef struct
{
    void (*init )(MMC *mmc);
    void (*free )(MMC *mmc);
    void (*reset)(MMC *mmc);
    void (*wcb0 )(MEM *pm, int addr, BYTE byte);
    void (*wcb1 )(MEM *pm, int addr, BYTE byte);
} MAPPER;


//++ mapper000 实现 ++//
static void mapper000_reset(MMC *mmc)
{
    mmc_switch_pbank0(mmc, 0); // prom0 - first bank
    mmc_switch_pbank1(mmc,-1); // prom1 - last  bank
}

static MAPPER mapper000 =
{
    NULL,
    NULL,
    mapper000_reset,
    NULL,
    NULL,
};
//-- mapper000 实现 --//


//++ mapper002 实现 ++//
static void mapper002_reset(MMC *mmc)
{
    mmc_switch_pbank0(mmc, 0); // prom0 - first bank
    mmc_switch_pbank1(mmc,-1); // prom1 - last  bank
}

static void mapper002_wcb0(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom0);
    mmc_switch_pbank0(&(nes->mmc), byte);
}

static void mapper002_wcb1(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom1);
    mmc_switch_pbank0(&(nes->mmc), byte);
}

static void mapper002_init(MMC *mmc)
{
    mmc->pbus[2].membank->type = MEM_RAM;
    mmc->pbus[2].membank->data = mmc->chrram;
}

static MAPPER mapper002 =
{
    mapper002_init,
    NULL,
    mapper002_reset,
    mapper002_wcb0,
    mapper002_wcb1,
};
//-- mapper002 实现 --//


//++ mapper003 实现 ++//
static void mapper003_reset(MMC *mmc)
{
    mmc_switch_cbank(mmc, 0);
}

static void mapper003_wcb0(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom0);
    mmc_switch_cbank(&(nes->mmc), byte);
}

static void mapper003_wcb1(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom1);
    mmc_switch_cbank(&(nes->mmc), byte);
}

static MAPPER mapper003 =
{
    NULL,
    NULL,
    mapper003_reset,
    mapper003_wcb0,
    mapper003_wcb1,
};
//-- mapper003 实现 --//

//++ mapper list ++//
static MAPPER *g_mapper_list[256] =
{
    &mapper000,
    NULL,
    &mapper002,
    &mapper003,
    NULL,
};
//-- mapper list --//

// 函数实现
void mmc_init(MMC *mmc, CARTRIDGE *cart, BUS cbus, BUS pbus)
{
    MAPPER *mapper;
    mmc->cart   = cart;
    mmc->cbus   = cbus;
    mmc->pbus   = pbus;
    mmc->number = cartridge_get_mappercode(cart);

    // get mapper by number
    mapper = g_mapper_list[mmc->number];

    // init cbus memory bank callback
    mmc->cbus[6].membank->w_callback = mapper->wcb0;
    mmc->cbus[7].membank->w_callback = mapper->wcb1;

    // call mapper init if exsits
    if (mapper && mapper->init ) mapper->init(mmc);

    // call mmc reset
    mmc_reset(mmc);
}

void mmc_free(MMC *mmc)
{
    MAPPER *mapper = g_mapper_list[mmc->number];
    if (mapper && mapper->free) mapper->free(mmc);
}

void mmc_reset(MMC *mmc)
{
    MAPPER *mapper = g_mapper_list[mmc->number];

    //++ by default, need mapper0 reset first
    mmc->bank8000 = -1; mmc->bankc000 = -1; mmc->bankchrrom = -1;
    g_mapper_list[0]->reset(mmc);
    //-- by default, need mapper0 reset first

    //++ call mapper xxx reset if exsits
    mmc->bank8000 = -1; mmc->bankc000 = -1; mmc->bankchrrom = -1;
    if (mapper && mapper->reset) mapper->reset(mmc);
    //-- call mapper xxx reset if exsits
}
