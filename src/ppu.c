// 包含头文件
#include "nes.h"
#include "vdev.h"
#include "log.h"

// 内部常量定义
#define PPU_IMAGE_WIDTH  256
#define PPU_IMAGE_HEIGHT 240

// 内部全局变量定义
static BYTE DEF_PPU_PAL[64 * 3] =
{
//  R     G     B
    0x75, 0x75, 0x75,
    0x27, 0x1B, 0x8F,
    0x00, 0x00, 0xAB,
    0x47, 0x00, 0x9F,
    0x8F, 0x00, 0x77,
    0xAB, 0x00, 0x13,
    0xA7, 0x00, 0x00,
    0x7F, 0x0B, 0x00,
    0x43, 0x2F, 0x00,
    0x00, 0x47, 0x00,
    0x00, 0x51, 0x00,
    0x00, 0x3F, 0x17,
    0x1B, 0x3F, 0x5F,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0xBC, 0xBC, 0xBC,
    0x00, 0x73, 0xEF,
    0x23, 0x3B, 0xEF,
    0x83, 0x00, 0xF3,
    0xBF, 0x00, 0xBF,
    0xE7, 0x00, 0x5B,
    0xDB, 0x2B, 0x00,
    0xCB, 0x4F, 0x0F,
    0x8B, 0x73, 0x00,
    0x00, 0x97, 0x00,
    0x00, 0xAB, 0x00,
    0x00, 0x93, 0x3B,
    0x00, 0x83, 0x8B,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF,
    0x3F, 0xBF, 0xFF,
    0x5F, 0x97, 0xFF,
    0xA7, 0x8B, 0xFD,
    0xF7, 0x7B, 0xFF,
    0xFF, 0x77, 0xB7,
    0xFF, 0x77, 0x63,
    0xFF, 0x9B, 0x3B,
    0xF3, 0xBF, 0x3F,
    0x83, 0xD3, 0x13,
    0x4F, 0xDF, 0x4B,
    0x58, 0xF8, 0x98,
    0x00, 0xEB, 0xDB,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF,
    0xAB, 0xE7, 0xFF,
    0xC7, 0xD7, 0xFF,
    0xD7, 0xCB, 0xFF,
    0xFF, 0xC7, 0xFF,
    0xFF, 0xC7, 0xDB,
    0xFF, 0xBF, 0xB3,
    0xFF, 0xDB, 0xAB,
    0xFF, 0xE7, 0xA3,
    0xF3, 0xFF, 0xA3,
    0xAB, 0xF3, 0xBF,
    0xB3, 0xFF, 0xCF,
    0x9F, 0xFF, 0xF3,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
};

/*
001   R: 123.9%   G: 091.5%   B: 074.3%
010   R: 079.4%   G: 108.6%   B: 088.2%
011   R: 101.9%   G: 098.0%   B: 065.3%
100   R: 090.5%   G: 102.6%   B: 127.7%
101   R: 102.3%   G: 090.8%   B: 097.9%
110   R: 074.1%   G: 098.7%   B: 100.1%
111   R: 075.0%   G: 075.0%   B: 075.0%
*/
static int color_emphasis_factor[8][3] =
{
    { 256, 256, 256 },
    { 317, 234, 190 },
    { 203, 278, 225 },
    { 261, 251, 167 },
    { 232, 263, 327 },
    { 262, 232, 251 },
    { 190, 253, 256 },
    { 192, 192, 192 },
};

// 内部函数实现
static void ppu_set_vdev_pal(void *ctxt, int flags)
{
    BYTE  pal[256 * 4];
    BYTE *psrc = DEF_PPU_PAL;
    BYTE *pend = psrc + 64 * 3;
    BYTE *pdst = pal;
    int   r, g, b, gray;
    int   factor, i;

    if (flags & (1 << 0))
    {   // monochrome
        for (i=0; i<256; i++)
        {
            gray    = (psrc[0] + psrc[1] + psrc[2]) / 3;
            *pdst++ = (BYTE)gray;
            *pdst++ = (BYTE)gray;
            *pdst++ = (BYTE)gray;
            *pdst++ = 0;
            psrc   += 3;
            if (psrc == pend) {
                psrc = DEF_PPU_PAL;
            }
        }
        //++ for 2001.5-7 ++//
        pal[255 * 4 + 0] = flags & (1 << 7) ? 255 : 0;
        pal[255 * 4 + 1] = flags & (1 << 6) ? 255 : 0;
        pal[255 * 4 + 2] = flags & (1 << 5) ? 255 : 0;
        //-- for 2001.5-7 --//
    }
    else
    {   // colorized
        factor = flags >> 5;
        for (i=0; i<256; i++)
        {
            r = (*psrc++ * color_emphasis_factor[factor][0]) >> 8;
            g = (*psrc++ * color_emphasis_factor[factor][1]) >> 8;
            b = (*psrc++ * color_emphasis_factor[factor][2]) >> 8;
            r = (r < 255) ? r : 255;
            g = (g < 255) ? g : 255;
            b = (b < 255) ? b : 255;
            *pdst++ = (BYTE)b;
            *pdst++ = (BYTE)g;
            *pdst++ = (BYTE)r;
            *pdst++ = (BYTE)0;
            if (psrc == pend) {
                psrc = DEF_PPU_PAL;
            }
        }
    }

    // set vdev palette
    vdev_setpal(ctxt, 0, 256, pal);
}

