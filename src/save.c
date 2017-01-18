// 包含头文件
#include <stdio.h>
#include "save.h"
#include "adev.h"
#include "vdev.h"
#include "lzw.h"

// 内部类型定义
typedef struct
{
    char  signature[8]; // ffnesave
    DWORD version;      // version
    DWORD head_size;    // head size
    DWORD neso_size;    // nes object size
    DWORD sram_size;    // sram size
    DWORD cram_size;    // cram size
    DWORD rply_size;    // replay size
} NES_SAVE_FILE;

// 内部函数实现
static void saver_restore_apu(APU *apu, int oldpclk, int newpclk)
{
    if (oldpclk > 0) apu->adev->enqueue(apu->actxt);
    if (newpclk > 0) apu->adev->dequeue(apu->actxt, &(apu->audiobuf));
}

static void saver_restore_ppu(PPU *ppu, int oldpclk, int newpclk)
{
    NES *nes = container_of(ppu, NES, ppu);

    ppu->chrom_bkg = (ppu->regs[0x0000] & (1 << 4)) ? nes->chrrom1.data : nes->chrrom0.data;
    ppu->chrom_spr = (ppu->regs[0x0000] & (1 << 3)) ? nes->chrrom1.data : nes->chrrom0.data;
    if (oldpclk <= NES_HTOTAL * 241 + 1) ppu->vdev->enqueue(ppu->vctxt);
    if (newpclk <= NES_HTOTAL * 241 + 1) ppu->vdev->dequeue(ppu->vctxt, (void**)&(ppu->draw_buffer), &(ppu->draw_stride));
}

static void saver_restore_mmc(MMC *mmc)
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
    void    *fp = NULL;
    int      running;
    int      value;

    // open file
    fp = lzw_fopen(file, "wb");
    if (!fp) return;

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    strncpy((char*)&save , "ffnesave", sizeof(save));
    save.version   = 0x00000001; // v0.1
    save.head_size = sizeof(save);
    save.neso_size = sizeof(NES );
    save.sram_size = cartridge_has_sram(&(nes->cart)) ? 0x2000 : 0;
    save.cram_size = nes->cart.ischrram ? nes->cart.crom_count * 0x2000 : 0;
    save.rply_size = ftell(nes->replay.fp);
    save.rply_size = (save.rply_size == -1) ? 0 : save.rply_size;

    if (save.head_size) lzw_fwrite(&save             , sizeof(save)  , 1, fp);
    if (save.neso_size) lzw_fwrite(nes               , sizeof(NES)   , 1, fp);
    if (save.sram_size) lzw_fwrite(nes->cart.buf_sram, save.sram_size, 1, fp);
    if (save.cram_size) lzw_fwrite(nes->cart.buf_crxm, save.cram_size, 1, fp);

    // copy replay data
    if (save.rply_size)
    {
        fseek(nes->replay.fp, 0, SEEK_SET);
        while (1)
        {
            value = fgetc(nes->replay.fp);
            if (value == EOF) break;
            else lzw_fputc(value, fp);
        }
    }

    // resume running
    if (running) nes_setrun(nes, 1);

    // close file
    lzw_fclose(fp);
}

void saver_load_game(NES *nes, char *file)
{
    NES_SAVE_FILE save;
    void         *fp;
    NES           buf;
    int           start, end, i;
    int           oldapuaclk, newapuaclk;
    int           oldppupclk, newppupclk;
    int           running;

    // protected member list
    static int plist[][2] =
    {
        { offsetof(NES, cpu.cbus    ),  sizeof(BUS       ) },
        { offsetof(NES, ppu.vdev    ),  sizeof(VDEV*     ) },
        { offsetof(NES, ppu.vctxt   ),  sizeof(void*     ) },
        { offsetof(NES, apu.adev    ),  sizeof(ADEV*     ) },
        { offsetof(NES, apu.actxt   ),  sizeof(void*     ) },
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
    fp = lzw_fopen(file, "rb");
    if (!fp) return;

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    // read .sav file header
    lzw_fread(&save, sizeof(save), 1, fp);

    if (save.neso_size)
    {
        lzw_fseek(fp, save.head_size, SEEK_SET);
        lzw_fread(&buf, sizeof(buf), 1, fp);
    }

    if (save.sram_size && cartridge_has_sram(&(nes->cart)))
    {
        lzw_fseek(fp, save.head_size + save.neso_size, SEEK_SET);
        lzw_fread(nes->cart.buf_sram, 0x2000, 1, fp);
    }

    if (save.cram_size && nes->cart.ischrram)
    {
        lzw_fseek(fp, save.head_size + save.neso_size + save.sram_size, SEEK_SET);
        lzw_fread(nes->cart.buf_crxm, nes->cart.crom_count * 0x2000, 1, fp);
    }

    // reset replay to record mode
    nes->replay.mode = NES_REPLAY_RECORD;
    replay_reset(&(nes->replay));

    // copy replay data from .sav file to relay temp file
    lzw_fseek(fp, save.head_size + save.neso_size + save.sram_size + save.cram_size, SEEK_SET);
    while (save.rply_size--) fputc(lzw_fgetc(fp), nes->replay.fp);

    // get ppu & apu old/new pclk
    oldapuaclk = nes->apu.aclk_counter;
    newapuaclk = buf. apu.aclk_counter;
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
    saver_restore_apu(&(nes->apu), oldapuaclk, newapuaclk);
    saver_restore_ppu(&(nes->ppu), oldppupclk, newppupclk);
    saver_restore_mmc(&(nes->mmc));

    // show text
    nes_textout(nes, 0, 222, "load save...", 2000, 2);

    // resume running
    if (running) nes_setrun(nes, 1);

    // close save file
    lzw_fclose(fp);
}

void saver_load_replay(NES *nes, char *file)
{
    NES_SAVE_FILE save;
    void         *fp;
    int           running;

    // open save file
    fp = lzw_fopen(file, "rb");
    if (!fp) return;

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    // read .sav file header
    lzw_fread(&save, sizeof(save), 1, fp);

    // reset replay to record mode
    nes->replay.mode = NES_REPLAY_RECORD;
    replay_reset(&(nes->replay));

    // copy replay data from .sav file to relay temp file
    lzw_fseek(fp, save.head_size + save.neso_size + save.sram_size + save.cram_size, SEEK_SET);
    while (save.rply_size--) fputc(lzw_fgetc(fp), nes->replay.fp);

    // reset replay to play mode
    nes->replay.mode = NES_REPLAY_PLAY;
    replay_reset(&(nes->replay));

    // reset nes
    nes_reset(nes);

    // show text
    nes_textout(nes, 0, 222, "replay", -1, 2);

    // resume running
    if (running) nes_setrun(nes, 1);

    // close save file
    lzw_fclose(fp);
}


