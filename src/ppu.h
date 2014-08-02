#ifndef _NES_PPU_H_
#define _NES_PPU_H_

// 包含头文件
#include "stdefine.h"
#include "mem.h"

// 类型定义
typedef struct {
    void *vdevctxt;
    int   scanline;
    BYTE  sprram[256]; // sprite ram
} PPU;

// 函数声明
void ppu_init  (PPU *ppu, DWORD extra);
void ppu_free  (PPU *ppu);
void ppu_reset (PPU *ppu);
void ppu_run   (PPU *ppu, int scanline);
int  ppu_getvbl(PPU *ppu);

void NES_PPU_REG_RCB(MEM *pm, int addr);
void NES_PPU_REG_WCB(MEM *pm, int addr, BYTE byte);

#endif

