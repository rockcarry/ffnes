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
    0x80, 0x80, 0x80,
    0x00, 0x00, 0xBB,
    0x37, 0x00, 0xBF,
    0x84, 0x00, 0xA6,
    0xBB, 0x00, 0x6A,
    0xB7, 0x00, 0x1E,
    0xB3, 0x00, 0x00,
    0x91, 0x26, 0x00,
    0x7B, 0x2B, 0x00,
    0x00, 0x3E, 0x00,
    0x00, 0x48, 0x0D,
    0x00, 0x3C, 0x22,
    0x00, 0x2F, 0x66,
    0x00, 0x00, 0x00,
    0x05, 0x05, 0x05,
    0x05, 0x05, 0x05,
    0xC8, 0xC8, 0xC8,
    0x00, 0x59, 0xFF,
    0x44, 0x3C, 0xFF,
    0xB7, 0x33, 0xCC,
    0xFF, 0x33, 0xAA,
    0xFF, 0x37, 0x5E,
    0xFF, 0x37, 0x1A,
    0xD5, 0x4B, 0x00,
    0xC4, 0x62, 0x00,
    0x3C, 0x7B, 0x00,
    0x1E, 0x84, 0x15,
    0x00, 0x95, 0x66,
    0x00, 0x84, 0xC4,
    0x11, 0x11, 0x11,
    0x09, 0x09, 0x09,
    0x09, 0x09, 0x09,
    0xFF, 0xFF, 0xFF,
    0x00, 0x95, 0xFF,
    0x6F, 0x84, 0xFF,
    0xD5, 0x6F, 0xFF,
    0xFF, 0x77, 0xCC,
    0xFF, 0x6F, 0x99,
    0xFF, 0x7B, 0x59,
    0xFF, 0x91, 0x5F,
    0xFF, 0xA2, 0x33,
    0xA6, 0xBF, 0x00,
    0x51, 0xD9, 0x6A,
    0x4D, 0xD5, 0xAE,
    0x00, 0xD9, 0xFF,
    0x66, 0x66, 0x66,
    0x0D, 0x0D, 0x0D,
    0x0D, 0x0D, 0x0D,
    0xFF, 0xFF, 0xFF,
    0x84, 0xBF, 0xFF,
    0xBB, 0xBB, 0xFF,
    0xD0, 0xBB, 0xFF,
    0xFF, 0xBF, 0xEA,
    0xFF, 0xBF, 0xCC,
    0xFF, 0xC4, 0xB7,
    0xFF, 0xCC, 0xAE,
    0xFF, 0xD9, 0xA2,
    0xCC, 0xE1, 0x99,
    0xAE, 0xEE, 0xB7,
    0xAA, 0xF7, 0xEE,
    0xB3, 0xEE, 0xFF,
    0xDD, 0xDD, 0xDD,
    0x11, 0x11, 0x11,
    0x11, 0x11, 0x11,
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
        *pdst++ = (BYTE)b;
        *pdst++ = (BYTE)g;
        *pdst++ = (BYTE)r;
        *pdst++ = (BYTE)0;
        if (psrc == pend) {
            psrc = DEF_PPU_PAL;
        }
    }

    // set vdev palette
    vdev_setpal(ctxt, 0, 256, pal);
}

#define FINEX (ppu->temp1)
#define FINEY (ppu->vaddr >> 12)
#define TILEX ((ppu->vaddr >> 0) & 0x1f)
#define TILEY ((ppu->vaddr >> 5) & 0x1f)
static int ppu_xincrement(PPU *ppu)
{
    if (FINEX == 7)
    {
        FINEX = 0;
        if ((ppu->vaddr & 0x1f) == 31)
        {
            ppu->vaddr &=~0x1f;
            ppu->vaddr ^= (1 << 10);
        }
        else ppu->vaddr++;
        return 1; // tile is changed
    }
    else {
        FINEX++;
        return 0; // tile not changed
    }
}

static int ppu_yincrement(PPU *ppu)
{
    if ((ppu->vaddr & 0x7000) == 0x7000)
    {
        ppu->vaddr &=~0x7000;
        switch (ppu->vaddr & (0x1f << 5))
        {
        case (29 << 5):
            ppu->vaddr &=~(0x1f <<  5);
            ppu->vaddr ^= (0x01 << 11);
            break;
        case (31 << 5):
            ppu->vaddr &=~(0x1f <<  5);
            break;
        default:
            ppu->vaddr += (0x01 <<  5);
            break;
        }
        return 1;
    }
    else
    {
        ppu->vaddr += 0x1000;
        return 0;
    }
}

static void ppu_render_scanline_bkg(PPU *ppu, int scanline)
{
    NES *nes    = container_of(ppu, NES, ppu);
    BYTE tdata, adata, cdatal, cdatah, pixell, pixelh;
    int  ntabn, toffs, aoffs, ashift;
    int  total  = 256; // total pixel number in a scanline
    int  ctflag = 1;   // flag for tile change

    do {
        if (ctflag)
        {
            // ntabn, toffs & aoffs
            ntabn = (ppu->vaddr >> 10) & 0x0003;
            toffs = 0x0000 + (ppu->vaddr & 0x03ff);
            aoffs = 0x03c0 + ((ppu->vaddr >> 4) & 0x38) + ((ppu->vaddr >> 2) & 0x07);

            // tdata, adata & cdata
            tdata  = nes->vram[ntabn].data[toffs];
            adata  = nes->vram[ntabn].data[aoffs];
            cdatal = ppu->chrom_bkg[tdata * 16 + 8 * 0 + FINEY] << FINEX;
            cdatah = ppu->chrom_bkg[tdata * 16 + 8 * 1 + FINEY] << FINEX;

            // ashift & pixelh
            ashift = ((TILEY & (1 << 1)) << 1) | ((TILEX & (1 << 1)) << 0);
            pixelh = ((adata >> ashift) & 0x3) << 2;
        }

        // write pixel on adev
        pixell = ((cdatal >> 7) << 0) | ((cdatah >> 7) << 1);
        cdatal <<= 1; cdatah <<= 1;
        *ppu->draw_buffer++ = ppu->palette[pixelh|pixell];

        // do x increment for scroll
        ctflag = ppu_xincrement(ppu);
    } while (--total > 0);

    ppu->draw_buffer -= 256;
    ppu->draw_buffer += ppu->draw_stride;

    // do y increment for scroll
    ppu_yincrement(ppu);

    // reget vaddr from temp0
    ppu->vaddr &= ~0x041f;
    ppu->vaddr |= (ppu->temp0 & 0x041f);
}

