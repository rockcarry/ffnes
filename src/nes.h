#ifndef _NES_NES_H_
#define _NES_NES_H_

// 包含头文件
#include "stdefine.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "mmc.h"
#include "bus.h"
#include "mem.h"
#include "ndb.h"
#include "cartridge.h"
#include "joypad.h"

// 常量定义
// clock defines
#define NES_FREQ_MCLK   21477272         // mclk freq
#define NES_FREQ_CPU   (FREQ_MCLK / 12)  // cpu freq
#define NES_FREQ_PPU   (FREQ_MCLK / 4 )  // ppu freq
#define NES_HTOTAL      341              // htotal
#define NES_VTOTAL      262              // vtotal
#define NES_FRAMERATE  (FREQ_PPU / (NTSC_HTOTAL*NTSC_VTOTAL))  // frame rate

// memory size defines
#define NES_CRAM_SIZE     0x0800
#define NES_PPUREGS_SIZE  0x0008
#define NES_APUREGS_SIZE  0x0016
#define NES_PADREGS_SIZE  0x000A
#define NES_EROM_SIZE     0x1FE0
#define NES_SRAM_SIZE     0x2000
#define NES_PRGROM_SIZE   0x4000

#define NES_CHRROM_SIZE   0x2000
#define NES_VRAM_SIZE     0x0400
#define NES_PALETTE_SIZE  0x0020

// nes max bus size
#define NES_MAX_BUS_SIZE  10

// 类型定义
typedef struct {
    CPU cpu;  // 6502
    PPU ppu;  // 2c02
    APU apu;  // 2a03
    MMC mmc;  // mmc
    NDB ndb;  // ndb
    CARTRIDGE cart;
    JOYPAD    pad ;
    DWORD     extra;

    // cpu bus
    BUSITEM cbus[NES_MAX_BUS_SIZE];
    MEM cram;    // cpu ram
    MEM ppuregs; // ppu regs
    MEM apuregs; // apu regs
    MEM padregs; // pad regs
    MEM erom;    // expansion rom
    MEM sram;    // sram
    MEM prom0;   // PRG-ROM 0
    MEM prom1;   // PRG-ROM 1

    // ppu bus
    BUSITEM pbus[NES_MAX_BUS_SIZE];
    MEM chrrom ; // CHR-ROM 8KB, int cart
    MEM vram[4]; // vram0 1KB * 4 vram, in ppu/cart
    MEM palette; // color palette in ppu

    BYTE buf_erom   [NES_EROM_SIZE]; // erom buffer on cbus
    BYTE buf_vram[4][NES_VRAM_SIZE]; // vram buffer on pbus

    // for reset nes
    int request_reset;
    int isrunning;

    // nes thread
    HANDLE hNesThread;
    HANDLE hNesEvent;
    BOOL   bExitThread;
} NES;

// 函数声明
BOOL nes_init (NES *nes, char *file, DWORD extra);
void nes_free (NES *nes);
void nes_reset(NES *nes);
void nes_run  (NES *nes);
void nes_pause(NES *nes);

#endif







