#ifndef _NES_PPU_H_
#define _NES_PPU_H_

// 包含头文件
#include "stdefine.h"
#include "mem.h"
#include "vdev.h"

// 类型定义
typedef struct {
    BYTE  regs   [8  ]; // ppu regs
    BYTE  sprram [256]; // sprite ram
    BYTE  palette[32 ]; // palette

// private:
    VDEV *vdev;
    void *vctxt;
    int   pinvbl;
    int   toggle;
    int   finex;
    WORD  vaddr;
    WORD  temp0; // correspond to vaddr
    WORD  temp1; // correspond to finex
    BYTE _2007_lazy;
    BYTE *chrom_bkg;
    BYTE *chrom_spr;

    //++ for ppu open bus ++//
    BYTE  open_busv;
    BYTE  open_bust[8];
    //-- for ppu open bus --//

    //++ for step pclk rendering ++//
    int   pclk_frame; // current pclk cycles in one frame
    int   pclk_line;  // current pclk cycles in one line
    int   oddevenflag;// odd/even frame toggle flag
    int   scanline;   // current scanline
    BYTE  vblklast;   // for vblk flag read suppression

    DWORD*draw_buffer;
    int   draw_stride;
    DWORD tdatal;
    DWORD tdatah;
    DWORD adata;
    //-- for step pclk rendering --//

    //++ sprite buffer for rendering
    BYTE  sprbuf[64]; // sprite buffer
    BYTE  sprnum;     // sprite number
    BYTE *sprzero;    // sprite zero
    //-- sprite buffer for rendering

    // palette for vdev rendering
    BYTE  vdevpal[64*4];
} PPU;

// 全局变量声明
extern BYTE DEF_PPU_PAL[];

// 函数声明
void ppu_init    (PPU *ppu, DWORD extra, VDEV *pdev);
void ppu_free    (PPU *ppu);
void ppu_reset   (PPU *ppu);
void ppu_run_pclk(PPU *ppu);

BYTE NES_PPU_REG_RCB(MEM *pm, int addr);
void NES_PPU_REG_WCB(MEM *pm, int addr, BYTE byte);

#endif

