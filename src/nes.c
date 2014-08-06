// 包含头文件
#include "nes.h"
#include "log.h"

// 内部常量定义
#define NES_FREQ_MCLK   21477272
#define NES_FREQ_CPU   (FREQ_MCLK / 12)
#define NES_FREQ_PPU   (FREQ_MCLK / 4 )
#define NES_HTOTAL      341
#define NES_VTOTAL      262
#define NES_FRAMERATE  (FREQ_PPU / (NTSC_HTOTAL*NTSC_VTOTAL))

// 内部函数实现
static DWORD WINAPI nes_thread_proc(LPVOID lpParam)
{
    NES *nes = (NES*)lpParam;
    DWORD dwTickLast  = 0;
    DWORD dwTickCur   = 0;
    DWORD dwTickDiff  = 0;
    DWORD dwTickSleep = 0;
    int   scanline    = 0;
    int   clkremain   = 0;
    int   clkcpurun   = 0;

    while (1)
    {
        WaitForSingleObject(nes->hNesEvent, -1);
        if (nes->bExitThread) break;

        // run cpu & ppu
        for (scanline=0; scanline<NES_VTOTAL; scanline++)
        {
            //++ 更精确的 cpu & ppu 时钟控制 ++//
            #define CPUCLK_PER_SCANLINE_ROUGH  (NES_HTOTAL / 3)
            #define CPUCLK_PER_SCANLINE_PDIFF  (NES_HTOTAL - NES_HTOTAL / 3 * 3)
            clkcpurun  = CPUCLK_PER_SCANLINE_ROUGH;
            clkremain += CPUCLK_PER_SCANLINE_PDIFF;
            if (clkremain >= 3) {
                clkremain -= 3;
                clkcpurun++;
            }
            //-- 更精确的 cpu & ppu 时钟控制 --//

            ppu_run(&(nes->ppu), scanline );
            cpu_run(&(nes->cpu), clkcpurun);
            cpu_nmi(&(nes->cpu), nes->ppu.pin_vbl);
        }

        // apu render frame
        apu_render_frame(&(nes->apu));

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
        log_printf("%d, %d\n", dwTickDiff, dwTickSleep);
    }
    return 0;
}

