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

// 内部函数实现
static void ppu_set_vdev_pal(int flag)
{
    BYTE  pal[256 * 4];
    BYTE *psrc = DEF_PPU_PAL;
    BYTE *pend = psrc + 64 * 3;
    BYTE *pdst = pal;
    int   gray, i;

    if (flag)
    {
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
    }
    else
    {
        for (i=0; i<256; i++)
        {
            *pdst++ = *psrc++;
            *pdst++ = *psrc++;
            *pdst++ = *psrc++;
            *pdst++ = 0;
            if (psrc == pend) {
                psrc = DEF_PPU_PAL;
            }
        }
    }
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
    ppu->color_flag   = 0;
    ppu->_2005_toggle = 0;
    ppu->_2006_toggle = 0;
    ppu->pio_addr     = 0;
    ppu->ntabn        = 0;
    ppu->tilex        = 0;
    ppu->tiley        = 0;
    ppu->finex        = 0;
    ppu->finey        = 0;
    ppu_set_vdev_pal(0);
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

    case 0x0004:
        byte = nes->ppu.sprram[pm->data[0x0003]];
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
    NES *nes = container_of(pm, NES, ppuregs);
    switch (addr)
    {
    case 0x0000:
        nes->ppu.ntabn = (byte & 0x3);
        break;

    case 0x0001:
        if (nes->ppu.color_flag != (byte & (1 << 0)))
        {
            nes->ppu.color_flag = byte & (1 << 0);
            ppu_set_vdev_pal(nes->ppu.color_flag);
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
        bus_writeb(nes->pbus, nes->ppu.pio_addr, byte);
        if (pm->data[0x0000] & (1 << 2)) {
            nes->ppu.pio_addr += 32;
        }
        else nes->ppu.pio_addr++;
        break;
    }
}


