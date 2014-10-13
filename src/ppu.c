// 包含头文件
#include "nes.h"
#include "vdev.h"
#include "log.h"

// 内部常量定义
#define PPU_IMAGE_WIDTH  256
#define PPU_IMAGE_HEIGHT 240

// 内部全局变量定义
BYTE DEF_PPU_PAL[64 * 3] =
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
static void ppu_set_vdev_pal(PPU *ppu, int flags)
{
    BYTE *psrc   = DEF_PPU_PAL;
    BYTE *pend   = psrc + 64 * 3;
    BYTE *pdst   = ppu->vdevpal;
    int   factor = flags >> 5;
    int   r, g, b, i;

    for (i=0; i<64; i++)
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
}

#define FINEX (ppu->finex)
#define FINEY (ppu->vaddr >> 12)
#define TILEX ((ppu->vaddr >> 0) & 0x1f)
#define TILEY ((ppu->vaddr >> 5) & 0x1f)
static void ppu_fetch_tile(PPU *ppu)
{
    NES *nes = container_of(ppu, NES, ppu);
    int  ntabn, toffs, aoffs, ashift;
    BYTE tdata, adata;

    // ntabn, toffs & aoffs
    ntabn = (ppu->vaddr >> 10) & 0x0003;
    toffs = 0x0000 + (ppu->vaddr & 0x03ff);
    aoffs = 0x03c0 + ((ppu->vaddr >> 4) & 0x38) + ((ppu->vaddr >> 2) & 0x07);

    // tdata, adata & cdata
    tdata       = nes->vram[ntabn].data[toffs];
    adata       = nes->vram[ntabn].data[aoffs];
    ppu->cdatal = ppu->chrom_bkg[tdata * 16 + 8 * 0 + FINEY] << FINEX;
    ppu->cdatah = ppu->chrom_bkg[tdata * 16 + 8 * 1 + FINEY] << FINEX;

    // ashift & pixelh
    ashift      = ((TILEY & (1 << 1)) << 1) | ((TILEX & (1 << 1)) << 0);
    ppu->pixelh = ((adata >> ashift) & 0x3) << 2;
}

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
        return 1;
    }
    else {
        FINEX++;
        return 0;
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
    else {
        ppu->vaddr += 0x1000;
        return 0;
    }
}

static void sprite_evaluate(PPU *ppu)
{
    NES  *nes = container_of(ppu, NES, ppu);
    int   sh  = (ppu->regs[0x0000] & (1 << 5)) ? 16 : 8;
    int   sy, tile, i;
    BYTE *chrrom, *sprsrc, *sprdst;

    sprsrc = ppu->sprram;
    sprdst = ppu->sprbuf;

    ppu->sprnum  = 0;
    ppu->sprzero = NULL;
    if (ppu->scanline >= sprsrc[0] && ppu->scanline < sprsrc[0] + sh)
    {
        ppu->sprzero = sprdst;
    }

    for (i=0; i<64; i++)
    {
        if (ppu->scanline >= sprsrc[0] && ppu->scanline < sprsrc[0] + sh)
        {
            if (ppu->sprnum++ == 8) // sprite overflow
            {
                ppu->regs[0x0002] |= (1 << 5);
                break;
            }

            tile = sprsrc[1];
            sy   = ppu->scanline - sprsrc[0]; // sy
            if (sprsrc[2] & (1 << 7)) sy = sh - sy - 1; // vflip

            switch (sh)
            {
            case 8:
                chrrom = ppu->chrom_spr;
                break;

            case 16:
                chrrom = (sprsrc[1] & 1) ? nes->chrrom1.data : nes->chrrom0.data;
                if (sy < 8)
                {
                    tile &= ~(1 << 0);
                }
                else
                {
                    tile |=  (1 << 0);
                    sy -= 8;
                }
                break;
            }

            // for sprdst[ppu->sprnum * 4 + 2], only h&p&c (hflip, background priority & upper two bits of colour)
            // is useful for sprite rendering, so I store these four bits in upper bits of this byte,
            // the lower bits of this byte I will use to counter how many horizontal pixels has been rendered
            *sprdst++ = chrrom[tile * 16 + 8 * 0 + sy];
            *sprdst++ = chrrom[tile * 16 + 8 * 1 + sy];
            *sprdst++ = ((sprsrc[2] << 1) & 0xc0) | ((sprsrc[2] << 4) & 0x30);
            *sprdst++ = sprsrc[3];
        }

        // next sprite
        sprsrc += 4;
    }
}

