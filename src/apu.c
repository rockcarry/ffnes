// 包含头文件
#include "nes.h"

// 内部常量定义
#define APU_ABUF_NUM  16
#define APU_ABUF_LEN  3200

#define FRAME_DIVIDER        (NES_FREQ_PPU / 240)
#define MIXER_DIVIDER        (NES_HTOTAL*NES_VTOTAL / 800 + 1)
#define SCH0_WAVSEQ_DIVIDER  (48 * ((((apu->regs[0x0003] & 0x7) << 8) | apu->regs[0x0002]) + 1))
#define SCH0_ENVLOP_DIVIDER  ((apu->regs[0x0000] & 0x0f) + 1)

// 内部全局变量定义
static int length_table[32] =
{
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

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
    // reset frame sequencer
    apu->frame_interrupt = 0;
    apu->frame_divider   = FRAME_DIVIDER;
    apu->frame_counter   = 0;

    // reset square channel 0
    apu->sch0_length_counter = 0;
    apu->sch0_envlop_divider = SCH0_ENVLOP_DIVIDER;
    apu->sch0_envlop_counter = 0;
    apu->sch0_envlop_volume  = 0;
    apu->sch0_wavseq_divider = SCH0_WAVSEQ_DIVIDER;
    apu->sch0_wavseq_counter = 0;
    apu->sch0_output_value   = 0;
    apu->_4003_write_flag    = 0;

    apu->pclk_frame = 0;
}

void apu_run_pclk(APU *apu, int pclk)
{
    int f = 0;
    int l = 0;
    int e = 0;
    int w = 0;
    int i;

    if (apu->pclk_frame == 0) {
        // request audio buffer
        adev_audio_buf_request(apu->adevctxt, &(apu->audiobuf));

        // reset mixer sequencer
        apu->mixer_divider = MIXER_DIVIDER;
        apu->mixer_counter = 0;
    }

    //++ render audio data on audio buffer ++//
    for (i=0; i<pclk; i++)
    {
        //+ frame sequencer
        if (--apu->frame_divider == 0)
        {
            if (apu->regs[0x0017] & (1 << 7))
            {   // 5 step
                switch (apu->frame_counter)
                {
                case 0: e = 1; l = 1; break;
                case 1: e = 1;        break;
                case 2: e = 1; l = 1; break;
                case 3: e = 1;        break;
                }
                apu->frame_counter++;
                apu->frame_counter %= 5;
            }
            else // 4 step
            {
                switch (apu->frame_counter)
                {
                case 0: e = 1;        break;
                case 1: e = 1; l = 1; break;
                case 2: e = 1;        break;
                case 3: e = 1; l = 1; f = 1; break;
                }
                apu->frame_counter++;
                apu->frame_counter %= 4;
            }

            // frame interrupt
            if (f) apu->frame_interrupt = 1;

            // reload frame divider
            apu->frame_divider = FRAME_DIVIDER;
        }
        //- frame sequencer

        //++ square channel 1
        //+length counter
        // when clocked by the frame sequencer, if the halt flag is clear
        // and the counter is non-zero, it is decremented
        if (l && !(apu->regs[0x0000] & (1 << 5)))
        {
            if (apu->sch0_length_counter > 0) apu->sch0_length_counter--;
        }
        //-length counter

        //+envelope generator
        if (e) // envelope clocked by the frame sequencer
        {
            // if there was a write to the fourth channel register since the last clock
            if (apu->_4003_write_flag)
            {
                // the counter is set to 15 and the divider is reset
                apu->sch0_envlop_divider = SCH0_ENVLOP_DIVIDER;
                apu->sch0_envlop_counter = 15;
                apu->_4003_write_flag = 0;
            }
            else
            {
                // otherwise the divider is clocked
                if (--apu->sch0_envlop_divider == 0)
                {
                    // when the divider outputs a clock
                    // if loop is set and counter is zero, it is set to 15
                    if (apu->regs[0x0000] & (1 << 5) && apu->sch0_envlop_counter == 0)
                    {
                        apu->sch0_envlop_counter = 15;
                    }
                    // otherwise if counter is non-zero, it is decremented
                    else if (apu->sch0_envlop_counter > 0) apu->sch0_envlop_counter--;

                    // reloaded with the period
                    apu->sch0_envlop_divider = SCH0_ENVLOP_DIVIDER;
                }
            }

            // when constant volume is set, the channel's volume is n
            if (apu->regs[0x0000] & (1 << 4))
            {
                apu->sch0_envlop_volume = apu->regs[0x0000] & 0x0f;
            }
            // otherwise it is the value in the counter
            else apu->sch0_envlop_volume = apu->sch0_envlop_counter;
        }
        //-envelope generator

        if (--apu->sch0_wavseq_divider == 0)
        {
            apu->sch0_wavseq_counter++;
            apu->sch0_wavseq_counter %= 8;
            switch (apu->regs[0x0000] >> 6)
            {
            case 0: if (apu->sch0_wavseq_counter == 1) w = 1;
            case 1: if (apu->sch0_wavseq_counter >= 1 && apu->sch0_wavseq_counter <= 2) w = 1;
            case 2: if (apu->sch0_wavseq_counter >= 1 && apu->sch0_wavseq_counter <= 4) w = 1;
            case 3: if (apu->sch0_wavseq_counter == 0 && apu->sch0_wavseq_counter >= 3) w = 1;
            }

            if (w) apu->sch0_output_value = apu->sch0_length_counter ? apu->sch0_envlop_volume : 0;
            else   apu->sch0_output_value = 0;

            apu->sch0_wavseq_divider = SCH0_WAVSEQ_DIVIDER;
        }
        //-- square channel 1

        // for mixer ouput
        if (--apu->mixer_divider == 0)
        {
            ((DWORD*)apu->audiobuf->lpdata)[apu->mixer_counter++] = apu->sch0_output_value;
            apu->mixer_divider = MIXER_DIVIDER;
        }
    }
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
    NES *nes  = container_of(pm, NES, apuregs);
    BYTE byte = 0;

    switch (addr)
    {
    case 0x0015:
        if (nes->apu.frame_interrupt)
        {
            nes->apu.frame_interrupt = 0;
            byte |= (1 << 6);
        }
        if (nes->apu.sch0_length_counter > 0) byte |= (1 << 0);
        return byte;

    case 0x0016:
    case 0x0017:
        return NES_PAD_REG_RCB(pm, addr);

    default:
        return pm->data[addr];
    }
}

void NES_APU_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    int  i;
    NES *nes = container_of(pm, NES, apuregs);
    switch (addr)
    {
    case 0x0003:
        // unless disabled, write this register immediately reloads
        // the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 0))
        {
            nes->apu.sch0_length_counter = length_table[byte >> 3];
        }

        // set 4003 write flag to 1
        nes->apu._4003_write_flag = 1;
        break;

    case 0x0014:
        for (i=0; i<256; i++) {
            nes->ppu.sprram[i] = bus_readb(nes->cbus, byte * 256 + i);
        }
        nes->cpu.cclk_dma += 512;
        break;

    case 0x0015:
        if (!(byte & (1 << 0)))
        {
            nes->apu.sch0_length_counter = 0;
        }
        break;

    case 0x0016:
        NES_PAD_REG_WCB(pm, addr, byte);
        break;

    case 0x0017:
        NES_PAD_REG_WCB(pm, addr, byte);
        nes->apu.frame_divider = FRAME_DIVIDER;
        nes->apu.frame_counter = 0;
        break;
    }
}

