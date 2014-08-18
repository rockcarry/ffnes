#ifndef _NES_PPU_H_
#define _NES_PPU_H_

// 包含头文件
#include "stdefine.h"
#include "mem.h"

// 类型定义
typedef struct {
    BYTE  regs   [8  ]; // ppu regs
    BYTE  sprram [256]; // sprite ram
    BYTE  palette[32 ]; // palette

// private:
    void *vdevctxt;
    int   pin_vbl;
    int   toggle;
    WORD  vaddr;
    WORD  temp0;
    WORD  temp1;
    int   color_flags;
    BYTE *chrom_bkg;
    BYTE *chrom_spr;

    //++ for step pclk rendering ++//
    int   scanline;   // current scanline
    int   pclk_frame; // current pclk cycles in one frame
    int   pclk_line;  // current pclk cycles in one line
    BYTE *draw_buffer;
    int   draw_stride;
    BYTE  cdatal;
    BYTE  cdatah;
    BYTE  pixelh;
    //-- for step pclk rendering --//
} PPU;

// 函数声明
void ppu_init    (PPU *ppu, DWORD extra);
void ppu_free    (PPU *ppu);
void ppu_reset   (PPU *ppu);
void ppu_run_pclk(PPU *ppu, int pclk);

BYTE NES_PPU_REG_RCB(MEM *pm, int addr);
void NES_PPU_REG_WCB(MEM *pm, int addr, BYTE byte);

#endif

