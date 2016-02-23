// 包含头文件
#include "nes.h"
#include "log.h"

// 预编译开关
// on real nes, sprite 0 hit detection is ignored at x=255
// but for scrolltest rom (scroll.nes), if detection is enabled
// at x=255, it will get a better performance, without any
// image flicker when you scrolling the screen.
// so I enable this feature for ffnes by default.
#define ENABLE_SPRITE0_HIT_ATX255 0

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
        if (psrc == pend) psrc = DEF_PPU_PAL;
    }
}

#define FINEX (ppu->finex)
#define FINEY (ppu->vaddr >> 12)
#define TILEX ((ppu->vaddr >> 0) & 0x1f)
#define TILEY ((ppu->vaddr >> 5) & 0x1f)
static void ppu_fetch_tile(PPU *ppu)
{
    NES *nes = container_of(ppu, NES, ppu);
    int  ntabn, noffs, aoffs, ndata, adata, ashift;

    // ntabn, noffs & aoffs
    ntabn = (ppu->vaddr >> 10) & 0x0003;
    noffs = 0x0000 + (ppu->vaddr & 0x03ff);
    aoffs = 0x03c0 + ((ppu->vaddr >> 4) & 0x38) + ((ppu->vaddr >> 2) & 0x07);

    // ndata, adata
    ndata = nes->vram[ntabn].data[noffs];
    adata = nes->vram[ntabn].data[aoffs];

    // tdata
    ppu->tdatal |= ppu->chrom_bkg[ndata * 16 + 8 * 0 + FINEY];
    ppu->tdatah |= ppu->chrom_bkg[ndata * 16 + 8 * 1 + FINEY];

    // ashift & pixelh
    ashift      = ((ppu->vaddr & (1 << 6)) >> 4) | ((ppu->vaddr & (1 << 1)));
    ppu->adata>>= 4;
    ppu->adata |= ((adata >> ashift) & 0x3) << 10;
}

// vaddr increment
static void ppu_vincrement(PPU *ppu)
{
    if ((ppu->vaddr & 0x1f) == 31)
    {
        ppu->vaddr &=~0x1f;
        ppu->vaddr ^= (1 << 10);
    }
    else ppu->vaddr++;
}

// x increment
static void ppu_xincrement(PPU *ppu)
{
    if (FINEX == 7)
    {
        FINEX = 0;
        ppu_fetch_tile(ppu);
        ppu_vincrement(ppu);
    }
    else FINEX++;
}

// y increment
static void ppu_yincrement(PPU *ppu)
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
    }
    else ppu->vaddr += 0x1000;
}

static void sprite_evaluate(PPU *ppu)
{
    NES  *nes = container_of(ppu, NES, ppu);
    int   sh  = (ppu->regs[0x0000] & (1 << 5)) ? 16 : 8;
    int   sy, tile, oy = 0, onum = 0;
    BYTE *chrrom, *sprsrc, *sprdst, *sprend;

    sprsrc = ppu->sprram;
    sprdst = ppu->sprbuf;
    sprend = sprsrc + 256;

    // sprite 0 hit flag should always clear when Y = 255
    if (sprsrc[0] == 255) ppu->regs[0x0002] &= ~(1 << 6);

    // for ppu->sprnum & ppu->sprzero
    ppu->sprnum  = 0;
    ppu->sprzero = NULL;
    if (ppu->scanline >= sprsrc[0] && ppu->scanline < sprsrc[0] + sh) ppu->sprzero = sprdst;

    do {
        if (ppu->scanline >= sprsrc[0] && ppu->scanline < sprsrc[0] + sh)
        {
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

            // sprnum++
            ppu->sprnum++;
        }

        //++ for sprite overflow obscure
        if (ppu->scanline >= sprsrc[oy] && ppu->scanline < sprsrc[oy] + sh) onum++;
        if (onum >= 9 ) { oy++; oy &= 3; }
        if (onum == 8 ) { onum++;        }
        if (onum == 10) { onum++; ppu->regs[0x0002] |= (1 << 5); }
        //-- for sprite overflow obscure

        // next sprite
        sprsrc += 4;
    } while (sprsrc < sprend && ppu->sprnum < 16);
}

