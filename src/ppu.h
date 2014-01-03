#ifndef _NES_PPU_H_
#define _NES_PPU_H_

// 包含头文件
#include "stdefine.h"

// 类型定义
typedef struct {
    BYTE sprram[256]; // sprite ram

    // 256-color bitmap for video rendering
    #define PPU_IMAGE_WIDTH  256
    #define PPU_IMAGE_HEIGHT 240
    BYTE  bmp_data[PPU_IMAGE_WIDTH * PPU_IMAGE_HEIGHT];
    BYTE *bmp_pal;
} PPU;

void ppu_init (PPU *ppu);
void ppu_close(PPU *ppu);

#endif

