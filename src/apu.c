// 包含头文件
#include "nes.h"

// 内部常量定义
#define APU_ABUF_NUM  16
#define APU_ABUF_LEN  3200

// 函数实现
void apu_init(APU *apu, DWORD extra)
{
    // create adev & request buffer
    apu->adevctxt = adev_create(APU_ABUF_NUM, APU_ABUF_LEN);

    // reset apu
    apu_reset(apu);
}

void apu_free(APU *apu)
{
    adev_destroy(apu->adevctxt);
}

void apu_reset(APU *apu)
{
    apu->pclk_frame = 0;
}

void apu_run_pclk(APU *apu, int pclk)
{
    if (apu->pclk_frame == 0) {
        adev_audio_buf_request(apu->adevctxt, &(apu->audiobuf));
    }

    //++ render audio data on audio buffer ++//
    // todo...
    //-- render audio data on audio buffer --//

    // add pclk to pclk_frame
    apu->pclk_frame += pclk;

    if (apu->pclk_frame == NES_HTOTAL * NES_VTOTAL) {
        adev_audio_buf_post(apu->adevctxt,  (apu->audiobuf));
        apu->pclk_frame = 0;
    }
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