static void sprite_render(PPU *ppu, int pixelc)
{
    int   n       = ppu->sprnum;
	BYTE *sprdata = ppu->sprbuf + n * 4;
    int   scolor;

    while (n-- > 0)
    {
        sprdata -= 4; // minus 4 sprdata point to the correct data

        if (  sprdata[3] == ppu->pclk_line
           && !(sprdata[2] & (1 << 3)) )
        {
            if (sprdata[2] & (1 << 7)) // hflip - 1
            {
                scolor = (sprdata[0] & 1) | ((sprdata[1] & 1) << 1);
                sprdata[0] >>= 1;
                sprdata[1] >>= 1;
            }
            else // hflip - 0
            {
                scolor = (sprdata[0] >> 7) | ((sprdata[1] >> 7) << 1);
                sprdata[0] <<= 1;
                sprdata[1] <<= 1;
            }

            if (!(ppu->_2001_lazy & (1 << 2)) && ppu->pclk_line < 8)
            {
                scolor = 0;
            }

            if (scolor)
            {
                if ((sprdata[2] & (1 << 6)) && (pixelc & 0x3))
                {
                    *ppu->draw_buffer = ((DWORD*)ppu->vdevpal)[ppu->palette[pixelc]];
                }
                else
                {
                    scolor |= (sprdata[2] >> 2) & 0xc;
                    *ppu->draw_buffer = ((DWORD*)ppu->vdevpal)[ppu->palette[16 + scolor]];
                }

                // update sprite 0 hit flag
                if (ppu->sprzero == sprdata && ppu->pclk_line != 255 && (pixelc & 0x3))
                {
                    ppu->regs[0x0002] |= (1 << 6);
                }
            }
            sprdata[2]++;
            sprdata[3]++;
        }
    }
}