static int sprite_render(PPU *ppu, int bkcolor)
{
    BYTE *sprdata = ppu->sprbuf + ppu->sprnum * 4;
    int   result  = bkcolor;
    int   scolor;

    while (sprdata > ppu->sprbuf)
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
                scolor = (sprdata[0] >> 7) | (sprdata[1] >> 7 << 1);
                sprdata[0] <<= 1;
                sprdata[1] <<= 1;
            }

            if (!(ppu->regs[0x0001] & (1 << 2)) && ppu->pclk_line < 8)
            {
                scolor = 0;
            }

            if (scolor)
            {
                if ((sprdata[2] & (1 << 6)) && (bkcolor & 0x3))
                {
                    result = bkcolor;
                }
                else
                {
                    result = (scolor | ((sprdata[2] >> 2) & 0xc)) + 16;
                }

                // update sprite 0 hit flag
#if ENABLE_SPRITE0_HIT_ATX255
                if (ppu->sprzero == sprdata && (bkcolor & 0x3))
#else
                if (ppu->sprzero == sprdata && ppu->pclk_line != 255 && (bkcolor & 0x3))
#endif
                {
                    ppu->regs[0x0002] |= (1 << 6);
                }
            }
            sprdata[2]++;
            sprdata[3]++;
        }
    }

    return result;
}

// 函数实现
void ppu_init(PPU *ppu, DWORD extra, VDEV *pdev)
{
    //++ for power up palette
    static BYTE power_up_pal[32] =
    {
        0x09,0x01,0x00,0x01,0x00,0x02,0x02,0x0D,0x08,0x10,0x08,0x24,0x00,0x00,0x04,0x2C,
        0x09,0x01,0x34,0x03,0x00,0x04,0x00,0x14,0x08,0x3A,0x00,0x02,0x00,0x20,0x2C,0x08,
    };
    //-- for power up palette

    // init vdev for ppu
    ppu->vdev = pdev ? pdev : &DEV_D3D;

    // create vdev for ppu
    ppu->vctxt = ppu->vdev->create(NES_WIDTH, NES_HEIGHT, extra);
    if (!ppu->vctxt) log_printf("ppu_init:: failed to create vdev !\n");

    // init power up palette
    memcpy(ppu->palette, power_up_pal, 32);

    // init clear ppu regs
    memset(ppu->regs, 0, 8);

    //++ for ppu open bus ++//
    memset(ppu->open_bust, 0, 8);
    ppu->open_busv = 0;
    //-- for ppu open bus --//

    // reset ppu
    ppu_reset(ppu);
}

void ppu_free(PPU *ppu)
{
    ppu->vdev->destroy(ppu->vctxt);
}

void ppu_reset(PPU *ppu)
{
    NES *nes = container_of(ppu, NES, ppu);

    ppu->regs[0]    = 0;
    ppu->regs[1]    = 0;
    ppu->regs[5]    = 0;
    ppu->pinvbl     = 1;
    ppu->toggle     = 0;
    ppu->finex      = 0;
    ppu->vaddr      = 0;
    ppu->temp0      = 0;
    ppu->temp1      = 0;
    ppu->_2007_lazy = 0;
    ppu->chrom_bkg  = (ppu->regs[0x0000] & (1 << 4)) ? nes->chrrom1.data : nes->chrrom0.data;
    ppu->chrom_spr  = (ppu->regs[0x0000] & (1 << 3)) ? nes->chrrom1.data : nes->chrrom0.data;
    ppu->pclk_frame = NES_HTOTAL * 242;
    ppu->pclk_line  = 0;
    ppu->oddevenflag= 0;
    ppu->scanline   = 0;

    // set default palette color
    ppu_set_vdev_pal(ppu, 0);
}

