// 包含头文件
#include "lzw.h"
#include "nes.h"
#include "save.h"

// 内部类型定义
typedef struct
{
    char  signature[8]; // ffnesave
    DWORD version;      // version
    DWORD head_size;    // head size
    DWORD neso_size;    // nes object size
    DWORD sram_size;    // sram size
    DWORD cram_size;    // cram size
} NES_SAVE_FILE;

// 内部函数实现
static void restore_apu(APU *apu, int oldaclk, int newaclk)
{
    if (oldaclk > 0) apu->adev->enqueue(apu->actxt);
    if (newaclk > 0) apu->adev->dequeue(apu->actxt, &apu->audiobuf);
}

static void restore_ppu(PPU *ppu)
{
    NES *nes = container_of(ppu, NES, ppu);
    ppu->chrom_bkg = (ppu->regs[0x0000] & (1 << 4)) ? nes->chrrom1.data : nes->chrrom0.data;
    ppu->chrom_spr = (ppu->regs[0x0000] & (1 << 3)) ? nes->chrrom1.data : nes->chrrom0.data;
}

static void restore_mmc(MMC *mmc)
{
    if (mmc->pbanksize == 16 * 1024) {
        mmc_switch_pbank16k0(mmc, mmc->pbank8000);
        mmc_switch_pbank16k1(mmc, mmc->pbankc000);
    }

    if (mmc->pbanksize == 32 * 1024) {
        mmc_switch_pbank32k (mmc, mmc->pbank8000);
        mmc_switch_pbank32k (mmc, mmc->pbankc000);
    }

    if (mmc->cbanksize == 4 * 1024) {
        mmc_switch_cbank4k0 (mmc, mmc->cbank0000);
        mmc_switch_cbank4k1 (mmc, mmc->cbank1000);
    }

    if (mmc->cbanksize == 8 * 1024) {
        mmc_switch_cbank8k  (mmc, mmc->cbank0000);
        mmc_switch_cbank8k  (mmc, mmc->cbank1000);
    }
}

// 函数实现
int nes_save_game(NES *nes, char *file)
{
    NES_SAVE_FILE save;
    void    *fp = NULL;
    int      running;
    int      length;

    // open file
    fp = fopen(file, "wb");
    if (!fp) return 0;

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    strncpy((char*)&save , "ffnesave", sizeof(save));
    save.version   = 0x00000001; // v0.1
    save.head_size = sizeof(save);
    save.neso_size = sizeof(NES );
    save.sram_size = cartridge_has_sram(&nes->cart) ? 0x2000 : 0;
    save.cram_size = nes->cart.ischrram ? nes->cart.crom_count * 0x2000 : 0;
    length         = save.head_size + save.neso_size + save.sram_size + save.cram_size;

    if (save.head_size) fwrite(&save             , sizeof(save)  , 1, fp);
    if (save.neso_size) fwrite(nes               , sizeof(NES )  , 1, fp);
    if (save.sram_size) fwrite(nes->cart.buf_sram, save.sram_size, 1, fp);
    if (save.cram_size) fwrite(nes->cart.buf_crxm, save.cram_size, 1, fp);

    // resume running
    if (running) nes_setrun(nes, 1);

    // close file
    fclose(fp);

    return length;
}

int nes_load_game(NES *nes, char *file)
{
    NES_SAVE_FILE save;
    void         *fp;
    NES           buf;
    int           start, end, i;
    int           oldapuaclk;
    int           newapuaclk;
    int           running;
    int           length;

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
    fp = fopen(file, "rb");
    if (!fp) return 0;

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    // read .sav file header
    fread(&save, sizeof(save), 1, fp);
    length = save.head_size + save.neso_size + save.sram_size + save.cram_size;

    if (save.neso_size) {
        fseek(fp, save.head_size, SEEK_SET);
        fread(&buf, sizeof(buf), 1, fp);
    }

    if (save.sram_size && cartridge_has_sram(&nes->cart)) {
        fseek(fp, save.head_size + save.neso_size, SEEK_SET);
        fread(nes->cart.buf_sram, 0x2000, 1, fp);
    }

    if (save.cram_size && nes->cart.ischrram) {
        fseek(fp, save.head_size + save.neso_size + save.sram_size, SEEK_SET);
        fread(nes->cart.buf_crxm, nes->cart.crom_count * 0x2000, 1, fp);
    }

    // reset replay to record mode
    nes->replay.mode = NES_REPLAY_RECORD;
    replay_reset(&nes->replay);

    // get ppu & apu old/new pclk
    oldapuaclk = nes->apu.aclk_counter;
    newapuaclk = buf. apu.aclk_counter;

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
    restore_apu(&nes->apu, oldapuaclk, newapuaclk);
    restore_ppu(&nes->ppu);
    restore_mmc(&nes->mmc);

    // show text
    nes_textout(nes, 0, 222, "load save...", 2000, 2);

    // resume running
    if (running) nes_setrun(nes, 1);

    // close save file
    fclose(fp);

    return length;
}

