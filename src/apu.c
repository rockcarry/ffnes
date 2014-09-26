// 包含头文件
#include "nes.h"

// 内部常量定义
#define APU_ABUF_NUM  16
#define APU_ABUF_LEN  (799 * 4)

#define FRAME_DIVIDER       (NES_FREQ_PPU / 240)
#define MIXER_DIVIDER       (NES_HTOTAL*NES_VTOTAL / 800 + 1)
#define SCH_ENVLOP_DIVIDER  ((regs[0x0000] & 0x0f) + 1)
#define SCH_SWEEPU_DIVIDER  (((regs[0x0001] >> 4) & 0x7) + 1)
#define SCH_WAVSEQ_DIVIDER  (48 * ((((regs[0x0003] & 0x7) << 8) | regs[0x0002]) + 1))

// 内部全局变量定义
static int length_table[32] =
{
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

// 内部函数实现
static void apu_reset_square_channel(SQUARE_CHANNEL *sch, BYTE *regs)
{
    // reset square channel 0
    sch->length_counter = 0;
    sch->envlop_divider = SCH_ENVLOP_DIVIDER;
    sch->envlop_counter = 0;
    sch->envlop_volume  = 0;
    sch->sweepu_divider = SCH_SWEEPU_DIVIDER;
    sch->sweepu_value   = 0;
    sch->wavseq_divider = SCH_WAVSEQ_DIVIDER;
    sch->wavseq_counter = 0;
    sch->output_value   = 0;
    regs[0x0001]        = 0x08;
}

static void apu_render_square_channel(SQUARE_CHANNEL *sch, BYTE *regs, int fel)
{
    //+envelope generator
    if ((fel & (1 << 1))) // envelope clocked by the frame sequencer
    {
        // if the start flag is set, the counter is loaded with 15,
        // and the divider's period is immediately reloaded
        if (sch->envlop_reset)
        {
            sch->envlop_counter = 15;
            sch->envlop_divider = SCH_ENVLOP_DIVIDER;
            sch->envlop_reset   = 0;
        }
        else
        {
            // otherwise the envelope's divider is clocked
            if (--sch->envlop_divider == 0)
            {
                // when the envelope's divider outputs a clock
                // if the counter is non-zero, it is decremented
                if (sch->envlop_counter > 0) sch->envlop_counter--;

                // if loop is set and counter is zero, it is set to 15
                else if (regs[0x0000] & (1 << 5)) sch->envlop_counter = 15;

                // reloaded with the period
                sch->envlop_divider = SCH_ENVLOP_DIVIDER;
            }
        }

        // when constant volume is set, the channel's volume is n
        if (regs[0x0000] & (1 << 4))
        {
            sch->envlop_volume = regs[0x0000] & 0x0f;
        }
        // otherwise it is the value in the counter
        else sch->envlop_volume = sch->envlop_counter;
    }
    //-envelope generator

    //+sweep unit
    if ((fel & (1 << 0)))
    {
        // the divider is *first* clocked
        if (--sch->sweepu_divider == 0)
        {
            int negate   = (regs[0x0001] & (1 << 3)) ? -1 : 1;
            int shifterc = regs[0x0001] & 0x7;
            int shifterv = (SCH_SWEEPU_DIVIDER / 48) >> shifterc;
            int shiftert = sch->wavseq_divider + 48 * negate * shifterv;

            if (sch->wavseq_divider < 8 * 48 || shiftert > 0x7ff * 48) sch->sweepu_silence = 1;
            else if (regs[0x0001] & (1 << 7) && shifterc)
            {
                sch->sweepu_silence = 0;
                sch->wavseq_divider = shiftert;
            }

            sch->sweepu_divider = SCH_SWEEPU_DIVIDER;
        }

        // and then if there was a write to the sweep register since the last sweep
        // clock, the divider is reset.
        if (sch->sweepu_reset)
        {
            sch->sweepu_divider = SCH_SWEEPU_DIVIDER;
            sch->sweepu_reset   = 0;
        }
    }
    //-sweep unit

    //+length counter
    // when clocked by the frame sequencer, if the halt flag is clear
    // and the counter is non-zero, it is decremented
    if ((fel & (1 << 0)) && !(regs[0x0000] & (1 << 5)))
    {
        if (sch->length_counter > 0) sch->length_counter--;
    }
    //-length counter

    //+wave generator
    if (--sch->wavseq_divider == 0)
    {
        int w = 0;
        switch (regs[0x0000] >> 6)
        {
        case 0: if (sch->wavseq_counter == 1) w = 1;
        case 1: if (sch->wavseq_counter >= 1 && sch->wavseq_counter <= 2) w = 1;
        case 2: if (sch->wavseq_counter >= 1 && sch->wavseq_counter <= 4) w = 1;
        case 3: if (sch->wavseq_counter == 0 && sch->wavseq_counter >= 3) w = 1;
        }

        if (sch->length_counter && !sch->sweepu_silence && w)
        {
            sch->output_value = sch->envlop_volume;
        }
        else sch->output_value = 0;

        sch->wavseq_counter++;
        sch->wavseq_counter %= 8;

        // reload wave sequencer divider
        sch->wavseq_divider = SCH_WAVSEQ_DIVIDER;
    }
    //-wave generator
}

// 函数实现
void apu_init(APU *apu, DWORD extra)
{
    int i;

    // create adev & request buffer
    apu->adevctxt = adev_create(APU_ABUF_NUM, APU_ABUF_LEN);

    // create mixer lookup table
    apu->mixer_table_ss[0] = apu->mixer_table_tnd[0] = 0;
    for (i=1; i<31 ; i++) apu->mixer_table_ss [i] = (int)(0xffff * 95.52  / (8128.0  / i + 100));
    for (i=1; i<203; i++) apu->mixer_table_tnd[i] = (int)(0xffff * 163.67 / (24329.0 / i + 100));

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

    // reset square channel 1 & 2
    apu_reset_square_channel(&(apu->sch1), (BYTE*)apu->regs + 0);
    apu_reset_square_channel(&(apu->sch2), (BYTE*)apu->regs + 4);

    apu->pclk_frame = 0;
}

void apu_run_pclk(APU *apu, int pclk)
{
    int fel = 0;
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
                case 0: fel |= (3 << 0); break; //  el
                case 1: fel |= (1 << 0); break; //  e
                case 2: fel |= (3 << 0); break; //  el
                case 3: fel |= (1 << 0); break; //  e
                }
                apu->frame_counter++;
                apu->frame_counter %= 5;
            }
            else // 4 step
            {
                switch (apu->frame_counter)
                {
                case 0: fel |= (1 << 0); break; //  e
                case 1: fel |= (3 << 0); break; //  el
                case 2: fel |= (1 << 0); break; //  e
                case 3: fel |= (7 << 0); break; // fel
                }
                apu->frame_counter++;
                apu->frame_counter %= 4;
            }

            // frame interrupt
            if ((fel & (1 << 2)) && !(apu->regs[0x0017] & (1 << 6))) apu->frame_interrupt = 1;

            // reload frame divider
            apu->frame_divider = FRAME_DIVIDER;
        }
        //- frame sequencer

        // render square channel 1 & 2
        apu_render_square_channel(&(apu->sch1), (BYTE*)apu->regs + 0, fel);
        apu_render_square_channel(&(apu->sch2), (BYTE*)apu->regs + 4, fel);

        // for mixer ouput
        if (--apu->mixer_divider == 0)
        {
            int triangle = 0;
            int noise    = 0;
            int dmc      = 0;
            int squ_out  = apu->mixer_table_ss [apu->sch1.output_value + apu->sch2.output_value];
            int tnd_out  = apu->mixer_table_tnd[3 * triangle + 2 * noise + dmc];
            int sample   = squ_out + tnd_out - 0x7fff;
            ((DWORD*)apu->audiobuf->lpdata)[apu->mixer_counter++] = (sample << 16) | (sample << 0);
            apu->mixer_divider = MIXER_DIVIDER;
        }
    }
    //-- render audio data on audio buffer --//

    // add pclk to pclk_frame
    apu->pclk_frame += pclk;

    if (apu->pclk_frame == NES_HTOTAL * NES_VTOTAL) {
        ((DWORD*)apu->audiobuf->lpdata)[797] = ((DWORD*)apu->audiobuf->lpdata)[796];
        ((DWORD*)apu->audiobuf->lpdata)[798] = ((DWORD*)apu->audiobuf->lpdata)[796];
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
        //++ apu status register
        if (nes->apu.frame_interrupt)
        {
            nes->apu.frame_interrupt = 0;
            byte |= (1 << 6);
        }
        if (nes->apu.sch1.length_counter > 0) byte |= (1 << 0);
        if (nes->apu.sch2.length_counter > 0) byte |= (1 << 1);
        //++ apu status register
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
    case 0x0001:
        // if there was a write to the sweep register the reset flag is set
        nes->apu.sch1.sweepu_reset = 1;
        break;

    case 0x0003:
        // unless disabled, write this register immediately reloads
        // the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 0)) nes->apu.sch1.length_counter = length_table[byte >> 3];
        nes->apu.sch1.envlop_reset = 1; // set square channel's envelop reset flag
        break;

    case 0x0005:
        // if there was a write to the sweep register the reset flag is set
        nes->apu.sch2.sweepu_reset = 1;
        break;

    case 0x0007:
        // unless disabled, write this register immediately reloads
        // the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 1)) nes->apu.sch2.length_counter = length_table[byte >> 3];
        nes->apu.sch2.envlop_reset = 1; // set square channel's envelop reset flag
        break;

    case 0x0014:
        //++ for sprite dma
        for (i=0; i<256; i++) {
            nes->ppu.sprram[i] = bus_readb(nes->cbus, byte * 256 + i);
        }
        nes->cpu.cclk_dma += 512;
        //-- for sprite dma
        break;

    case 0x0015:
        //++ apu status register
        if (!(byte & (1 << 0))) nes->apu.sch1.length_counter = 0;
        if (!(byte & (1 << 1))) nes->apu.sch2.length_counter = 0;
        //-- apu status register
        break;

    case 0x0016:
        NES_PAD_REG_WCB(pm, addr, byte);
        break;

    case 0x0017:
        NES_PAD_REG_WCB(pm, addr, byte);

        //++ for frame sequencer
        nes->apu.frame_divider = FRAME_DIVIDER;
        nes->apu.frame_counter = 0;
        //-- for frame sequencer

        // interrupt inhibit flag. ff set, the frame interrupt flag is cleared
        if (byte & (1 << 6)) nes->apu.frame_interrupt = 0;

        // if the 5-step is selected the sequencer is immediately clocked once
        if (byte & (1 << 7))
        {
            apu_render_square_channel(&(nes->apu.sch1), (BYTE*)nes->apu.regs + 0, 0x03);
            apu_render_square_channel(&(nes->apu.sch2), (BYTE*)nes->apu.regs + 4, 0x03);
        }
        break;
    }
}

