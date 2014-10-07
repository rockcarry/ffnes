// 包含头文件
#include "nes.h"

// 内部函数实现
static void mmc_switch_pbank16k0(MMC *mmc, int bank)
{
    BYTE *oldbank0 = mmc->cbus[5].membank->data;
    mmc->pbanksize = 16 * 1024;
    mmc->pbank8000 = bank;
    bank = (bank == -1) ? (mmc->cart->prom_count - 1) : bank; // -1 is special, means the last bank
    mmc->cbus[5].membank->data = mmc->cart->buf_prom + 0x4000 * (bank % mmc->cart->prom_count);

    if (oldbank0 != mmc->cbus[5].membank->data)
    {
        ((NES*)container_of(mmc, NES, mmc))->ndb.banksw = 1;
    }
}

static void mmc_switch_pbank16k1(MMC *mmc, int bank)
{
    BYTE *oldbank1 = mmc->cbus[6].membank->data;
    mmc->pbanksize = 16 * 1024;
    mmc->pbankc000 = bank;
    bank = (bank == -1) ? (mmc->cart->prom_count - 1) : bank; // -1 is special, means the last bank
    mmc->cbus[6].membank->data = mmc->cart->buf_prom + 0x4000 * (bank % mmc->cart->prom_count);

    if (oldbank1 != mmc->cbus[6].membank->data)
    {
        ((NES*)container_of(mmc, NES, mmc))->ndb.banksw = 1;
    }
}

static void mmc_switch_pbank32k(MMC *mmc, int bank)
{
    BYTE *oldbank0 = mmc->cbus[5].membank->data;
    BYTE *oldbank1 = mmc->cbus[6].membank->data;

    mmc->pbanksize = 32 * 1024;
    mmc->pbank8000 = bank;
    mmc->pbankc000 = bank;

    bank = (bank == -1) ? (mmc->cart->prom_count - 2) : bank * 2; // -1 is special, means the last bank
    bank = (bank <   0) ? 0 : bank; // make sure bank is >= 0
    mmc->cbus[5].membank->data = mmc->cbus[6].membank->data = mmc->cart->buf_prom + 0x4000 * (bank % mmc->cart->prom_count);
    if (bank + 1 < mmc->cart->prom_count) mmc->cbus[6].membank->data += 0x4000;

    if (oldbank0 != mmc->cbus[5].membank->data || oldbank1 != mmc->cbus[6].membank->data)
    {
        ((NES*)container_of(mmc, NES, mmc))->ndb.banksw = 1;
    }
}

static void mmc_switch_cbank4k0(MMC *mmc, int bank)
{
    mmc->cbanksize = 4 * 1024;
    mmc->cbank0000 = bank;
    bank = (bank == -1) ? (mmc->cart->crom_count * 2 - 1) : bank; // -1 is special, means the last bank
    mmc->pbus[2].membank->data = mmc->cart->buf_crom + 0x1000 * (bank % (mmc->cart->crom_count * 2));
}

static void mmc_switch_cbank4k1(MMC *mmc, int bank)
{
    mmc->cbanksize = 4 * 1024;
    mmc->cbank1000 = bank;
    bank = (bank == -1) ? (mmc->cart->crom_count * 2 - 1) : bank; // -1 is special, means the last bank
    mmc->pbus[3].membank->data = mmc->cart->buf_crom + 0x1000 * (bank % (mmc->cart->crom_count * 2));
}

static void mmc_switch_cbank8k(MMC *mmc, int bank)
{
    mmc->cbanksize = 8 * 1024;
    mmc->cbank0000 = bank;
    mmc->cbank1000 = bank;
    bank = (bank == -1) ? (mmc->cart->crom_count - 1) : bank; // -1 is special, means the last bank
    mmc->pbus[2].membank->data = mmc->cart->buf_crom + 0x2000 * (bank % mmc->cart->crom_count);
    mmc->pbus[3].membank->data = mmc->pbus[2].membank->data + 0x1000;
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
    mmc_switch_pbank16k0(mmc, 0); // prom0 - first bank
    mmc_switch_pbank16k1(mmc,-1); // prom1 - last  bank
    mmc_switch_cbank4k0 (mmc, 0); // crom0 - first bank
    mmc_switch_cbank4k1 (mmc,-1); // crom1 - last  bank
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
static void mapper002_wcb0(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom0);
    mmc_switch_pbank16k0(&(nes->mmc), byte);
}

static void mapper002_wcb1(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom1);
    mmc_switch_pbank16k0(&(nes->mmc), byte);
}

static MAPPER mapper002 =
{
    NULL,
    NULL,
    NULL,
    mapper002_wcb0,
    mapper002_wcb1,
};
//-- mapper002 实现 --//


//++ mapper003 实现 ++//
static void mapper003_reset(MMC *mmc)
{
    mmc_switch_cbank8k(mmc, 0);
}

static void mapper003_wcb0(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom0);
    mmc_switch_cbank8k(&(nes->mmc), byte);
}

static void mapper003_wcb1(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom1);
    mmc_switch_cbank8k(&(nes->mmc), byte);
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
    if (mapper)
    {
        mmc->cbus[5].membank->w_callback = mapper->wcb0;
        mmc->cbus[6].membank->w_callback = mapper->wcb1;
    }

    // if cartridge is using chr-ram
    if (mmc->cart->ischrram)
    {
        mmc->pbus[2].membank->type = MEM_RAM;
        mmc->pbus[3].membank->type = MEM_RAM;
    }

    // call mapper init if exsits
    if (mapper && mapper->init) mapper->init(mmc);

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

    // need do mapper0 reset first
    g_mapper_list[0]->reset(mmc);

    // do call mapper xxx reset if exsits
    if (mapper && mapper->reset) mapper->reset(mmc);
}
