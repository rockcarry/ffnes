// 包含头文件
#include "nes.h"
#include "log.h"

// 内部函数实现
static DWORD WINAPI nes_thread_proc(LPVOID lpParam)
{
    NES *nes = (NES*)lpParam;
    DWORD dwTickLast  = 0;
    DWORD dwTickCur   = 0;
    DWORD dwTickDiff  = 0;
    DWORD dwTickSleep = 0;
    int   totalpclk;
    int   torunpclk;

    while (1)
    {
        WaitForSingleObject(nes->hNesEvent, -1);
        if (nes->bExitThread) break;

        totalpclk = NES_HTOTAL * NES_VTOTAL;
        do {
            torunpclk  = (totalpclk < 16) ? totalpclk : 16;
            totalpclk -= torunpclk;

            apu_run_pclk(&(nes->apu), torunpclk);
            cpu_run_pclk(&(nes->cpu), torunpclk);
            ppu_run_pclk(&(nes->ppu), torunpclk);
            cpu_nmi(&(nes->cpu), nes->ppu.pin_vbl);
        } while (totalpclk > 0);

        //++ framerate control ++//
        dwTickCur  = GetTickCount();
        dwTickDiff = dwTickCur - dwTickLast;
        dwTickLast = dwTickCur;
        if (dwTickDiff > 16) {
            if (dwTickSleep > 0) dwTickSleep--;
        }
        else if (dwTickDiff < 16) {
            dwTickSleep++;
        }
        if (dwTickSleep) Sleep(dwTickSleep);
        //-- framerate control --//

//      log_printf("%d, %d\n", dwTickDiff, dwTickSleep);
    }
    return 0;
}

