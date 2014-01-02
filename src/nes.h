#ifndef _NES_NES_H_
#define _NES_NES_H_

// 包含头文件
#include "stdefine.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "bus.h"
#include "mem.h"

// 类型定义
typedef struct {
    CPU cpu;  // 6502
    PPU ppu;  // 2c02
    APU apu;  // 2a03

    BUS cbus;    // cpu bus
    MEM cram;    // cpu ram
    MEM ppuregs; // ppu regs
    MEM apuregs; // apu regs
    MEM erom;    // expansion rom
    MEM sram;    // sram
    MEM prgrom0; // PRG-ROM 0
    MEM prgrom1; // PRG-ROM 1

    BUS pbus;    // ppu bus
    MEM pattab0; // pattern table #0
    MEM pattab1; // pattern table #1
    MEM vram0;   // vram0 1KB, in ppu
    MEM vram1;   // vram1 1KB, in ppu
    MEM vram2;   // vram2 1KB, in cart
    MEM vram3;   // vram3 1KB, in cart
    MEM vrammir; // vram mirror
} NES;

// 函数声明
BOOL nes_create (NES *nes);
void nes_destroy(NES *nes);

#endif