// 函数实现
BOOL nes_init(NES *nes, char *file, DWORD extra)
{
    log_init("DEBUGER");

    // clear it
    memset(nes, 0, sizeof(NES));

    // extra data
    nes->extra = extra;

    // load cartridge first
    if (cartridge_load(&(nes->cart), file)) {
        // sram
        nes->buf_sram = nes->cart.buf_sram;

        // prom
        if (nes->cart.prom_count <= 1) {
            nes->buf_prgrom0 = nes->cart.buf_prom;
            nes->buf_prgrom1 = nes->cart.buf_prom;
        }
        else {
            nes->buf_prgrom0 = nes->cart.buf_prom;
            nes->buf_prgrom1 = nes->cart.buf_prom + NES_PRGROM0_SIZE;
        }

        // crom
        if (nes->cart.crom_count <= 1) {
            nes->buf_pattab0 = nes->cart.buf_crom;
            nes->buf_pattab1 = nes->cart.buf_crom;
        }
        else {
            nes->buf_pattab0 = nes->cart.buf_crom;
            nes->buf_pattab1 = nes->cart.buf_crom + NES_PRGROM0_SIZE;
        }
    }
    else return FALSE;

    //++ cbus mem map ++//
    // create cpu ram
    nes->cram.type = MEM_RAM;
    nes->cram.size = NES_CRAM_SIZE;
    nes->cram.data = nes->buf_cram;

    // create ppu regs
    nes->ppuregs.type = MEM_REG;
    nes->ppuregs.size = NES_PPUREGS_SIZE;
    nes->ppuregs.data = nes->buf_ppuregs;
    nes->ppuregs.r_callback = NES_PPU_REG_RCB;
    nes->ppuregs.w_callback = NES_PPU_REG_WCB;

    // create apu regs
    nes->apuregs.type = MEM_REG;
    nes->apuregs.size = NES_APUREGS_SIZE;
    nes->apuregs.data = nes->buf_apuregs;
    nes->apuregs.r_callback = NES_APU_REG_RCB;
    nes->apuregs.w_callback = NES_APU_REG_WCB;

    // create pad regs
    nes->padregs.type = MEM_REG;
    nes->padregs.size = NES_PADREGS_SIZE;
    nes->padregs.data = nes->buf_padregs;
    nes->padregs.r_callback = NES_PAD_REG_RCB;
    nes->padregs.w_callback = NES_PAD_REG_WCB;

    // create expansion rom
    nes->erom.type = MEM_ROM;
    nes->erom.size = NES_EROM_SIZE;
    nes->erom.data = nes->buf_erom;

    // create sram
    nes->sram.type = MEM_RAM;
    nes->sram.size = NES_SRAM_SIZE;
    nes->sram.data = nes->buf_sram;

    // create PRG-ROM 0
    nes->prgrom0.type = MEM_ROM;
    nes->prgrom0.size = NES_PRGROM0_SIZE;
    nes->prgrom0.data = nes->buf_prgrom0;

    // create PRG-ROM 1
    nes->prgrom1.type = MEM_ROM;
    nes->prgrom1.size = NES_PRGROM1_SIZE;
    nes->prgrom1.data = nes->buf_prgrom1;

    // init nes cbus
    bus_setmem(nes->cbus, 0, 0x0000, 0x1FFF, &(nes->cram   ));
    bus_setmem(nes->cbus, 1, 0x2000, 0x3FFF, &(nes->ppuregs));
    bus_setmem(nes->cbus, 2, 0x4000, 0x4015, &(nes->apuregs));
    bus_setmem(nes->cbus, 3, 0x4016, 0x401F, &(nes->padregs));
    bus_setmem(nes->cbus, 4, 0x4020, 0x5FFF, &(nes->erom   ));
    bus_setmem(nes->cbus, 5, 0x6000, 0x7FFF, &(nes->sram   ));
    bus_setmem(nes->cbus, 6, 0x8000, 0xBFFF, &(nes->prgrom0));
    bus_setmem(nes->cbus, 7, 0xC000, 0xFFFF, &(nes->prgrom1));
    bus_setmem(nes->cbus, 8, 0x0000, 0x0000, NULL           );
    //-- cbus mem map --//

    //++ pbus mem map ++//
    // create pattern table #0
    nes->pattab0.type = MEM_ROM;
    nes->pattab0.size = NES_PATTAB0_SIZE;
    nes->pattab0.data = nes->buf_pattab0;

    // create pattern table #1
    nes->pattab1.type = MEM_ROM;
    nes->pattab1.size = NES_PATTAB1_SIZE;
    nes->pattab1.data = nes->buf_pattab1;

    if (cartridge_has_4screen(&(nes->cart))) {
        // create vram0
        nes->vram0.type = MEM_RAM;
        nes->vram0.size = NES_VRAM0_SIZE;
        nes->vram0.data = nes->buf_vram0;

        // create vram1
        nes->vram1.type = MEM_RAM;
        nes->vram1.size = NES_VRAM1_SIZE;
        nes->vram1.data = nes->buf_vram1;

        // create vram2
        nes->vram2.type = MEM_RAM;
        nes->vram2.size = NES_VRAM2_SIZE;
        nes->vram2.data = nes->buf_vram2;

        // create vram3
        nes->vram3.type = MEM_RAM;
        nes->vram3.size = NES_VRAM3_SIZE;
        nes->vram3.data = nes->buf_vram3;
    }
    else {
        if (cartridge_get_hvmirroring(&(nes->cart))) {
            // create vram0
            nes->vram0.type = MEM_RAM;
            nes->vram0.size = NES_VRAM0_SIZE;
            nes->vram0.data = nes->buf_vram0;

            // create vram1
            nes->vram1.type = MEM_RAM;
            nes->vram1.size = NES_VRAM1_SIZE;
            nes->vram1.data = nes->buf_vram0;

            // create vram2
            nes->vram2.type = MEM_RAM;
            nes->vram2.size = NES_VRAM2_SIZE;
            nes->vram2.data = nes->buf_vram1;

            // create vram3
            nes->vram3.type = MEM_RAM;
            nes->vram3.size = NES_VRAM3_SIZE;
            nes->vram3.data = nes->buf_vram1;
        }
        else {
            // create vram0
            nes->vram0.type = MEM_RAM;
            nes->vram0.size = NES_VRAM0_SIZE;
            nes->vram0.data = nes->buf_vram0;

            // create vram1
            nes->vram1.type = MEM_RAM;
            nes->vram1.size = NES_VRAM1_SIZE;
            nes->vram1.data = nes->buf_vram1;

            // create vram2
            nes->vram2.type = MEM_RAM;
            nes->vram2.size = NES_VRAM2_SIZE;
            nes->vram2.data = nes->buf_vram0;

            // create vram3
            nes->vram3.type = MEM_RAM;
            nes->vram3.size = NES_VRAM3_SIZE;
            nes->vram3.data = nes->buf_vram1;
        }
    }

    // create color palette
    nes->palette.type = MEM_RAM;
    nes->palette.size = NES_PALETTE_SIZE;
    nes->palette.data = nes->buf_palette;

    // init nes pbus
    bus_setmir(nes->pbus, 0, 0x4000, 0xFFFF, 0x3FFF);
    bus_setmir(nes->pbus, 1, 0x3000, 0x3EFF, 0x2EFF);
    bus_setmir(nes->pbus, 2, 0x3F00, 0x3FFF, 0x3F1F);

    bus_setmem(nes->pbus, 3, 0x0000, 0x0FFF, &(nes->pattab0));
    bus_setmem(nes->pbus, 4, 0x1000, 0x1FFF, &(nes->pattab1));
    bus_setmem(nes->pbus, 5, 0x2000, 0x23FF, &(nes->vram0  ));
    bus_setmem(nes->pbus, 6, 0x2400, 0x27FF, &(nes->vram1  ));
    bus_setmem(nes->pbus, 7, 0x2800, 0x2BFF, &(nes->vram2  ));
    bus_setmem(nes->pbus, 8, 0x2C00, 0x2FFF, &(nes->vram3  ));

    // palette
    // 0x3F00 - 0x3F0F: image palette
    // 0x3F10 - 0x3F1F: sprite palette
    // 0x3F00, 0x3F04, 0x3F08, 0x3F0C, 0x3F10, 0x3F14, 0x3F18, 0x3F1C mirring and store the background color
    bus_setmem(nes->pbus, 9, 0x3F00, 0x3F1F, &(nes->palette));
    //-- pbus mem map --//

    joypad_init(&(nes->pad));
    mmc_init   (&(nes->mmc), &(nes->cart), nes->cbus, nes->pbus);
    cpu_init   (&(nes->cpu), nes->cbus );
    ppu_init   (&(nes->ppu), nes->extra);
    apu_init   (&(nes->apu), nes->extra);

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

    ppu_free      (&(nes->ppu));
    apu_free      (&(nes->apu));
    cpu_free      (&(nes->cpu));
    mmc_free      (&(nes->mmc));
    joypad_free   (&(nes->pad )); // free joypad
    cartridge_free(&(nes->cart)); // free cartridge

    log_done();
}

void nes_reset(NES *nes)
{
    joypad_reset(&(nes->pad));
    mmc_reset   (&(nes->mmc));
    cpu_reset   (&(nes->cpu));
    ppu_reset   (&(nes->ppu));
    apu_reset   (&(nes->apu));
}

void nes_run  (NES *nes) {   SetEvent(nes->hNesEvent); }
void nes_pause(NES *nes) { ResetEvent(nes->hNesEvent); }