// 函数实现
BOOL nes_init(NES *nes, char *file, DWORD extra)
{
    int *mirroring = NULL;
    int  i         = 0;

    log_init("DEBUGER"); // log init

    // clear it
    memset(nes, 0, sizeof(NES));

    // extra data
    nes->extra = extra;

    // load cartridge first
    if (!cartridge_load(&(nes->cart), file)) {
        log_printf("failed to load nes rom file !");
        return FALSE;
    }

    //++ cbus mem map ++//
    // create cpu ram
    nes->cram.type = MEM_RAM;
    nes->cram.size = NES_CRAM_SIZE;
    nes->cram.data = nes->cpu.cram;

    // create ppu regs
    nes->ppuregs.type = MEM_REG;
    nes->ppuregs.size = NES_PPUREGS_SIZE;
    nes->ppuregs.data = nes->ppu.regs;
    nes->ppuregs.r_callback = NES_PPU_REG_RCB;
    nes->ppuregs.w_callback = NES_PPU_REG_WCB;

    // create apu regs
    nes->apuregs.type = MEM_REG;
    nes->apuregs.size = NES_APUREGS_SIZE;
    nes->apuregs.data = nes->apu.regs;
    nes->apuregs.r_callback = NES_APU_REG_RCB;
    nes->apuregs.w_callback = NES_APU_REG_WCB;

    // create pad regs
    nes->padregs.type = MEM_REG;
    nes->padregs.size = NES_PADREGS_SIZE;
    nes->padregs.data = nes->pad.regs;
    nes->padregs.r_callback = NES_PAD_REG_RCB;
    nes->padregs.w_callback = NES_PAD_REG_WCB;

    // create expansion rom
    nes->erom.type = MEM_ROM;
    nes->erom.size = NES_EROM_SIZE;
    nes->erom.data = nes->buf_erom;

    // create sram
    nes->sram.type = MEM_RAM;
    nes->sram.size = NES_SRAM_SIZE;
    nes->sram.data = nes->cart.buf_sram;

    // create PRG-ROM 0
    nes->prom0.type = MEM_ROM;
    nes->prom0.size = NES_PRGROM_SIZE;
    nes->prom0.data = nes->cart.buf_prom;

    // create PRG-ROM 1
    nes->prom1.type = MEM_ROM;
    nes->prom1.size = NES_PRGROM_SIZE;
    nes->prom1.data = nes->cart.buf_prom;
    if (nes->cart.prom_count > 1) {
        nes->prom1.data += NES_PRGROM_SIZE;
    }

    // init nes cbus
    bus_setmem(nes->cbus, 0, 0x0000, 0x1FFF, &(nes->cram   ));
    bus_setmem(nes->cbus, 1, 0x2000, 0x3FFF, &(nes->ppuregs));
    bus_setmem(nes->cbus, 2, 0x4000, 0x4015, &(nes->apuregs));
    bus_setmem(nes->cbus, 3, 0x4016, 0x401F, &(nes->padregs));
    bus_setmem(nes->cbus, 4, 0x4020, 0x5FFF, &(nes->erom   ));
    bus_setmem(nes->cbus, 5, 0x6000, 0x7FFF, &(nes->sram   ));
    bus_setmem(nes->cbus, 6, 0x8000, 0xBFFF, &(nes->prom0  ));
    bus_setmem(nes->cbus, 7, 0xC000, 0xFFFF, &(nes->prom1  ));
    bus_setmem(nes->cbus, 8, 0x0000, 0x0000, NULL           );
    //-- cbus mem map --//

    //++ pbus mem map ++//
    // create CHR-ROM
    nes->chrrom.type = MEM_ROM;
    nes->chrrom.size = NES_CHRROM_SIZE;
    nes->chrrom.data = nes->cart.buf_crom ;

    // create vram
    mirroring = cartridge_get_vram_mirroring(&(nes->cart));
    for (i=0; i<4; i++)
    {
        nes->vram[i].type = MEM_RAM;
        nes->vram[i].size = NES_VRAM_SIZE;
        nes->vram[i].data = nes->buf_vram[mirroring[i]];
    }

    // create color palette
    nes->palette.type = MEM_RAM;
    nes->palette.size = NES_PALETTE_SIZE;
    nes->palette.data = nes->ppu.palette;

    // init nes pbus
    bus_setmir(nes->pbus, 0, 0x3000, 0x3EFF, 0x2FFF);
    bus_setmir(nes->pbus, 1, 0x3F00, 0x3FFF, 0x3F1F);

    bus_setmem(nes->pbus, 2, 0x0000, 0x1FFF, &(nes->chrrom ));
    bus_setmem(nes->pbus, 3, 0x2000, 0x23FF, &(nes->vram[0]));
    bus_setmem(nes->pbus, 4, 0x2400, 0x27FF, &(nes->vram[1]));
    bus_setmem(nes->pbus, 5, 0x2800, 0x2BFF, &(nes->vram[2]));
    bus_setmem(nes->pbus, 6, 0x2C00, 0x2FFF, &(nes->vram[3]));

    // palette
    // 0x3F00 - 0x3F0F: image palette
    // 0x3F10 - 0x3F1F: sprite palette
    // 0x3F00, 0x3F04, 0x3F08, 0x3F0C, 0x3F10, 0x3F14, 0x3F18, 0x3F1C mirring and store the background color
    bus_setmem(nes->pbus, 7, 0x3F00, 0x3F1F, &(nes->palette));
    bus_setmem(nes->pbus, 8, 0x0000, 0x0000, NULL           );
    //-- pbus mem map --//

    // init mmc before cpu & ppu & apu, due to mmc will do bank switch
    // this will change memory mapping on cbus & pbus
    mmc_init(&(nes->mmc), &(nes->cart), nes->cbus, nes->pbus);

    // now it's time to init cpu & ppu & apu
    cpu_init(&(nes->cpu), nes->cbus );
    ppu_init(&(nes->ppu), nes->extra);
    apu_init(&(nes->apu), nes->extra);

    // init joypad
    joypad_init  (&(nes->pad));
    joypad_setkey(&(nes->pad), 0, NES_PAD_CONNECT, 1);
    joypad_setkey(&(nes->pad), 1, NES_PAD_CONNECT, 1);

    // create nes event & thread
    nes->hNesEvent  = CreateEvent (NULL, TRUE, FALSE, NULL);
    nes->hNesThread = CreateThread(NULL, 0, nes_thread_proc, (LPVOID)nes, 0, 0);
    return TRUE;
}

void nes_free(NES *nes)
{
    // destroy nes event & thread
    nes->bExitThread = TRUE;
    SetEvent(nes->hNesEvent);
    WaitForSingleObject(nes->hNesThread, -1);
    CloseHandle(nes->hNesEvent );
    CloseHandle(nes->hNesThread);

    // free joypad
    joypad_setkey(&(nes->pad), 0, NES_PAD_CONNECT, 0);
    joypad_setkey(&(nes->pad), 1, NES_PAD_CONNECT, 0);
    joypad_free  (&(nes->pad ));

    // free cpu & ppu & apu & mmc
    cpu_free(&(nes->cpu));
    ppu_free(&(nes->ppu));
    apu_free(&(nes->apu));
    mmc_free(&(nes->mmc));

    // free cartridge
    cartridge_free(&(nes->cart));

    log_done(); // log done
}

void nes_reset(NES *nes)
{
    joypad_reset(&(nes->pad));

    // mmc need reset first
    mmc_reset(&(nes->mmc));

    // reset cpu & ppu & apu
    cpu_reset(&(nes->cpu));
    ppu_reset(&(nes->ppu));
    apu_reset(&(nes->apu));

    // reset joypad
    joypad_reset(&(nes->pad));
}

void nes_run  (NES *nes) {   SetEvent(nes->hNesEvent); }
void nes_pause(NES *nes) { ResetEvent(nes->hNesEvent); }

