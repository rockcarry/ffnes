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
    BYTE *psrc   = DEF_PPU_PAL;
    BYTE *pend   = psrc + 64 * 3;
    BYTE *pdst   = pal;
    int   factor = flags >> 5;
    int   r, g, b, i;

    for (i=0; i<256; i++)
    {
        if (flags & (1 << 0))
        {   // monochrome
            // how to convert rgb color to gray ?
            // gray = R*0.299 + G*0.587 + B*0.114
            r = g = b = (psrc[0]*77 + psrc[1]*150 + psrc[2]*29) >> 8;
            psrc += 3;
        }
        else
        {   // colorized
            r = *psrc++;
            g = *psrc++;
            b = *psrc++;
        }
        r = (r * color_emphasis_factor[factor][0]) >> 8;
        g = (g * color_emphasis_factor[factor][1]) >> 8;
        b = (b * color_emphasis_factor[factor][2]) >> 8;
        r = (r < 255) ? r : 255;
        g = (g < 255) ? g : 255;
        b = (b < 255) ? b : 255;
        *pdst++ = (BYTE)r;
        *pdst++ = (BYTE)g;
        *pdst++ = (BYTE)b;
        *pdst++ = (BYTE)0;
        if (psrc == pend) {
            psrc = DEF_PPU_PAL;
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
    NES *nes = container_of(ppu, NES, ppu);

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
    ppu->chrom_bkg    = ppu->regs[0x0000] & (1 << 4) ? nes->pattab0.data : nes->pattab1.data;
    ppu->chrom_spr    = ppu->regs[0x0000] & (1 << 3) ? nes->pattab0.data : nes->pattab1.data;
    ppu_set_vdev_pal(ppu->vdevctxt, 0);
}

void ppu_run(PPU *ppu, int scanline)
{
    // scanline 0, junk/refresh
    if (scanline == 0)
    {
        ppu->regs[0x0002] &= ~(1 << 7);
        ppu->pin_vbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 7);

        ppu->regs[0x0002] &= ~(3 << 5);
        memset(ppu->sprram, 0, 256);

        // lock video device, obtain draw buffer address & stride
        vdev_lock(ppu->vdevctxt, &(ppu->draw_buffer), &(ppu->draw_stride));
    }
    // renders the screen for 240 lines
    else if (scanline >=1 && scanline <= 240)
    {
        NES  *nes    = container_of(ppu, NES, ppu);
        BYTE  ntabn  = ppu->ntabn & (1 << 0);
        BYTE  ntile, chdatal, chdatah, atdata, atoffs, pixell, pixelh, pixelf;
        int   ctflag = 1;   // flag for tile change
        int   total  = 256; // total pixel number in a scanline

        do {
            if (ctflag)
            {
                ntile   = nes->vram[ntabn].data[ppu->tiley * 32 + ppu->tilex];
                chdatal = ppu->chrom_bkg[ntile * 16 + 8 * 0 + ppu->finey];
                chdatah = ppu->chrom_bkg[ntile * 16 + 8 * 1 + ppu->finey];
                atdata  = (nes->vram[ntabn].data + 960)[(ppu->tiley >> 2) * 8 + (ppu->tilex >> 2)];
                atoffs  = ((ppu->tiley & (1 << 1)) << 1) | ((ppu->tilex & (1 << 1)) << 0);
                pixelh  = (atdata >> atoffs) & 0x3;
            }

            pixell = (((chdatal >> ppu->finex) & 0x1) << 0) | (((chdatah >> ppu->finex) & 0x1) << 1);
            pixelf = (pixelh << 2) | (pixell << 0);
            *ppu->draw_buffer++ = ppu->palette[pixelf];

            if (++ppu->finex == 8) {
                ppu->finex = 0;
                ppu->tilex++;

                if (ppu->tilex == 32) {
                    ppu->tilex = 0;
                    ntabn ^= (1 << 0);
                }

                ctflag = 1; // tile is changed
            }
            else ctflag = 0; // tile not change
        } while (--total > 0);

        if (++ppu->finey == 8) {
            ppu->finey = 0;
            ppu->tiley++;
        }
        if (ppu->tiley == 30) {
            ppu->tiley = 0;
            ntabn ^= (1 << 1);
        }

        ppu->draw_buffer -= 256;
        ppu->draw_buffer += ppu->draw_stride;
    }
    // scanline 242: dead/junk
    else if (scanline == 241)
    {
        ppu->regs[0x0002] |= (1 << 7);
        ppu->pin_vbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 7);

        // unlock video device
        vdev_unlock(ppu->vdevctxt);
    }
    // line 243 - 262(NTSC) - 312(PAL): vblank
    else if (scanline >= 242 && scanline <= 261)
    {
        ppu->pin_vbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 7);
    }
}

