// 包含头文件
#include "nes.h"

// 内部函数实现
static void mmc_switch_pbank16k0(MMC *mmc, int bank)
{
    BYTE *oldbank0 = mmc->cbus[1].membank->data;
    mmc->pbanksize = 16 * 1024;
    mmc->pbank8000 = bank;
    bank = (bank == -1) ? (mmc->cart->prom_count - 1) : bank; // -1 is special, means the last bank
    mmc->cbus[1].membank->data = mmc->cart->buf_prom + 0x4000 * (bank % mmc->cart->prom_count);

    if (oldbank0 != mmc->cbus[1].membank->data)
    {
        ((NES*)container_of(mmc, NES, mmc))->ndb.banksw = 1;
    }
}

static void mmc_switch_pbank16k1(MMC *mmc, int bank)
{
    BYTE *oldbank1 = mmc->cbus[0].membank->data;
    mmc->pbanksize = 16 * 1024;
    mmc->pbankc000 = bank;
    bank = (bank == -1) ? (mmc->cart->prom_count - 1) : bank; // -1 is special, means the last bank
    mmc->cbus[0].membank->data = mmc->cart->buf_prom + 0x4000 * (bank % mmc->cart->prom_count);

    if (oldbank1 != mmc->cbus[0].membank->data)
    {
        ((NES*)container_of(mmc, NES, mmc))->ndb.banksw = 1;
    }
}

static void mmc_switch_pbank32k(MMC *mmc, int bank)
{
    BYTE *oldbank0 = mmc->cbus[1].membank->data;
    BYTE *oldbank1 = mmc->cbus[0].membank->data;

    mmc->pbanksize = 32 * 1024;
    mmc->pbank8000 = bank;
    mmc->pbankc000 = bank;

    bank = (bank == -1) ? (mmc->cart->prom_count - 2) : bank * 2; // -1 is special, means the last bank
    bank = (bank <   0) ? 0 : bank; // make sure bank is >= 0
    mmc->cbus[1].membank->data = mmc->cbus[0].membank->data = mmc->cart->buf_prom + 0x4000 * (bank % mmc->cart->prom_count);
    if (bank + 1 < mmc->cart->prom_count) mmc->cbus[0].membank->data += 0x4000;

    if (oldbank0 != mmc->cbus[1].membank->data || oldbank1 != mmc->cbus[0].membank->data)
    {
        ((NES*)container_of(mmc, NES, mmc))->ndb.banksw = 1;
    }
}

static void mmc_switch_cbank4k0(MMC *mmc, int bank)
{
    mmc->cbanksize = 4 * 1024;
    mmc->cbank0000 = bank;
    bank = (bank == -1) ? (mmc->cart->crom_count * 2 - 1) : bank; // -1 is special, means the last bank
    mmc->pbus[8].membank->data = mmc->cart->buf_crom + 0x1000 * (bank % (mmc->cart->crom_count * 2));
}

static void mmc_switch_cbank4k1(MMC *mmc, int bank)
{
    mmc->cbanksize = 4 * 1024;
    mmc->cbank1000 = bank;
    bank = (bank == -1) ? (mmc->cart->crom_count * 2 - 1) : bank; // -1 is special, means the last bank
    mmc->pbus[7].membank->data = mmc->cart->buf_crom + 0x1000 * (bank % (mmc->cart->crom_count * 2));
}

