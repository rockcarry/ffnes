// 包含头文件
#include "mmc.h"

// 内部类型定义
typedef struct
{
    void (*init )(MMC *mmc);
    void (*free )(MMC *mmc);
    void (*reset)(MMC *mmc);
    void (*rcb  )(MEM *pm, int addr);
    void (*wcb  )(MEM *pm, int addr);
} MAPPER;

//++ mapper002 实现 ++//
static void mapper002_init(MMC *mmc)
{
}

static void mapper002_free(MMC *mmc)
{
}

static void mapper002_reset(MMC *mmc)
{
}

static void mapper002_rcb(MEM *pm, int addr)
{
}

static void mapper002_wcb(MEM *pm, int addr)
{
}

static MAPPER mapper002 =
{
    mapper002_init,
    mapper002_free,
    mapper002_reset,
    mapper002_rcb,
    mapper002_wcb,
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