// 函数实现
void ppu_init(PPU *ppu, DWORD extra)
{
    // create vdev for ppu
    ppu->vdevctxt = vdev_create(PPU_IMAGE_WIDTH, PPU_IMAGE_HEIGHT, 8, extra);
    if (!ppu->vdevctxt) log_printf("ppu_init:: failed to create vdev !\n");

    // reset ppu
    ppu_reset(ppu);
}

void ppu_free(PPU *ppu)
{
    vdev_destroy(ppu->vdevctxt);
}

void ppu_reset(PPU *ppu)
{
    ppu->pin_vbl      = 1;
    ppu->color_flags  = 0;
    ppu->_2005_toggle = 0;
    ppu->_2006_toggle = 0;
    ppu->pio_addr     = 0;
    ppu->ntabn        = 0;
    ppu->tilex        = 0;
    ppu->tiley        = 0;
    ppu->finex        = 0;
    ppu->finey        = 0;
    ppu_set_vdev_pal(ppu->vdevctxt, 0);
}

void ppu_run(PPU *ppu, int scanline)
{
    // scanline 0, junk/refresh
    if (scanline == 0)
    {
        ppu->regs[0x0002] &= ~(1 << 7);
        ppu->pin_vbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 0);
    }
    // renders the screen for 240 lines
    else if (scanline >=1 && scanline <= 240)
    {
    }
    // scanline 242: dead/junk
    else if (scanline == 241)
    {
        ppu->regs[0x0002] |=  (1 << 7);
        ppu->pin_vbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 0);
    }
    // line 243 - 262(NTSC) - 312(PAL): vblank
    else if (scanline >= 242 && scanline <= 261)
    {
        ppu->pin_vbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 0);
    }
}

BYTE NES_PPU_REG_RCB(MEM *pm, int addr)
{
    NES *nes  = container_of(pm, NES, ppuregs);
    BYTE byte = pm->data[addr];

    switch (addr)
    {
    case 0x0002:
        pm->data[0x0002] &= ~(1 << 7);  // after a read occurs, D7 is set to 0 
        nes->ppu._2005_toggle = 0;      // after a read occurs, $2005 toggle is reset
        nes->ppu._2006_toggle = 0;      // after a read occurs, $2006 toggle is reset
        break;

    case 0x0007:
        byte = bus_readb(nes->pbus, nes->ppu.pio_addr);
        if (pm->data[0x0000] & (1 << 2)) {
            nes->ppu.pio_addr += 32;
        }
        else nes->ppu.pio_addr++;
        break;
    }
    return byte;
}

void NES_PPU_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    NES *nes       = container_of(pm, NES, ppuregs);
    int  pbus_addr = nes->ppu.pio_addr;

    switch (addr)
    {
    case 0x0000:
        nes->ppu.ntabn = (byte & 0x3);
        break;

    case 0x0001:
        if (nes->ppu.color_flags != (byte & 0xe1))
        {
            nes->ppu.color_flags = byte & 0xe1;
            ppu_set_vdev_pal(nes->ppu.vdevctxt, nes->ppu.color_flags);
        }
        break;

    case 0x0004:
        nes->ppu.sprram[pm->data[0x0003]] = byte;
        break;

    case 0x0005:
        nes->ppu._2005_toggle = !nes->ppu._2005_toggle;
        if (nes->ppu._2005_toggle)
        {
            nes->ppu.tilex =  (byte >> 3 );
            nes->ppu.finex =  (byte & 0x7);
        }
        else
        {
            nes->ppu.tiley =  (byte >> 3 );
            nes->ppu.finey =  (byte & 0x7);
        }
        break;

    case 0x0006:
        nes->ppu._2006_toggle = !nes->ppu._2006_toggle;
        if (nes->ppu._2006_toggle)
        {
            nes->ppu.ntabn    = (byte >>  2) & 0x3;
            nes->ppu.finey    = (byte >>  4) & 0x3;
            nes->ppu.tiley   &=~(0x3  <<  3);
            nes->ppu.tiley   |= (byte & 0x3) << 3;
        }
        else
        {
            nes->ppu.tilex    = (byte & 0x1f);
            nes->ppu.tiley   &=~(0x7  << 0);
            nes->ppu.tiley   |= (byte >> 5);

            nes->ppu.pio_addr = (nes->ppu.finey << 12)
                              | (nes->ppu.ntabn << 10)
                              | (nes->ppu.tiley << 5 )
                              | (nes->ppu.tilex << 0 );
        }
        break;

    case 0x0007:
        if (pbus_addr >= 0x3f00 && pbus_addr <= 0x3fff)
        {
            // for palette mirrors
            if ((pbus_addr & 0x0003) == 0x0000) {
                pbus_addr = 0x3f00;
            }
            byte &= 0x3f; // only 6bits is valid for palette
        }

        bus_writeb(nes->pbus, pbus_addr, byte);
        if (pm->data[0x0000] & (1 << 2)) {
            nes->ppu.pio_addr += 32;
        }
        else nes->ppu.pio_addr++;
        break;
    }
}


