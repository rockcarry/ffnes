#ifndef _NES_PPU_H_
#define _NES_PPU_H_

#include "mem.h"

#define NES_HTOTAL      341 // htotal
#define NES_VTOTAL      262 // vtotal
#define NES_WIDTH       256 // screen width
#define NES_HEIGHT      240 // screen height
#define NES_FRAMERATE  (FREQ_PPU / (NES_HTOTAL * NES_VTOTAL)) // frame rate

typedef void (*PPU_RENDER_CB)(void *cbctx, int type, uint8_t *buf, int len);

typedef struct {
    uint8_t  regs   [8  ]; // ppu regs
    uint8_t  sprram [256]; // sprite ram
    uint8_t  palette[32 ]; // palette

// private:
    int       toggle;
    int       finex;
    uint16_t  vaddr;
    uint16_t  temp0; // correspond to vaddr
    uint16_t  temp1; // correspond to finex
    uint8_t  _2007_lazy;
    uint8_t  *chrom_bkg;
    uint8_t  *chrom_spr;

    //++ for step pclk rendering ++//
    int      pclk_frame; // current pclk cycles in one frame
    int      pclk_line;  // current pclk cycles in one line
    int      oddevenflag;// odd/even frame toggle flag
    int      scanline;   // current scanline

    uint32_t tdatal;
    uint32_t tdatah;
    uint32_t adata;
    //-- for step pclk rendering --//

    //++ sprite buffer for rendering
    uint8_t  sprbuf[64]; // sprite buffer
    uint8_t  sprnum;     // sprite number
    uint8_t *sprzero;    // sprite zero
    //-- sprite buffer for rendering

    // palette for vdev rendering
    uint8_t  vdevpal[64 * 4];

    uint32_t*draw_buffer;
    uint32_t draw_bitmap[NES_WIDTH * NES_HEIGHT];

    PPU_RENDER_CB callback;
    void         *cbctxt;
} PPU;

extern uint8_t DEF_PPU_PAL[];

void ppu_init (PPU *ppu, PPU_RENDER_CB callback, void *cbctxt);
void ppu_free (PPU *ppu);
void ppu_reset(PPU *ppu);
void ppu_run  (PPU *ppu);

static __inline int ppu_pin_vbl(PPU *ppu) { return (ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 7) ? 0 : 1; }

uint8_t NES_PPU_REG_RCB(MEM *pm, int addr);
void    NES_PPU_REG_WCB(MEM *pm, int addr, uint8_t byte);

#endif

