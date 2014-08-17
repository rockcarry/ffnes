// 包含头文件
#include "nes.h"
#include "adev.h"

// 内部常量定义
#define APU_ABUF_NUM  16
#define APU_ABUF_LEN  3200

// 函数实现
void apu_init(APU *apu, DWORD extra)
{
    apu->adevctxt = adev_create(APU_ABUF_NUM, APU_ABUF_LEN);
}

void apu_free(APU *apu)
{
    adev_destroy(apu->adevctxt);
}

void apu_reset(APU *apu)
{
}

void apu_run_pclk(APU *apu, int pclk)
{
    AUDIOBUF *paudiobuf = NULL;
    adev_audio_buf_request(apu->adevctxt, &paudiobuf);
    adev_audio_buf_post   (apu->adevctxt,  paudiobuf);
}

BYTE NES_APU_REG_RCB(MEM *pm, int addr)
{
    return pm->data[addr];
}

void NES_APU_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    int  i;
    NES *nes = container_of(pm, NES, apuregs);
    switch (addr)
    {
    case 0x0014:
        for (i=0; i<256; i++) {
            nes->ppu.sprram[i] = bus_readb(nes->cbus, byte * 256 + i);
        }
        nes->cpu.cclk_dma += 512;
        break;
    }
}