void ppu_run_pclk(PPU *ppu)
{
    //++ update vblank pin status
    if (  (ppu->pclk_frame >= NES_HTOTAL * 241 + 4 && ppu->pclk_frame <= NES_HTOTAL * 261)
       || (ppu->pclk_frame >= NES_HTOTAL * 261 + 4) )
    {
        ppu->pinvbl = ~(ppu->regs[0x0002] & ppu->regs[0x0000]) & (1 << 7);
    }
    //-- update vblank pin status

    // scanline 261 pre-render scanline
    if (ppu->pclk_frame == NES_HTOTAL * 261 + 0) // scanline 261, tick 0
    {
        ppu->regs[0x0002] &= ~(3 << 5);
    }
    else if (ppu->pclk_frame == NES_HTOTAL * 261 + 1) // scanline 261, tick 1
    {
        // clear vblank bit of reg $2002
        ppu->vblklast = ppu->regs[0x0002] & (1 << 7);
        ppu->regs[0x0002] &= ~(1 << 7);

        // if sprite or name-tables are visible.
        if (ppu->regs[0x0001] & (0x3 << 3))
        {
            // the ppu address copies the ppu's
            // temp at the beginning of the line
            ppu->vaddr = ppu->temp0;
            ppu->finex = ppu->temp1;
        }

        // request vdev buffer, obtain address & stride
        ppu->vdev->bufrequest(ppu->vctxt, (void**)&(ppu->draw_buffer), &(ppu->draw_stride));
    }

    // scanline 0 - 239 visible scanlines
    else if (ppu->pclk_frame < NES_HTOTAL * 240)
    {
        if (ppu->pclk_line < 256)
        {
            int pixel = 0; // default pixel value

            // fetch tile and do x increment
            if (ppu->regs[0x0001] & (0x3 << 3))
            {
                // according to ppu pipeline, on each visiable scanline
                //++ pre-fetch three times tile data for rendering
                if (ppu->pclk_line == 0)
                {
                    // clear it first
                    ppu->tdatal  = 0;
                    ppu->tdatah  = 0;

                    // tile data 1
                    ppu_fetch_tile(ppu);
                    ppu_vincrement(ppu);
                    ppu->tdatal<<= 8;
                    ppu->tdatah<<= 8;

                    // tile data 2
                    ppu_fetch_tile(ppu);
                    ppu_vincrement(ppu);
                    ppu->tdatal<<= 8;
                    ppu->tdatah<<= 8;

                    // tile data 3
                    ppu_fetch_tile(ppu);
                    ppu_vincrement(ppu);
                    ppu->tdatal<<= FINEX;
                    ppu->tdatah<<= FINEX;
                }
                //-- pre-fetch three times tile data for rendering

                // render background
                if (ppu->regs[0x0001] & (1 << 3))
                {
                    if ((ppu->regs[0x0001] & (1 << 1)) || ppu->pclk_line >= 8)
                    {
                        pixel = (ppu->tdatah << 8 >> 31 << 1) | (ppu->tdatal << 8 >> 31);
                        if (pixel) pixel |= (ppu->adata & 0xf);
                    }
                    ppu->tdatal <<= 1; ppu->tdatah <<= 1;
                }

                // render sprite
                if (ppu->regs[0x0001] & (1 << 4)) pixel = sprite_render(ppu, pixel);

                // do x increment
                ppu_xincrement(ppu);
            }
            else if ((ppu->vaddr & 0x3f00) == 0x3f00)
            {
                // pixel value for palette hack
                pixel = ppu->vaddr & 0x1f;
            }

            // write pixel on vdev
            *ppu->draw_buffer++ = ((DWORD*)ppu->vdevpal)[ppu->palette[pixel]];
        }
        else if (ppu->pclk_line == 321) // tick 321
        {
            if (ppu->regs[0x0001] & (0x3 << 3))
            {
                // evaluate sprite
                sprite_evaluate(ppu);

                // reget vaddr from temp0
                ppu->vaddr &= ~0x041f;
                ppu->vaddr |= (ppu->temp0 & 0x041f);
                ppu->finex  = ppu->temp1;

                // do y increment
                ppu_yincrement(ppu);
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
        {//++ for replay progress bar ++//
            NES *nes = container_of(ppu, NES, ppu);
            if (nes->replay.mode == NES_REPLAY_PLAY)
            {
                int i = 256 * nes->replay.curpos / nes->replay.total;
                ppu->draw_buffer -= ppu->draw_stride;
                while (i--) ppu->draw_buffer[i] = RGB(255, 128, 255);
            }
        }//-- for replay progress bar --//

        // post vdev buffer
        ppu->vdev->bufpost(ppu->vctxt);

        // set vblank bit of reg $2002
        ppu->vblklast = ppu->regs[0x0002] & (1 << 7);
        ppu->regs[0x0002] |= (1 << 7);
    }

    // the last tick of frame, odd frame with tick skipping
    else if (ppu->pclk_frame == NES_HTOTAL * NES_VTOTAL - 2)
    {
        //++ for ppu open bus ++//
        int i = 7;
        do {
            if (ppu->open_bust[i] > 0)
            {
                if (--ppu->open_bust[i] == 0)
                {
                    ppu->open_busv &= ~(1 << i);
                }
            }
        } while (i--);
        //-- for ppu open bus --//

        if (ppu->oddevenflag && (ppu->regs[0x0001] & (0x3 << 3)))
        {
            // frame change
            ppu->pclk_frame = 0;
            ppu->pclk_line  = 0;
            ppu->scanline   = 0;
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
        ppu->scanline    = 0;
        if (ppu->regs[0x0001] & (0x3 << 3)) {
            ppu->oddevenflag = 1; // next frame is odd frame
        }
        return;
    }

    // pclk increment
    ppu->pclk_frame++;
    ppu->pclk_line ++;
    if (ppu->pclk_line == NES_HTOTAL)
    {
        // scanline change
        ppu->pclk_line = 0;
        ppu->scanline++;
    }
}

BYTE NES_PPU_REG_RCB(MEM *pm, int addr)
{
    NES *nes   = container_of(pm, NES, ppuregs);
    PPU *ppu   = &(nes->ppu);
    BYTE byte  = pm->data[addr];
    int  vaddr = ppu->vaddr & 0x3fff;

    //++ for ppu open bus ++//
    int  mask, i;
    static BYTE open_bus_mask[8] = { 0xff, 0xff, 0x1f, 0xff, 0x00, 0xff, 0xff, 0x00 };
    mask = open_bus_mask[addr];
    //-- for ppu open bus --//

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
        // read sprite attributes byte, d2-d4 always 0
        if ((pm->data[0x0003] & 0x3) == 2) byte &= ~(0x7 << 2);
        break;

    case 0x0007:
        if (vaddr < 0x3f00)
        {
            byte            = ppu->_2007_lazy;
            ppu->_2007_lazy = bus_readb(nes->pbus, vaddr);
        }
        else
        {
            //++ for ppu open bus & palette read ++//
            mask            = 0xc0;
            byte            = bus_readb(nes->pbus, vaddr) & 0x3f;
            byte           |= ppu->open_busv & 0xc0;
            //-- for ppu open bus & palette read --//
            ppu->_2007_lazy = bus_readb(nes->pbus, vaddr & 0x2fff); // palette addr dummy read
        }
        // increase vaddr by 1 or 32
        ppu->vaddr += (pm->data[0x0000] & (1 << 2)) ? 32 : 1;
        break;
    }

    //++ for ppu open bus ++//
    byte &=~mask;
    byte |= mask & ppu->open_busv;
    ppu->open_busv &= mask;
    ppu->open_busv |=~mask & byte;
    for (i=0; i<8; i++) if (~mask & (1 << i)) ppu->open_bust[i] = 60;
    //-- for ppu open bus --//

    return byte;
}

void NES_PPU_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    NES *nes   = container_of(pm, NES, ppuregs);
    PPU *ppu   = &(nes->ppu);
    int  vaddr = ppu->vaddr & 0x3fff;

    //++ for ppu open bus ++//
    ppu->open_busv = byte;
    memset(ppu->open_bust, 60, 8);
    //-- for ppu open bus --//

    switch (addr)
    {
    case 0x0000:
        ppu->temp0 &=~(0x03 << 10);
        ppu->temp0 |= (byte & 0x03) << 10;
        ppu->chrom_bkg = (byte & (1 << 4)) ? nes->chrrom1.data : nes->chrrom0.data;
        ppu->chrom_spr = (byte & (1 << 3)) ? nes->chrrom1.data : nes->chrrom0.data;
        break;

    case 0x0001:
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
            //+ for ppu palette mirroring
            vaddr &= 0x1f;
            byte  &= 0x3f;
            if (vaddr & 0x03) ppu->palette[vaddr] = byte;
            else
            {
                ppu->palette[vaddr & ~0x10] = byte;
                ppu->palette[vaddr |  0x10] = byte;
            }
            //- for ppu palette mirroring
        }
        // increase vaddr by 1 or 32
        ppu->vaddr += (pm->data[0x0000] & (1 << 2)) ? 32 : 1;
        break;
    }

    // save reg write value
    pm->data[addr] = byte;
}


