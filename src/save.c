// 包含头文件
#include <stdio.h>
#include "save.h"
#include "adev.h"
#include "vdev.h"

// 内部类型定义
typedef struct
{
    char  signature[8]; // ffnesave
    DWORD version;      // version
    DWORD head_size;    // head size
    DWORD neso_size;    // nes object size
    DWORD sram_size;    // sram size
    DWORD cram_size;    // sram size
} NES_SAVE_FILE;

// 内部函数实现
void saver_restore_apu(APU *apu, int oldpclk, int newpclk)
{
    if (oldpclk > 0) adev_buf_post(apu->adevctxt, apu->audiobuf);
    if (newpclk > 0) adev_buf_request(apu->adevctxt, &(apu->audiobuf));
}

void saver_restore_ppu(PPU *ppu, int oldpclk, int newpclk)
{
    NES *nes = container_of(ppu, NES, ppu);

    ppu->chrom_bkg = (ppu->regs[0x0000] & (1 << 4)) ? nes->chrrom1.data : nes->chrrom0.data;
    ppu->chrom_spr = (ppu->regs[0x0000] & (1 << 3)) ? nes->chrrom1.data : nes->chrrom0.data;

    if (oldpclk <= NES_HTOTAL * 241 + 1) vdev_buf_post(ppu->vdevctxt);
    if (newpclk <= NES_HTOTAL * 241 + 1) vdev_buf_request(ppu->vdevctxt, (void**)&(ppu->draw_buffer), &(ppu->draw_stride));
}

void saver_restore_mmc(MMC *mmc)
{
    if (mmc->pbanksize == 16 * 1024)
    {
        mmc_switch_pbank16k0(mmc, mmc->pbank8000);
        mmc_switch_pbank16k1(mmc, mmc->pbankc000);
    }

    if (mmc->pbanksize == 32 * 1024)
    {
        mmc_switch_pbank32k (mmc, mmc->pbank8000);
        mmc_switch_pbank32k (mmc, mmc->pbankc000);
    }

    if (mmc->cbanksize == 4 * 1024)
    {
        mmc_switch_cbank4k0 (mmc, mmc->cbank0000);
        mmc_switch_cbank4k1 (mmc, mmc->cbank1000);
    }

    if (mmc->cbanksize == 8 * 1024)
    {
        mmc_switch_cbank8k  (mmc, mmc->cbank0000);
        mmc_switch_cbank8k  (mmc, mmc->cbank1000);
    }
}

// 函数实现
void saver_save_game(NES *nes, char *file)
{
    NES_SAVE_FILE save;
    FILE    *fp = NULL;
    int      running;

    strncpy((char*)&save , "ffnesave", sizeof(save));
    save.version   = 0x00000001; // v0.1
    save.head_size = sizeof(save);
    save.neso_size = sizeof(NES);
    save.sram_size = cartridge_has_sram(&(nes->cart)) ? 0x2000 : 0;
    save.cram_size = nes->cart.ischrram ? nes->cart.crom_count * 0x2000 : 0;

    // open file
    fp = fopen(file, "wb");
    if (!fp) return;

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    if (save.head_size) fwrite(&save             , sizeof(save)  , 1, fp);
    if (save.neso_size) fwrite(nes               , sizeof(NES)   , 1, fp);
    if (save.sram_size) fwrite(nes->cart.buf_sram, save.sram_size, 1, fp);
    if (save.cram_size) fwrite(nes->cart.buf_crxm, save.cram_size, 1, fp);

    // resume running
    if (running) nes_setrun(nes, 1);

    // close file
    fclose(fp);
}

void saver_load_game(NES *nes, char *file)
{
    NES_SAVE_FILE save;
    FILE         *fp;
    NES           buf;
    int           start, end, i;
    int           oldapupclk, newapupclk;
    int           oldppupclk, newppupclk;
    int           running;

    // protected member list
    static int plist[][2] =
    {
        { offsetof(NES, cpu.cbus    ),  sizeof(BUS       ) },
        { offsetof(NES, ppu.vdevctxt),  sizeof(void*     ) },
        { offsetof(NES, apu.adevctxt),  sizeof(void*     ) },
        { offsetof(NES, mmc.cart    ),  sizeof(CARTRIDGE*) },
        { offsetof(NES, mmc.cbus    ),  sizeof(BUS       ) },
        { offsetof(NES, mmc.pbus    ),  sizeof(BUS       ) },
        { offsetof(NES, cart        ),  sizeof(CARTRIDGE ) },
        { offsetof(NES, extra       ),  sizeof(DWORD     ) },
        { offsetof(NES, cbus        ),  (offsetof(NES, ndb) - offsetof(NES, cbus)) },
        { offsetof(NES, ndb.nes     ),  sizeof(NES*      ) },
        { sizeof(NES)                ,  0                  },
    };

    // open save file
    fp = fopen(file, "rb");
    if (!fp) return;

    // read save file header
    fread(&save, sizeof(save), 1, fp);

    if (save.neso_size)
    {
        fseek(fp, save.head_size, SEEK_SET);
        fread(&buf, sizeof(buf), 1, fp);
    }

    if (save.sram_size && cartridge_has_sram(&(nes->cart)))
    {
        fseek(fp, save.head_size + save.neso_size, SEEK_SET);
        fread(nes->cart.buf_sram, 0x2000, 1, fp);
    }

    if (save.cram_size && nes->cart.ischrram)
    {
        fseek(fp, save.head_size + save.neso_size + save.sram_size, SEEK_SET);
        fread(nes->cart.buf_crxm, nes->cart.crom_count * 0x2000, 1, fp);
    }

    // close save file
    fclose(fp);

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    // get ppu & apu old/new pclk
    oldapupclk = nes->apu.pclk_frame;
    newapupclk = buf. apu.pclk_frame;
    oldppupclk = nes->ppu.pclk_frame;
    newppupclk = buf. ppu.pclk_frame;

    //++ restore nes context data from save file data ++//
    i = start = 0;
    do {
        end    = plist[i][0];
        memcpy((BYTE*)nes + start, (BYTE*)&buf + start, end - start);
        start  = end;
        start += plist[i][1];
    } while (plist[i++][1]);
    //-- restore nes context data from save file data --//

    // restore ppu & mmc
    saver_restore_apu(&(nes->apu), oldapupclk, newapupclk);
    saver_restore_ppu(&(nes->ppu), oldppupclk, newppupclk);
    saver_restore_mmc(&(nes->mmc));

    // resume running
    if (running) nes_setrun(nes, 1);
}


