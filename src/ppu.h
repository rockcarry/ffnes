#ifndef _NES_PPU_H_
#define _NES_PPU_H_

// 包含头文件
#include "stdefine.h"
#include "mem.h"

// 类型定义
typedef struct {
    BYTE sprram[256]; // sprite ram

    // 256-color bitmap for video rendering
    #define PPU_IMAGE_WIDTH  (256 * 2)
    #define PPU_IMAGE_HEIGHT (240 * 2)
    BYTE  bmp_data[PPU_IMAGE_WIDTH * PPU_IMAGE_HEIGHT];
    BYTE *bmp_pal;
    int   bmp_offx;
    int   bmp_offy;
} PPU;

void ppu_init  (PPU *ppu);
void ppu_reset (PPU *ppu);
void ppu_render(PPU *ppu);
void NES_PPU_REG_RCB(MEM *pm, int addr);
void NES_PPU_REG_WCB(MEM *pm, int addr);

#endif