void replay_init(REPLAY *rep)
{
    // init replay mode
    strcpy(rep->fname, getenv("TEMP"));
    strcat(rep->fname, "\\lastreplay");
    rep->mode = NES_REPLAY_RECORD;
    replay_reset(rep); // reset replay
}

void replay_free(REPLAY *rep)
{
    if (rep->fp) {
        fclose(rep->fp);
        rep->fp = NULL;
    }
    unlink(rep->fname);
}

void replay_reset(REPLAY *rep)
{
    NES *nes = container_of(rep, NES, replay);
    int  len = 0;

    // close old fp if needed
    if (rep->fp) fclose(rep->fp);

    switch (rep->mode)
    {
    case NES_REPLAY_RECORD:
        len = nes_save_game(nes, rep->fname);
        rep->fp = fopen(rep->fname, "ab+");
        break;
    case NES_REPLAY_PLAY:
        len = nes_load_game(nes, rep->fname);
        rep->fp = fopen(rep->fname, "rb+");
        fseek(rep->fp, 0, SEEK_END);
        rep->total = ftell(rep->fp) - len;
        fseek(rep->fp, len, SEEK_SET);
        break;
    }

    // reset curpos
    rep->curpos = 0;
}

BYTE replay_run(REPLAY *rep, BYTE data)
{
    int value = 0;
    switch (rep->mode)
    {
    case NES_REPLAY_RECORD:
        fputc(data, rep->fp);
        break;
    case NES_REPLAY_PLAY:
        value = fgetc(rep->fp);
        data  = (value == EOF) ? 0 : value;
        break;
    }
    rep->curpos++;
    return data;
}

int replay_isend(REPLAY *rep)
{
    if (rep->mode == NES_REPLAY_RECORD) return 1; // if record mode
    else return (rep->curpos < rep->total);       // if play mode
}

static int copyfile(FILE *fpdst, FILE *fpsrc)
{
    int data = 0;
    int len  = 0;
    if (!fpdst || !fpsrc) return -1;
    while (1) {
        data = fgetc(fpsrc);
        if (data == EOF) break;
        fputc(data, fpdst);
        len++;
    }
    return len;
}

int nes_save_replay(NES *nes, char *file)
{
    FILE *fpdst   = fopen(file, "wb");
    FILE *fpsrc   = nes->replay.fp;
    int   running = 0;
    int   ret     = 0;

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    if (fpsrc) fseek(fpsrc, 0, SEEK_SET);
    ret = copyfile(fpdst, fpsrc);

    // resume running
    if (running) nes_setrun(nes, 1);

    return ret;
}

int nes_load_replay(NES *nes, char *file)
{
    FILE *fpsrc   = fopen(file, "rb");
    FILE *fpdst   = NULL;
    int   running = 0;
    int   ret     = 0;

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    // close lastreplay file
    if (nes->replay.fp) {
        fclose(nes->replay.fp);
        nes->replay.fp = NULL;
    }

    fpdst = fopen(nes->replay.fname, "wb");
    ret   = copyfile(fpdst, fpsrc);

    // set replay to play mode
    nes->replay.mode = NES_REPLAY_PLAY;

    // reset nes
    nes_reset(nes);

    // show text
    nes_textout(nes, 0, 222, "replay", -1, 2);

    // resume running
    if (running) nes_setrun(nes, 1);

    return ret;
}