BYTE NES_PPU_REG_RCB(MEM *pm, int addr)
{
    NES *nes  = container_of(pm, NES, ppuregs);
    PPU *ppu  = &(nes->ppu);
    BYTE byte = pm->data[addr];

    switch (addr)
    {
    case 0x0002:
        pm->data[0x0002] &= ~(1 << 7); // after a read occurs, D7 is set to 0 
        ppu->_2005_toggle = 0; // after a read occurs, $2005 toggle is reset
        ppu->_2006_toggle = 0; // after a read occurs, $2006 toggle is reset
        break;

    case 0x0007:
        byte = bus_readb(nes->pbus, ppu->pio_addr);
        if (pm->data[0x0000] & (1 << 2)) {
            ppu->pio_addr += 32;
        }
        else ppu->pio_addr++;
        break;
    }
    return byte;
}

void NES_PPU_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    NES *nes       = container_of(pm, NES, ppuregs);
    PPU *ppu       = &(nes->ppu);
    int  pbus_addr = ppu->pio_addr;

    switch (addr)
    {
    case 0x0000:
        ppu->ntabn     = (byte & 0x3);
        ppu->chrom_bkg = ppu->regs[0x0000] & (1 << 4) ? nes->pattab0.data : nes->pattab1.data;
        ppu->chrom_spr = ppu->regs[0x0000] & (1 << 3) ? nes->pattab0.data : nes->pattab1.data;
        break;

    case 0x0001:
        if (ppu->color_flags != (byte & 0xe1))
        {
            ppu->color_flags = byte & 0xe1;
            ppu_set_vdev_pal(ppu->vdevctxt, ppu->color_flags);
        }
        break;

    case 0x0004:
        ppu->sprram[pm->data[0x0003]] = byte;
        break;

    case 0x0005:
        ppu->_2005_toggle = !ppu->_2005_toggle;
        if (ppu->_2005_toggle)
        {
            ppu->tilex = (byte >> 3 );
            ppu->finex = (byte & 0x7);
        }
        else
        {
            ppu->tiley = (byte >> 3 );
            ppu->finey = (byte & 0x7);
            if (ppu->tiley > 29) ppu->tiley = 0;
        }
        break;

    case 0x0006:
        ppu->_2006_toggle = !ppu->_2006_toggle;
        if (ppu->_2006_toggle)
        {
            ppu->ntabn  = (byte >>  2) & 0x3;
            ppu->finey  = (byte >>  4) & 0x3;
            ppu->tiley &=~(0x3  <<  3);
            ppu->tiley |= (byte & 0x3) << 3;
        }
        else
        {
            ppu->tilex  = (byte & 0x1f);
            ppu->tiley &=~(0x7  << 0);
            ppu->tiley |= (byte >> 5);

            ppu->pio_addr = (ppu->finey << 12)
                          | (ppu->ntabn << 10)
                          | (ppu->tiley << 5 )
                          | (ppu->tilex << 0 );
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
            ppu->pio_addr += 32;
        }
        else ppu->pio_addr++;
        break;
    }
}


