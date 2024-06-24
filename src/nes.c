#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "nes.h"

static void nes_do_reset(NES *nes)
{
    // mmc need reset first
    mmc_reset(&nes->mmc);

    // reset cpu & ppu & apu
    cpu_reset(&nes->cpu);
    ppu_reset(&nes->ppu);
    apu_reset(&nes->apu);

    // reset joypad
    joypad_reset(&nes->pad);
}

static void* nes_thread_proc(void *param)
{
    NES *nes = (NES*)param;
    while (!(nes->thread_status & TS_EXIT)) {
        //++ for run/pause and end of replay ++//
        if (nes->thread_status & TS_PAUSE_REQ) { // request pause
            nes->thread_status |= TS_PAUSE_ACK;
            usleep(20 * 1000); continue;
        }
        //-- for run/pause and end of replay --//

        //++ for nes reset ++//
        if (nes->thread_status & TS_RESET) {
            nes->thread_status &= ~TS_RESET;
            nes_do_reset(nes);
        }
        //-- for nes reset --//

        //++ run cpu & apu & ppu
        for (int i = 0; i < NES_HTOTAL * NES_VTOTAL; i += 3) {
            apu_run(&nes->apu); // apu run 1 pclk
            apu_run(&nes->apu); // apu run 1 pclk
            apu_run(&nes->apu); // apu run 1 pclk
            ppu_run(&nes->ppu); // ppu run 1 pclk
            ppu_run(&nes->ppu); // ppu run 1 pclk
            ppu_run(&nes->ppu); // ppu run 1 pclk
            cpu_nmi(&nes->cpu, ppu_pin_vbl(&nes->ppu));
            cpu_irq(&nes->cpu, apu_pin_irq(&nes->apu));
            cpu_run(&nes->cpu); // cpu run 1 cclk
        }
        //-- run cpu & apu & ppu

        //++ run joypad
        uint32_t keystate = 0;
        nes->callback(nes->cbctx, 2, (uint8_t*)&keystate, sizeof(keystate)); joypad_setkey(&nes->pad, 0, NES_KEY_ALL, keystate);
        nes->callback(nes->cbctx, 3, (uint8_t*)&keystate, sizeof(keystate)); joypad_setkey(&nes->pad, 1, NES_KEY_ALL, keystate);
        nes->callback(nes->cbctx, 4, (uint8_t*)&keystate, sizeof(keystate)); joypad_setkey(&nes->pad, 2, NES_KEY_ALL, keystate);
        nes->callback(nes->cbctx, 5, (uint8_t*)&keystate, sizeof(keystate)); joypad_setkey(&nes->pad, 3, NES_KEY_ALL, keystate);
        joypad_run(&nes->pad);
        //-- run joypad
    }
    return NULL;
}

