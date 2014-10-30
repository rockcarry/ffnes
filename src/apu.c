// 包含头文件
#include "nes.h"

// 内部常量定义
#define APU_ABUF_NUM  16
#define APU_ABUF_LEN  (797 * 4)

#define FRAME_DIVIDER       (NES_FREQ_PPU / 240)
#define MIXER_DIVIDER       (NES_HTOTAL*NES_VTOTAL / 800 + 1)
#define SCH_ENVLOP_DIVIDER  ((regs[0x0000] & 0xf) + 1)
#define SCH_SWEEPU_DIVIDER  (((regs[0x0001] >> 4) & 0x7) + 1)
#define SCH_STIMER_DIVIDER  (6 * ((((regs[0x0003] & 0x7) << 8) | regs[0x0002]) + 1))
#define TCH_TTIMER_DIVIDER  (3 * ((((regs[0x0003] & 0x7) << 8) | regs[0x0002]) + 1))

// 内部全局变量定义
static BYTE length_table[32] =
{
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

// 内部函数实现
static void apu_reset_square_channel(SQUARE_CHANNEL *sch, BYTE *regs)
{
    // reset square channel
    sch->length_counter = 0;
    sch->envlop_divider = SCH_ENVLOP_DIVIDER;
    sch->envlop_counter = 0;
    sch->envlop_volume  = 0;
    sch->sweepu_divider = SCH_SWEEPU_DIVIDER;
    sch->sweepu_value   = 0;
    sch->sweepu_reset   = 0;
    sch->sweepu_silence = 0;
    sch->stimer_divider = SCH_STIMER_DIVIDER;
    sch->schseq_counter = 0;
    sch->output_value   = 0;
    regs[0x0001]        = 0x08;
}

static void apu_reset_triangle_channel(TRIANGLE_CHANNEL *tch, BYTE *regs)
{
    // reset triangle channel
    tch->ttimer_divider = TCH_TTIMER_DIVIDER;
    tch->ttimer_output  = 0;
    tch->linear_haltflg = 1;
    tch->linear_counter = 0;
    tch->length_counter = 0;
    tch->tchseq_counter = 0;
    tch->output_value   = 0;
}

static void apu_reset_noise_channel(NOISE_CHANNEL *nch, BYTE *regs)
{
    // reset noise channel
    nch->output_value = 0;
}

static void apu_reset_dmc_channel(DMC_CHANNEL *dmc, BYTE *regs)
{
    // reset dmc channel
    dmc->output_value = 0;
}

static void apu_render_square_channel(SQUARE_CHANNEL *sch, BYTE *regs, int flew)
{
    if ((flew & (1 << 1))) // envelope clocked by the frame sequencer
    {
        //++ envelope generator
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
        //-- envelope generator
    }

    if ((flew & (1 << 2)))
    {
        //++ sweep unit
        // the divider is *first* clocked
        if (--sch->sweepu_divider == 0)
        {
            int negate    = (regs[0x0001] & (1 << 3)) ? -1 : 1;
            int shifterc  = regs[0x0001] & 0x7;
            int shifterv  = (SCH_SWEEPU_DIVIDER / 48) >> shifterc;
            int tarperiod = sch->stimer_divider + 48 * negate * shifterv;

            if (SCH_STIMER_DIVIDER < 8 * 48 || tarperiod > 0x7ff * 48) sch->sweepu_silence = 1;
            else
            {
                if (regs[0x0001] & (1 << 7) && shifterc)
                {
                    sch->stimer_divider = tarperiod;
                }
                sch->sweepu_silence = 0;
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
        //-- sweep unit

        //++ length counter
        // when clocked by the frame sequencer, if the halt flag is clear
        // and the counter is non-zero, it is decremented
        if (!(regs[0x0000] & (1 << 5)))
        {
            if (sch->length_counter > 0) sch->length_counter--;
        }
        //-- length counter
    }

    if (flew & (1 << 0))
    {
        //++ square channel sequencer
        if (--sch->stimer_divider == 0)
        {
            int w = -1;
            switch (regs[0x0000] >> 6)
            {
            case 0: if (sch->schseq_counter == 1                            ) w = 1; break;
            case 1: if (sch->schseq_counter == 1 || sch->schseq_counter == 2) w = 1; break;
            case 2: if (sch->schseq_counter >= 1 && sch->schseq_counter <= 4) w = 1; break;
            case 3: if (sch->schseq_counter == 0 || sch->schseq_counter >= 3) w = 1; break;
            }

            if (sch->length_counter && !sch->sweepu_silence)
            {
                sch->output_value = w * sch->envlop_volume;
            }
            else sch->output_value = 0;

            sch->schseq_counter++;
            sch->schseq_counter %= 8;

            // reload square channel timer divider
            sch->stimer_divider = SCH_STIMER_DIVIDER;
        }
        //-- square channel sequencer
    }
}

static void apu_render_triangle_channel(TRIANGLE_CHANNEL *tch, BYTE *regs, int flew)
{
    if (flew & (1 << 0))
    {
        //++ timer generator
        if (--tch->ttimer_divider == 0)
        {
            // output clock
            tch->ttimer_output  = 1;

            // reload wave sequencer divider
            tch->ttimer_divider = TCH_TTIMER_DIVIDER;
        }
        else tch->ttimer_output = 0;
        //-- timer generator
    }

    if ((flew & (1 << 1)))
    {
        //++ linear counter
        // if halt flag is set, set counter to reload value,
        // otherwise if counter is non-zero, decrement it.
        if (tch->linear_haltflg) tch->linear_counter = regs[0x0000] & 0x7f;
        else if (tch->linear_counter > 0) tch->linear_counter--;

        // if control flag is clear, clear halt flag.
        if (!(regs[0x0000] & (1 << 7))) tch->linear_haltflg = 0;
        //-- linear counter
    }

    if ((flew & (1 << 2)))
    {
        //++ length counter
        // when clocked by the frame sequencer, if the halt flag is clear
        // and the counter is non-zero, it is decremented
        if (!(regs[0x0000] & (1 << 7)))
        {
            if (tch->length_counter > 0) tch->length_counter--;
        }
        //-- length counter
    }

    if (tch->ttimer_output && tch->linear_counter && tch->length_counter)
    {
        //-- triangle channel sequencer
        if ((regs[0x0003] & 0x7) + regs[0x0002] < 2)
        {
            // at the lowest two periods ($400B = 0 and $400A = 0 or 1),
            // the resulting frequency is so high that the DAC effectively
            // outputs a value half way between 7 and 8.
            tch->output_value = 7 + (tch->tchseq_counter & 1);
        }
        else
        {
            static char tch_seq_out_tab[32] =
            {
                15, 13, 11, 9, 7, 5, 3, 1,-1,-3,-5,-7,-9,-11,-13,-15,
               -15,-13,-11,-9,-7,-5,-3,-1, 1, 3, 5, 7, 9, 11, 13, 15,
            };
            tch->output_value = tch_seq_out_tab[tch->tchseq_counter];
        }
        tch->tchseq_counter++;
        tch->tchseq_counter %= 32;
        //-- triangle channel sequencer
    }
//  else tch->output_value = 0;
}

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
    // reset pclk_frame value
    apu->pclk_frame = 0;

    // reset frame sequencer
    apu->frame_interrupt = 0;
    apu->frame_divider   = FRAME_DIVIDER;
    apu->frame_counter   = 0;

    // reset square channel 1 & 2
    apu_reset_square_channel  (&(apu->sch1), (BYTE*)apu->regs + 0x0000);
    apu_reset_square_channel  (&(apu->sch2), (BYTE*)apu->regs + 0x0004);
    apu_reset_triangle_channel(&(apu->tch ), (BYTE*)apu->regs + 0x0008);
    apu_reset_noise_channel   (&(apu->nch ), (BYTE*)apu->regs + 0x000c);
    apu_reset_dmc_channel     (&(apu->dmc ), (BYTE*)apu->regs + 0x0010);
}

void apu_run_pclk(APU *apu)
{
    int flew = (1 << 0);

    if (apu->pclk_frame == 0) {
        // request audio buffer
        adev_audio_buf_request(apu->adevctxt, &(apu->audiobuf));

        // reset mixer sequencer
        apu->mixer_divider = MIXER_DIVIDER;
        apu->mixer_counter = 0;
    }

    //++ render audio data on audio buffer ++//
    //+ frame sequencer
    if (--apu->frame_divider == 0)
    {
        if (apu->regs[0x0017] & (1 << 7))
        {   // 5 step
            switch (apu->frame_counter)
            {
            case 0: flew |= (3 << 1); break; //  le
            case 1: flew |= (1 << 1); break; //   e
            case 2: flew |= (3 << 1); break; //  le
            case 3: flew |= (1 << 1); break; //   e
            }
            apu->frame_counter++;
            apu->frame_counter %= 5;
        }
        else // 4 step
        {
            switch (apu->frame_counter)
            {
            case 0: flew |= (1 << 1); break; //   e
            case 1: flew |= (3 << 1); break; //  le
            case 2: flew |= (1 << 1); break; //   e
            case 3: flew |= (7 << 1); break; // fle
            }
            apu->frame_counter++;
            apu->frame_counter %= 4;
        }

        // frame interrupt
        if ((flew & (1 << 3)) && !(apu->regs[0x0017] & (1 << 6))) apu->frame_interrupt = 1;

        // reload frame divider
        apu->frame_divider = FRAME_DIVIDER;
    }
    //- frame sequencer

    // render square channel 1 & 2
    apu_render_square_channel  (&(apu->sch1), (BYTE*)apu->regs + 0, flew);
    apu_render_square_channel  (&(apu->sch2), (BYTE*)apu->regs + 4, flew);
    apu_render_triangle_channel(&(apu->tch ), (BYTE*)apu->regs + 8, flew);

    // for mixer ouput
    if (--apu->mixer_divider == 0)
    {
        short sample = (short)( 247 *(apu->sch1.output_value + apu->sch2.output_value)
                              + 279 * apu->tch.output_value
                              + 162 * apu->nch.output_value
                              + 110 * apu->dmc.output_value);
        ((DWORD*)apu->audiobuf->lpdata)[apu->mixer_counter++] = (sample << 16) | (sample << 0);
        apu->mixer_divider = MIXER_DIVIDER;
    }
    //-- render audio data on audio buffer --//

    if (++apu->pclk_frame == NES_HTOTAL * NES_VTOTAL) {
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
        return replay_run(&(nes->replay), NES_PAD_REG_RCB(pm, addr));

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

    case 0x000B:
        // unless disabled, write this register immediately reloads
        // the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 2)) nes->apu.tch.length_counter = length_table[byte >> 3];
        nes->apu.tch.linear_haltflg = 1; // when register $400B is written to, the halt flag is set.
        break;

    case 0x0014:
        //++ for sprite dma
        for (i=0; i<256; i++) {
            nes->ppu.sprram[nes->ppu.regs[0x0003]++] = bus_readb(nes->cbus, byte * 256 + i);
        }
        nes->cpu.cclk_dma += 512;
        //-- for sprite dma
        break;

    case 0x0015:
        //++ apu status register
        if (!(byte & (1 << 0))) nes->apu.sch1.length_counter = 0;
        if (!(byte & (1 << 1))) nes->apu.sch2.length_counter = 0;
        if (!(byte & (1 << 2))) nes->apu.tch .length_counter = 0;
        //-- apu status register
        break;

    case 0x0016:
        // call joypad memrw callback
        NES_PAD_REG_WCB(pm, addr, byte);
        return; // return directly

    case 0x0017:
        //++ for frame sequencer
        nes->apu.frame_divider = FRAME_DIVIDER;
        nes->apu.frame_counter = 0;
        //-- for frame sequencer

        // interrupt inhibit flag. ff set, the frame interrupt flag is cleared
        if (byte & (1 << 6)) nes->apu.frame_interrupt = 0;

        // if the 5-step is selected the sequencer is immediately clocked once
        if (byte & (1 << 7))
        {
            apu_render_square_channel  (&(nes->apu.sch1), (BYTE*)nes->apu.regs + 0, 6);
            apu_render_square_channel  (&(nes->apu.sch2), (BYTE*)nes->apu.regs + 4, 6);
            apu_render_triangle_channel(&(nes->apu.tch ), (BYTE*)nes->apu.regs + 8, 6);
        }

        // call joypad memrw callback
        NES_PAD_REG_WCB(pm, addr, byte);
        return; // return directly
    }

    // save reg write value
    pm->data[addr] = byte;
}