void ppu_run_pclk(PPU *ppu)
{
    //++ update vblank pin status
    if (  ppu->pclk_frame >= NES_HTOTAL * 241 + 4 && ppu->pclk_frame <= NES_HTOTAL * 261 + 1
       || ppu->pclk_frame == NES_HTOTAL * 261 + 7 )
    {
        ppu->pinvbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 7);
    }
    //-- update vblank pin status

    // scanline 261 pre-render scanline
    if (ppu->pclk_frame == NES_HTOTAL * 261 + 1) // scanline 261, tick 1
    {
        // clear vblank bit of reg $2002
        ppu->vblklast = ppu->regs[0x0002] & (1 << 7);
        ppu->regs[0x0002] &= ~(7 << 5);

        // update _2001_lazy variable
        ppu->_2001_lazy = ppu->regs[0x0001];

        // if sprite or name-tables are visible.
        if (ppu->_2001_lazy & (0x3 << 3))
        {
            // the ppu address copies the ppu's
            // temp at the beginning of the line
            ppu->vaddr = ppu->temp0;
            ppu->finex = ppu->temp1;

            // y increment
            ppu_yincrement(ppu);

            // fetch tile data
            ppu_fetch_tile(ppu);
        }

        // lock video device, obtain draw buffer address & stride
        vdev_lock(ppu->vdevctxt, (void**)&(ppu->draw_buffer), &(ppu->draw_stride));
    }

    // scanline 0 - 239 visible scanlines
    else if (ppu->pclk_frame < NES_HTOTAL * 240)
    {
        if (ppu->pclk_line < 256)
        {
            int pixelc = 0;

            // render background
            if (ppu->_2001_lazy & (1 << 3))
            {
                // write pixel on adev
                if ((ppu->_2001_lazy & (1 << 1)) || ppu->pclk_line >= 8)
                {
                    pixelc = ppu->pixelh | ((ppu->cdatal >> 7) << 0) | ((ppu->cdatah >> 7) << 1);
                }
                ppu->cdatal <<= 1; ppu->cdatah <<= 1;
                *ppu->draw_buffer = ((DWORD*)ppu->vdevpal)[ppu->palette[pixelc]];
            }

            // render sprite
            if (ppu->_2001_lazy & (1 << 4)) sprite_render(ppu, pixelc);

            // do x increment
            if (ppu->_2001_lazy & (0x3 << 3))
            {
                if (ppu_xincrement(ppu)) {
                    // fetch tile data
                    ppu_fetch_tile(ppu);
                }
            }
            else
            {
                if (ppu->vaddr > 0x3f00)
                {
                    *ppu->draw_buffer = ((DWORD*)ppu->vdevpal)[ppu->palette[ppu->vaddr & 0x1f]];
                }
            }

            // next pixel of vdev draw buffer
            ppu->draw_buffer++;
        }
        else if (ppu->pclk_line == 256)
        {
            if (ppu->_2001_lazy & (0x3 << 3))
            {
                // evaluate sprite
                if (ppu->scanline < 240) sprite_evaluate(ppu);

                // at dot 256, reget vaddr from temp0
                ppu->vaddr &= ~0x041f;
                ppu->vaddr |= (ppu->temp0 & 0x041f);
                ppu->finex  = ppu->temp1;

                // at dot 257, do y increment
                ppu_yincrement(ppu);

                // fetch tile data
                ppu_fetch_tile(ppu);
            }

            // next scanline of vdev draw buffer
            ppu->draw_buffer -= 256;
            ppu->draw_buffer += ppu->draw_stride;
        }
    }

    // scanline 240 post-render scanline
    // do nothing

    // scanline 241 - 260 vblank
    else if (ppu->pclk_frame == NES_HTOTAL * 241 + 1) // scanline 241, tick 1
    {
        // unlock video device
        vdev_unlock(ppu->vdevctxt);

        // set vblank bit of reg $2002
        ppu->vblklast = ppu->regs[0x0002] & (1 << 7);
        ppu->regs[0x0002] |= (1 << 7);
    }

    // the last tick of frame, odd frame with tick skipping
    else if (ppu->pclk_frame == NES_HTOTAL * NES_VTOTAL - 2)
    {
        if (ppu->oddevenflag && (ppu->regs[0x0001] & (0x3 << 3)))
        {
            // frame change
            ppu->pclk_frame = 0;
            ppu->pclk_line  = 0;
            ppu->scanline   = 1;
            ppu->oddevenflag= 0; // next frame is even frame
            return;
        }
    }

    // the last tick of frame, even frame or without tick skipping
    else if (ppu->pclk_frame == NES_HTOTAL * NES_VTOTAL - 1)
    {
        // frame change
        ppu->pclk_frame  = 0;
        ppu->pclk_line   = 0;
        ppu->scanline    = 1;
        ppu->oddevenflag = !ppu->oddevenflag; // toggle odd/even frame flag
        return;
    }

    ppu->pclk_frame++;
    ppu->pclk_line ++;
    if (ppu->pclk_line == NES_HTOTAL)
    {
        // scanline change
        ppu->pclk_line = 0;
        ppu->scanline++;
    }
}

// 函数实现
//++ for power up palette
static BYTE power_up_pal[32] =
{
    0x09,0x01,0x00,0x01,0x00,0x02,0x02,0x0D,0x08,0x10,0x08,0x24,0x00,0x00,0x04,0x2C,
    0x09,0x01,0x34,0x03,0x00,0x04,0x00,0x14,0x08,0x3A,0x00,0x02,0x00,0x20,0x2C,0x08,
};
//-- for power up palette
void ppu_init(PPU *ppu, DWORD extra)
{
    // create vdev for ppu
    ppu->vdevctxt = vdev_create(PPU_IMAGE_WIDTH, PPU_IMAGE_HEIGHT, 32, extra);
    if (!ppu->vdevctxt) log_printf("ppu_init:: failed to create vdev !\n");

    // init power up palette
    memcpy(ppu->palette, power_up_pal, 32);

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

    ppu->pinvbl     = 1;
    ppu->toggle     = 0;
    ppu->finex      = 0;
    ppu->vaddr      = 0;
    ppu->temp0      = 0;
    ppu->temp1      = 0;
    ppu->_2007_lazy = 0;
    ppu->_2001_lazy = 0;
    ppu->chrom_bkg  = (ppu->regs[0x0000] & (1 << 4)) ? nes->chrrom1.data : nes->chrrom0.data;
    ppu->chrom_spr  = (ppu->regs[0x0000] & (1 << 3)) ? nes->chrrom1.data : nes->chrrom0.data;
    ppu->pclk_frame = 0;
    ppu->pclk_line  = 0;
    ppu->pclk_fend  = NES_HTOTAL * NES_VTOTAL;
    ppu->oddevenflag= 0;
    ppu->scanline   = 1;
    ppu_set_vdev_pal(ppu, 0);
    memset(ppu->regs, 0, 8); // reset need clear regs
}

