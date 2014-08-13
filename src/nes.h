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
#include "cartridge.h"
#include "joypad.h"

#define NES_CRAM_SIZE     0x0800
#define NES_PPUREGS_SIZE  0x0008
#define NES_APUREGS_SIZE  0x0016
#define NES_PADREGS_SIZE  0x000A
#define NES_EROM_SIZE     0x1FE0
#define NES_SRAM_SIZE     0x2000
#define NES_PRGROM0_SIZE  0x4000
#define NES_PRGROM1_SIZE  0x4000

#define NES_PATTAB0_SIZE  0x1000
#define NES_PATTAB1_SIZE  0x1000
#define NES_VRAM_SIZE     0x0400
#define NES_PALETTE_SIZE  0x0020

// 类型定义
typedef struct {
    CPU cpu;  // 6502
    PPU ppu;  // 2c02
    APU apu;  // 2a03
    MMC mmc;  // mmc
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
    MEM prgrom0; // PRG-ROM 0
    MEM prgrom1; // PRG-ROM 1

    // ppu bus
    BUSITEM pbus[NES_MAX_BUS_SIZE];
    MEM pattab0; // pattern table #0
    MEM pattab1; // pattern table #1
    MEM vram[4]; // vram0 1KB * 4 vram, in ppu/cart
    MEM palette; // color palette in ppu

    BYTE buf_erom   [NES_EROM_SIZE]; // erom buffer on cbus
    BYTE buf_vram[4][NES_VRAM_SIZE]; // vram buffer on pbus

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