int nes_init(NES *nes, char *file, NES_RENDER_CB callback, void *cbctxt)
{
    int *mirroring = NULL;
    int  i         = 0;

    // clear it
    memset(nes, 0, sizeof(NES));

    nes->thread_status = TS_PAUSE_REQ;
    nes->callback      = callback;
    nes->cbctx         = cbctxt;

    // load cartridge first
    if (cartridge_load(&nes->cart, file) != 0) {
        printf("failed to load nes rom file !\n");
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

    // create sram
    nes->sram.type = MEM_RAM;
    nes->sram.size = NES_SRAM_SIZE;
    nes->sram.data = nes->cart.buf_sram;

    // create PRG-ROM 0
    nes->prom0.type = MEM_ROM;
    nes->prom0.size = NES_PRGROM_SIZE;

    // create PRG-ROM 1
    nes->prom1.type = MEM_ROM;
    nes->prom1.size = NES_PRGROM_SIZE;

    // init nes cbus
    bus_setmem(nes->cbus, 0, 0xC000, 0xFFFF, &nes->prom1  );
    bus_setmem(nes->cbus, 1, 0x8000, 0xBFFF, &nes->prom0  );
    bus_setmem(nes->cbus, 2, 0x6000, 0x7FFF, &nes->sram   );
    bus_setmem(nes->cbus, 3, 0x4000, 0x5FFF, &nes->apuregs);
    bus_setmem(nes->cbus, 4, 0x2000, 0x3FFF, &nes->ppuregs);
    bus_setmem(nes->cbus, 5, 0x0000, 0x1FFF, &nes->cram   );
    //-- cbus mem map --//

    //++ pbus mem map ++//
    // create CHR-ROM0
    nes->chrrom0.type = MEM_ROM;
    nes->chrrom0.size = NES_CHRROM_SIZE;

    // create CHR-ROM1
    nes->chrrom1.type = MEM_ROM;
    nes->chrrom1.size = NES_CHRROM_SIZE;

    // create vram
    mirroring = cartridge_get_vram_mirroring(&nes->cart);
    for (i = 0; i < 4; i++) {
        nes->vram[i].type = MEM_RAM;
        nes->vram[i].size = NES_VRAM_SIZE;
        nes->vram[i].data = nes->buf_vram[mirroring[i]];
    }

    // create color palette
    nes->palette.type = MEM_RAM;
    nes->palette.size = NES_PALETTE_SIZE;
    nes->palette.data = nes->ppu.palette;

    // init nes pbus
    bus_setmem(nes->pbus, 0, 0x3F00, 0x3FFF, &nes->palette);
    bus_setmir(nes->pbus, 1, 0x3000, 0x3EFF, 0x2FFF       );
    bus_setmem(nes->pbus, 2, 0x2C00, 0x2FFF, &nes->vram[3]);
    bus_setmem(nes->pbus, 3, 0x2800, 0x2BFF, &nes->vram[2]);
    bus_setmem(nes->pbus, 4, 0x2400, 0x27FF, &nes->vram[1]);
    bus_setmem(nes->pbus, 5, 0x2000, 0x23FF, &nes->vram[0]);
    bus_setmem(nes->pbus, 6, 0x1000, 0x1FFF, &nes->chrrom1);
    bus_setmem(nes->pbus, 7, 0x0000, 0x0FFF, &nes->chrrom0);
    //-- pbus mem map --//

    // init mmc before cpu & ppu & apu, due to mmc will do bank switch
    // this will change memory mapping on cbus & pbus
    mmc_init(&nes->mmc, &nes->cart, nes->cbus, nes->pbus);

    // now it's time to init cpu & ppu & apu
    cpu_init(&nes->cpu, nes->cbus);
    ppu_init(&nes->ppu, callback, cbctxt);
    apu_init(&nes->apu, callback, cbctxt);

    // init joypad
    joypad_init  (&nes->pad);
    joypad_setkey(&nes->pad, 0, NES_PAD_CONNECT, 1);
    joypad_setkey(&nes->pad, 1, NES_PAD_CONNECT, 1);

    // create nes event & thread
    pthread_create(&nes->thread_id, NULL, nes_thread_proc, nes);
    return 0;
}

void nes_free(NES *nes)
{
    // destroy nes thread
    nes->thread_status |= TS_EXIT;
    pthread_join(nes->thread_id, NULL);

    // free joypad
    joypad_setkey(&nes->pad, 0, NES_PAD_CONNECT, 0);
    joypad_setkey(&nes->pad, 1, NES_PAD_CONNECT, 0);
    joypad_free  (&nes->pad);

    // free cpu & ppu & apu & mmc
    cpu_free(&nes->cpu);
    ppu_free(&nes->ppu);
    apu_free(&nes->apu);
    mmc_free(&nes->mmc);

    // free cartridge
    cartridge_free(&nes->cart);
}

void nes_reset(NES *nes)
{
    nes->thread_status |= TS_RESET; // request reset
}

void nes_setrun(NES *nes, int run)
{
    if (run) {
        nes->thread_status &= ~TS_PAUSE_ACK;
        nes->thread_status &= ~TS_PAUSE_REQ;
    } else {
        nes->thread_status &= ~TS_PAUSE_ACK;
        nes->thread_status |=  TS_PAUSE_REQ;
        while (!(nes->thread_status & (TS_PAUSE_ACK|TS_RESET))) usleep(20 * 1000);
    }
}

int nes_getrun(NES *nes)
{
    return !(nes->thread_status & TS_PAUSE_REQ);
}
