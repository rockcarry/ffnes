#ifndef _NES_NES_H_
#define _NES_NES_H_

#include <pthread.h>
#include "apu.h"
#include "cpu.h"
#include "ppu.h"
#include "bus.h"
#include "mem.h"
#include "mmc.h"
#include "cartridge.h"
#include "joypad.h"

// render callback
typedef void (*NES_RENDER_CB)(void *cbctx, int type, uint8_t *buf, int len);

// clock defines
#define NES_FREQ_MCLK   21477272            // mclk freq
#define NES_FREQ_CPU   (NES_FREQ_MCLK / 12) // cpu freq
#define NES_FREQ_PPU   (NES_FREQ_MCLK / 4 ) // ppu freq
#define NES_FREQ_APU   (NES_FREQ_MCLK / 24) // apu freq

// memory size defines
#define NES_CRAM_SIZE     0x0800
#define NES_PPUREGS_SIZE  0x0008
#define NES_APUREGS_SIZE  0x0020
#define NES_SRAM_SIZE     0x2000
#define NES_PRGROM_SIZE   0x4000

#define NES_CHRROM_SIZE   0x1000
#define NES_VRAM_SIZE     0x0400
#define NES_PALETTE_SIZE  0x0020

// nes max bus size
#define NES_MAX_BUS_SIZE  8

typedef struct {
    CPU cpu;  // 6502
    PPU ppu;  // 2c02
    APU apu;  // 2a03
    MMC mmc;  // mmc
    CARTRIDGE cart;
    JOYPAD    pad ;

    // cpu bus
    BUS cbus[NES_MAX_BUS_SIZE];
    MEM cram;    // cpu ram
    MEM ppuregs; // ppu regs
    MEM apuregs; // apu regs
    MEM sram;    // sram
    MEM prom0;   // PRG-ROM 0
    MEM prom1;   // PRG-ROM 1

    // ppu bus
    BUS pbus[NES_MAX_BUS_SIZE];
    MEM chrrom0; // CHR-ROM 4KB, int cart
    MEM chrrom1; // CHR-ROM 4KB, int cart
    MEM vram[4]; // vram0 1KB * 4 vram, in ppu/cart
    MEM palette; // color palette in ppu

    // for nes thread
    #define TS_EXIT      (1 << 0 )
    #define TS_RESET     (1 << 1 )
    #define TS_PAUSE_REQ (1 << 2 )
    #define TS_PAUSE_ACK (1 << 18)
    pthread_t thread_id;
    int       thread_status;

    uint8_t buf_vram[4][NES_VRAM_SIZE]; // vram buffer on pbus
    NES_RENDER_CB callback;
    void         *cbctx;
} NES;

int  nes_init  (NES *nes, char *file, NES_RENDER_CB callback, void *cbctxt);
void nes_free  (NES *nes);
void nes_reset (NES *nes);
void nes_setrun(NES *nes, int run);
int  nes_getrun(NES *nes);

#endif

