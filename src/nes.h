#ifndef _NES_NES_H_
#define _NES_NES_H_

// 包含头文件
#include <pthread.h>
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
#include "replay.h"

// 常量定义
// clock defines
#define NES_FREQ_MCLK   21477272            // mclk freq
#define NES_FREQ_CPU   (NES_FREQ_MCLK / 12) // cpu freq
#define NES_FREQ_PPU   (NES_FREQ_MCLK / 4 ) // ppu freq
#define NES_FREQ_APU   (NES_FREQ_MCLK / 24) // apu freq
#define NES_HTOTAL      341                 // htotal
#define NES_VTOTAL      262                 // vtotal
#define NES_WIDTH       256                 // screen width
#define NES_HEIGHT      240                 // screen height
#define NES_FRAMERATE  (FREQ_PPU / (NES_HTOTAL*NES_VTOTAL))  // frame rate

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

// 类型声明
struct tagNDB; // for ndb

// 类型定义
typedef struct tagNES {
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
    MEM sram;    // sram
    MEM prom0;   // PRG-ROM 0
    MEM prom1;   // PRG-ROM 1

    // ppu bus
    BUSITEM pbus[NES_MAX_BUS_SIZE];
    MEM chrrom0; // CHR-ROM 4KB, int cart
    MEM chrrom1; // CHR-ROM 4KB, int cart
    MEM vram[4]; // vram0 1KB * 4 vram, in ppu/cart
    MEM palette; // color palette in ppu

    // for reset nes
    int request_reset;

    // for nes thread
    pthread_t thread_id;
    BOOL      thread_exit;
    BOOL      isrunning;
    BOOL      ispaused;

    // for replay
    REPLAY replay;

    // ndb object
    struct tagNDB ndb;

    BYTE buf_vram[4][NES_VRAM_SIZE]; // vram buffer on pbus
} NES;

// 函数声明
BOOL nes_init   (NES *nes, char *file, DWORD extra);
void nes_free   (NES *nes);
void nes_reset  (NES *nes);

void nes_setrun (NES *nes, int run);
int  nes_getrun (NES *nes);
void nes_joypad (NES *nes, int pad, int key, int value);
void nes_textout(NES *nes, int x, int y, char *text, int time, int priority);

// for nes save
void nes_save_game  (NES *nes, char *file);
void nes_load_game  (NES *nes, char *file);
void nes_load_replay(NES *nes, char *file);

// fullscreen
int  nes_getfullscreen(NES *nes);
void nes_setfullscreen(NES *nes, int mode);

#endif