static void ppu_render_scanline_spr(PPU *ppu, int scanline)
{
    // todo..
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
    ppu->toggle       = 0;
    ppu->vaddr        = 0;
    ppu->temp0        = 0;
    ppu->temp1        = 0;
    ppu->chrom_bkg    = ppu->regs[0x0000] & (1 << 4) ? nes->pattab1.data : nes->pattab0.data;
    ppu->chrom_spr    = ppu->regs[0x0000] & (1 << 3) ? nes->pattab1.data : nes->pattab0.data;
    ppu_set_vdev_pal(ppu->vdevctxt, 0);
}

void ppu_run(PPU *ppu, int scanline)
{
    // save scanline
    ppu->scanline = scanline;

    // scanline 0, junk/refresh
    if (scanline == 0)
    {
        ppu->regs[0x0002] &= ~(1 << 7);
        ppu->pin_vbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 7);

        // this line clears bits $2002.5-7 & clear toggle
        // also throws away all the sprite data.
        ppu->regs[0x0002] &= ~(3 << 5);
        ppu->toggle = 0; // clear toogle
        memset(ppu->sprram, 0, 256);

        if (ppu->regs[0x0001] & (0x3 << 3))
        {
            ppu->vaddr &= ~0xfbe0;
            ppu->vaddr |= (ppu->temp0 & 0xfbe0);
        }

        // lock video device, obtain draw buffer address & stride
        vdev_lock(ppu->vdevctxt, (void**)&(ppu->draw_buffer), &(ppu->draw_stride));
    }
    // renders the screen for 240 lines
    else if (scanline >=1 && scanline <= 240)
    {
        if (ppu->regs[0x0001] & (1 << 3)) ppu_render_scanline_bkg(ppu, scanline);
        if (ppu->regs[0x0001] & (1 << 4)) ppu_render_scanline_spr(ppu, scanline);
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
        ppu->toggle = 0; // after a read occurs, toggle is reset
        break;

    case 0x0004:
        byte = ppu->sprram[pm->data[0x0003]];
        break;

    case 0x0007:
        if ( ppu->scanline >= 0 && ppu->scanline <= 240
           && (ppu->regs[0x0001] & (0x3 << 3)) != 0 )
        {
            ppu_xincrement(ppu);
            ppu_yincrement(ppu);
        }
        else
        {
            byte = bus_readb(nes->pbus, ppu->vaddr & 0x3fff);
            if (pm->data[0x0000] & (1 << 2)) {
                ppu->vaddr += 32;
            }
            else ppu->vaddr++;
        }
        break;
    }
    return byte;
}

void NES_PPU_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    NES *nes = container_of(pm, NES, ppuregs);
    PPU *ppu = &(nes->ppu);

    switch (addr)
    {
    case 0x0000:
        ppu->temp0 &=~(0x03 << 10);
        ppu->temp0 |= (byte & 0x03) << 10;
        ppu->chrom_bkg = ppu->regs[0x0000] & (1 << 4) ? nes->pattab1.data : nes->pattab0.data;
        ppu->chrom_spr = ppu->regs[0x0000] & (1 << 3) ? nes->pattab1.data : nes->pattab0.data;
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
        ppu->toggle = !ppu->toggle;
        if (ppu->toggle)
        {
            ppu->temp0 &= ~(0x1f <<  0);
            ppu->temp0 |=  (byte >>  3);
            ppu->temp1  =  (byte & 0x7);
        }
        else
        {
            ppu->temp0 &= ~(0x1f <<  5);
            ppu->temp0 |=  (byte >>  3) << 5;
            ppu->temp0 &= ~(0x07 << 12);
            ppu->temp0 |=  (byte & 0x7) << 12;
        }
        break;

    case 0x0006:
        ppu->toggle = !ppu->toggle;
        if (ppu->toggle)
        {
            ppu->temp0 &=~(0xff << 8);
            ppu->temp0 |= (byte << 8);
            ppu->temp0 &= 0x3fff;
        }
        else
        {
            ppu->temp0 &=~(0xff << 0);
            ppu->temp0 |= (byte << 0);
            ppu->vaddr  = ppu->temp0;
        }
        break;

    case 0x0007:
        if ( ppu->scanline >= 0 && ppu->scanline <= 240
           && (ppu->regs[0x0001] & (0x3 << 3)) != 0 )
        {
            ppu_xincrement(ppu);
            ppu_yincrement(ppu);
        }
        else
        {
            bus_writeb(nes->pbus, ppu->vaddr & 0x3fff, byte);
            if (pm->data[0x0000] & (1 << 2)) {
                ppu->vaddr += 32;
            }
            else ppu->vaddr++;
        }
        break;
    }
}


