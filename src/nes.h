#ifndef _NES_NES_H_
#define _NES_NES_H_

// 包含头文件
#include "stdefine.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "bus.h"
#include "mem.h"
#include "cartridge.h"

#define NES_CRAM_SIZE     0x0800
#define NES_PPUREGS_SIZE  0x0008
#define NES_APUREGS_SIZE  0x0018
#define NES_EROM_SIZE     0x1FE8
#define NES_SRAM_SIZE     0x2000
#define NES_PRGROM0_SIZE  0x4000
#define NES_PRGROM1_SIZE  0x4000

#define NES_PATTAB0_SIZE  0x1000
#define NES_PATTAB1_SIZE  0x1000
#define NES_VRAM0_SIZE    0x0400
#define NES_VRAM1_SIZE    0x0400
#define NES_VRAM2_SIZE    0x0400
#define NES_VRAM3_SIZE    0x0400
#define NES_PALETTE_SIZE  0x0020

// 类型定义
typedef struct {
    CPU cpu;  // 6502
    PPU ppu;  // 2c02
    APU apu;  // 2a03
    CARTRIDGE cart;

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
    MEM palette; // color palette in ppu

    BYTE  buf_cram   [NES_CRAM_SIZE   ];
    BYTE  buf_ppuregs[NES_PPUREGS_SIZE];
    BYTE  buf_apuregs[NES_APUREGS_SIZE];
    BYTE *buf_sram;     // in cartridge
    BYTE  buf_erom   [NES_SRAM_SIZE   ];
    BYTE *buf_prgrom0;  // in cartridge
    BYTE *buf_prgrom1;  // in cartridge

    BYTE *buf_pattab0;  // in cartridge
    BYTE *buf_pattab1;  // in cartridge
    BYTE  buf_vram0  [NES_VRAM0_SIZE  ];
    BYTE  buf_vram1  [NES_VRAM1_SIZE  ];
    BYTE  buf_vram2  [NES_VRAM2_SIZE  ];
    BYTE  buf_vram3  [NES_VRAM3_SIZE  ];
    BYTE  buf_palette[NES_PALETTE_SIZE];
} NES;

// 函数声明
BOOL nes_init(NES *nes, char *file);
void nes_free(NES *nes);

#endif