BYTE NES_PPU_REG_RCB(MEM *pm, int addr)
{
    NES *nes   = container_of(pm, NES, ppuregs);
    PPU *ppu   = &(nes->ppu);
    BYTE byte  = pm->data[addr];
    int  vaddr = ppu->vaddr & 0x3fff;

    switch (addr)
    {
    case 0x0002:
        pm->data[0x0002] &= ~(1 << 7); // after a read occurs, D7 is set to 0 
        if (  ppu->pclk_frame == NES_HTOTAL * 241 + 2
           || ppu->pclk_frame == NES_HTOTAL * 261 + 2)
        {
            byte &= ~(1 << 7);
            byte |=  ppu->vblklast;
        }
        ppu->toggle = 0; // after a read occurs, toggle is reset
        break;

    case 0x0004:
        byte = ppu->sprram[pm->data[0x0003]];
        break;

    case 0x0007:
        if (vaddr < 0x3f00)
        {
            byte            = ppu->_2007_lazy;
            ppu->_2007_lazy = bus_readb(nes->pbus, vaddr);
        }
        else
        {
            byte            = bus_readb(nes->pbus, vaddr);
            ppu->_2007_lazy = bus_readb(nes->pbus, vaddr & 0x2fff);
        }
        // increase vaddr by 1 or 32
        ppu->vaddr += (pm->data[0x0000] & (1 << 2)) ? 32 : 1;
        break;
    }
    return byte;
}

void NES_PPU_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    NES *nes   = container_of(pm, NES, ppuregs);
    PPU *ppu   = &(nes->ppu);
    int  vaddr = ppu->vaddr & 0x3fff;

    switch (addr)
    {
    case 0x0000:
        ppu->temp0 &=~(0x03 << 10);
        ppu->temp0 |= (byte & 0x03) << 10;
        ppu->chrom_bkg = (byte & (1 << 4)) ? nes->chrrom1.data : nes->chrrom0.data;
        ppu->chrom_spr = (byte & (1 << 3)) ? nes->chrrom1.data : nes->chrrom0.data;
        break;

    case 0x0001:
        // if write d3 & d4 to zero, update _2001_lazy immediately
        if (!(byte & 0x18)) ppu->_2001_lazy = byte;

        // if d7-d5 or d0 changed, we need update palette
        if ((ppu->regs[0x0001] & 0xe1) != (byte & 0xe1))
        {
            ppu_set_vdev_pal(ppu, byte & 0xe1);
        }
        break;

    case 0x0002:
        byte &= ~(1 << 7);
        byte |= pm->data[0x0002] & (1 << 7);
        break;

    case 0x0004:
        ppu->sprram[pm->data[0x0003]++] = byte;
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
        if (vaddr < 0x3f00) bus_writeb(nes->pbus, vaddr, byte);
        else
        {
            // for ppu palette mirroring
            vaddr &= 0x001f;
            if (vaddr == 0x0000 || vaddr == 0x0010)
            {
                ppu->palette[0x00] = byte;
                ppu->palette[0x04] = byte;
                ppu->palette[0x08] = byte;
                ppu->palette[0x0c] = byte;
                ppu->palette[0x10] = byte;
                ppu->palette[0x14] = byte;
                ppu->palette[0x18] = byte;
                ppu->palette[0x1c] = byte;
            }
            else if (vaddr & 0x3)
            {
                ppu->palette[vaddr] = byte;
            }
        }
        // increase vaddr by 1 or 32
        ppu->vaddr += (pm->data[0x0000] & (1 << 2)) ? 32 : 1;
        break;
    }

    // save reg write value
    pm->data[addr] = byte;
}


