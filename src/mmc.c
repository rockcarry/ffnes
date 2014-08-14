// 包含头文件
#include "nes.h"
#include "log.h"

// 内部类型定义
typedef struct
{
    void (*init )(MMC *mmc);
    void (*free )(MMC *mmc);
    void (*reset)(MMC *mmc);
    void (*wcb0 )(MEM *pm, int addr, BYTE byte);
    void (*wcb1 )(MEM *pm, int addr, BYTE byte);
} MAPPER;

//++ mapper002 实现 ++//
static void mapper002_reset(MMC *mmc)
{
    // prom0 - first back & prom1 - last bank
    mmc->cbus[6].membank->data = mmc->cart->buf_prom + 16384 * 0;
    mmc->cbus[7].membank->data = mmc->cart->buf_prom + 16384 * (mmc->cart->prom_count - 1);
}

static void mapper002_wcb0(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prgrom0);
    MMC *mmc = &(nes->mmc);
    mmc->cbus[6].membank->data = mmc->cart->buf_prom + 0x4000 * (byte & (mmc->cart->prom_count - 1));
}

static void mapper002_wcb1(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prgrom1);
    MMC *mmc = &(nes->mmc);
    mmc->cbus[6].membank->data = mmc->cart->buf_prom + 0x4000 * (byte & (mmc->cart->prom_count - 1));
}

static void mapper002_init(MMC *mmc)
{
    // allocate memory for 8KB ram
    mmc->pram = (BYTE*)malloc(8192);
    if (!mmc->pram) {
        log_printf("mapper002_init, malloc failed !\n");
    }
    else
    {
        memset(mmc->pram, 0, 8192);
        mmc->pbus[3].membank->type = MEM_RAM;
        mmc->pbus[3].membank->data = mmc->pram + 4096 * 0;
        mmc->pbus[4].membank->type = MEM_RAM;
        mmc->pbus[4].membank->data = mmc->pram + 4096 * 1;
    }

    // register bus memory callback
    mmc->cbus[6].membank->w_callback = mapper002_wcb0;
    mmc->cbus[7].membank->w_callback = mapper002_wcb1;

    // reset mapper002
    mapper002_reset(mmc);
}

static void mapper002_free(MMC *mmc)
{
    // free memory
    if (mmc->pram) {
        free(mmc->pram);
        mmc->pram = NULL;
    }
}

static MAPPER mapper002 =
{
    mapper002_init,
    mapper002_free,
    mapper002_reset,
    mapper002_wcb0,
    mapper002_wcb1,
};
//-- mapper02 实现 --//

//++ mapper list ++//
static MAPPER *g_mapper_list[256] =
{
    NULL,
    NULL,
    &mapper002,
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
    mapper = g_mapper_list[mmc->number];
    if (mapper && mapper->init) mapper->init(mmc);
}

void mmc_free(MMC *mmc)
{
    MAPPER *mapper;
    mapper = g_mapper_list[mmc->number];
    if (mapper && mapper->init) mapper->free(mmc);
}

void mmc_reset(MMC *mmc)
{
    MAPPER *mapper;
    mapper = g_mapper_list[mmc->number];
    if (mapper && mapper->reset) mapper->reset(mmc);
}