static void mmc_switch_cbank8k(MMC *mmc, int bank)
{
    mmc->cbanksize = 8 * 1024;
    mmc->cbank0000 = bank;
    mmc->cbank1000 = bank;
    bank = (bank == -1) ? (mmc->cart->crom_count - 1) : bank; // -1 is special, means the last bank
    mmc->pbus[8].membank->data = mmc->cart->buf_crom + 0x2000 * (bank % mmc->cart->crom_count);
    mmc->pbus[7].membank->data = mmc->pbus[8].membank->data + 0x1000;
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


//++ mapper001 实现 ++//
#define MAPPER001_TEMPREG  (nes->mmc.regs[6])
#define MAPPER001_COUNTER  (nes->mmc.regs[7])
#define MAPPER001_REG(x)   (nes->mmc.regs[x])
static void mapper001_reset(MMC *mmc)
{
    mmc->regs[0] = 0x0c; // on powerup, bits 2,3 of $8000 are set (16k PRG mode, $8000 swappable)
    mmc->regs[3] = 0x00; // when emudeving, assume WRAM is enabled at startup
    mmc->regs[6] = 0x00; // set temp reg to 0
    mmc->regs[7] = 0x00; // set bit counter to 0
}

static void mapper001_wcb0(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom0);
    if (byte & (1 << 7))
    {
        MAPPER001_TEMPREG = 0;    // reset tempreg
        MAPPER001_COUNTER = 0;    // reset counter
        MAPPER001_REG(0) |= 0xc0; // bits 2,3 of reg $8000 are set (16k PRG mode, $8000 swappable)
    }
    else
    {
        // shift bit into temp reg
        MAPPER001_TEMPREG |= (byte & 1) << MAPPER001_COUNTER;
        if (++MAPPER001_COUNTER == 5)
        {
            //++ for vram nametable mirroring ++//
            static int vram_mirroring_map[4][4] =
            {
                {0, 0, 0, 0}, // one-screen, lower bank
                {1, 1, 1, 1}, // one-screen, upper bank
                {0, 1, 0, 1}, // vertical
                {0, 0, 1, 1}, // horizontal
            };
            int *mirroring = NULL, i;
            //-- for vram nametable mirroring --//

            // it's time to store value into reg
            int regaddr = 0 + (addr >> 13);
            MAPPER001_REG(regaddr) = MAPPER001_TEMPREG;

            //++ handle mapper001 reg writing ++//
            switch (regaddr)
            {
            case 0:
                // for mmc1 mirroring control
                mirroring = vram_mirroring_map[MAPPER001_REG(0) & 0x3];
                for (i=0; i<4; i++) nes->vram[i].data = nes->buf_vram[mirroring[i]];
                break;

            case 1:
                // chr rom reg0, chr rom bank switch 0
                if (MAPPER001_REG(0) & (1 << 4))
                {   // 4k mode
                    mmc_switch_cbank4k0(&(nes->mmc), MAPPER001_REG(0) & 0x1f);
                }
                else
                {   // 8k mode
                    mmc_switch_cbank8k (&(nes->mmc), MAPPER001_REG(0) & 0x1f);
                }
                break;
            }
            //-- handle mapper001 reg writing --//

            // reset tempreg & counter
            MAPPER001_TEMPREG = 0;
            MAPPER001_COUNTER = 0;
        }
    }
}

static void mapper001_wcb1(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, prom1);
    if (byte & (1 << 7))
    {
        MAPPER001_TEMPREG = 0;    // reset tempreg
        MAPPER001_COUNTER = 0;    // reset counter
        MAPPER001_REG(0) |= 0xc0; // bits 2,3 of reg $8000 are set (16k PRG mode, $8000 swappable)
    }
    else
    {
        // shift bit into temp reg
        MAPPER001_TEMPREG |=  (byte & 1) << MAPPER001_COUNTER;
        if (++MAPPER001_COUNTER == 5)
        {
            // it's time to store value into reg
            int regaddr = 1 + (addr >> 13);
            MAPPER001_REG(regaddr) = MAPPER001_TEMPREG;

            //++ handle mapper001 reg writing ++//
            switch (regaddr)
            {
            case 2:
                // chr rom reg1, chr rom bank switch 1
                if (MAPPER001_REG(0) & (1 << 4))
                {   // 4k mode
                    mmc_switch_cbank4k0(&(nes->mmc), MAPPER001_REG(2) & 0x1f);
                }
                break;

            case 3:
                //++ prg rom bank switch
                if (MAPPER001_REG(0) & (1 << 3))
                {   // 16k mode
                    if (MAPPER001_REG(0) & (1 << 4))
                    {   // $8000 swappable, $C000 fixed to page $0F (mode B)
                        mmc_switch_pbank16k0(&(nes->mmc), MAPPER001_REG(3) & 0x0f);
                        mmc_switch_pbank16k1(&(nes->mmc), -1                     );
                    }
                    else
                    {   // $C000 swappable, $8000 fixed to page $00 (mode A)
                        mmc_switch_pbank16k0(&(nes->mmc), 0                      );
                        mmc_switch_pbank16k1(&(nes->mmc), MAPPER001_REG(3) & 0x0f);
                    }
                }
                else
                {   // 32k mode
                    mmc_switch_pbank32k(&(nes->mmc), (MAPPER001_REG(3) & 0x0f) >> 1);
                }
                //-- prg rom bank switch

                //++ for wram enable/disable
                if (MAPPER001_REG(3) & (1 << 4))
                {
                    nes->cbus[4].membank = NULL;
                }
                else
                {
                    nes->cbus[4].membank = &(nes->sram);
                }
                //-- for wram enable/disable
                break;
            }
            //-- handle mapper001 reg writing --//

            // reset tempreg & counter
            MAPPER001_TEMPREG = 0;
            MAPPER001_COUNTER = 0;
        }
    }
}

static MAPPER mapper001 =
{
    NULL,
    NULL,
    mapper001_reset,
    mapper001_wcb0,
    mapper001_wcb1,
};
//-- mapper001 实现 --//


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
    &mapper001,
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
        mmc->cbus[1].membank->w_callback = mapper->wcb0;
        mmc->cbus[0].membank->w_callback = mapper->wcb1;
    }

    // if cartridge is using chr-ram
    if (mmc->cart->ischrram)
    {
        mmc->pbus[8].membank->type = MEM_RAM;
        mmc->pbus[7].membank->type = MEM_RAM;
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
