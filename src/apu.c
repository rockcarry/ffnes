// 包含头文件
#include "nes.h"

// 内部常量定义
#define APU_ABUF_NUM   6
#define APU_ABUF_LEN  (735 * 4)

#define FRAME_DIVIDER       (NES_FREQ_PPU / 240)
#define MIXER_DIVIDER       (NES_HTOTAL*NES_VTOTAL*2 / 735)
#define SCH_SWEEPU_DIVIDER  (((regs[0x0001] >> 4) & 0x7) + 1)
#define SCH_STIMER_DIVIDER  (6 * ((((regs[0x0003] & 0x7) << 8) | regs[0x0002]) + 1))
#define TCH_TTIMER_DIVIDER  (3 * ((((regs[0x0003] & 0x7) << 8) | regs[0x0002]) + 1))
#define NCH_NTIMER_DIVIDER  (3 * (noise_timer_period_table[regs[0x0002] & 0xf]))
#define DCH_DTIMER_DIVIDER  (3 * (dmc_timer_period_table  [regs[0x0000] & 0xf]))

// 内部全局变量定义
static BYTE length_table[32] =
{
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

static short noise_timer_period_table[16] =
{
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

static short dmc_timer_period_table[16] =
{
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
};

// 内部函数实现
//+++ for envelope unit +++//
#define ENVELOPE_VOLUME(env)  (env[0])
#define ENVELOPE_RESET(env)   (env[1])
#define ENVELOPE_DIVIDER(env) (env[2])
#define ENVELOPE_COUNTER(env) (env[3])
#define ENVELOPE_DRELOAD      ((reg & 0xf) + 1)
static void apu_envelope_unit_reset(int *env, BYTE reg)
{
    ENVELOPE_VOLUME(env)  = 0;
    ENVELOPE_RESET(env)   = 0;
    ENVELOPE_DIVIDER(env) = ENVELOPE_DRELOAD;
    ENVELOPE_COUNTER(env) = 0;
}

static void apu_envelope_unit_clocked(int *env, BYTE reg)
{
    //++ envelope generator
    // if the start flag is set, the counter is loaded with 15,
    // and the divider's period is immediately reloaded
    if (ENVELOPE_RESET(env))
    {
        ENVELOPE_COUNTER(env) = 15;
        ENVELOPE_DIVIDER(env) = ENVELOPE_DRELOAD;
        ENVELOPE_RESET(env)   = 0;
    }
    else
    {
        // otherwise the envelope's divider is clocked
        if (--ENVELOPE_DIVIDER(env) == 0)
        {
            // when the envelope's divider outputs a clock
            // if the counter is non-zero, it is decremented
            if (ENVELOPE_COUNTER(env) > 0) ENVELOPE_COUNTER(env)--;

            // if loop is set and counter is zero, it is set to 15
            else if (reg & (1 << 5)) ENVELOPE_COUNTER(env) = 15;

            // reloaded with the period
            ENVELOPE_DIVIDER(env) = ENVELOPE_DRELOAD;
        }
    }

    // when constant volume is set, the channel's volume is n
    if (reg & (1 << 4))
    {
        ENVELOPE_VOLUME(env) = reg & 0x0f;
    }
    // otherwise it is the value in the counter
    else ENVELOPE_VOLUME(env) = ENVELOPE_COUNTER(env);
    //-- envelope generator
}
//--- for envelope unit ---//

static void apu_reset_square_channel(SQUARE_CHANNEL *sch, BYTE *regs)
{
    // reset square channel
    sch->length_counter = 0;
    sch->sweepu_divider = SCH_SWEEPU_DIVIDER;
    sch->sweepu_value   = 0;
    sch->sweepu_reset   = 0;
    sch->sweepu_silence = 0;
    sch->stimer_divider = SCH_STIMER_DIVIDER;
    sch->schseq_counter = 0;
    sch->output_value   = 0;
    apu_envelope_unit_reset(sch->envlop_unit, regs[0x0000]);
}

static void apu_reset_triangle_channel(TRIANGLE_CHANNEL *tch, BYTE *regs)
{
    // reset triangle channel
    tch->length_counter = 0;
    tch->linear_haltflg = 0;
    tch->linear_counter = 0;
    tch->ttimer_divider = TCH_TTIMER_DIVIDER;
    tch->tchseq_counter = 0;
    tch->output_value   = 0;
}

static void apu_reset_noise_channel(NOISE_CHANNEL *nch, BYTE *regs)
{
    // reset noise channel
    nch->length_counter = 0;
    nch->ntimer_divider = NCH_NTIMER_DIVIDER;
    nch->output_value   = 0;
    apu_envelope_unit_reset(nch->envlop_unit, regs[0x0000]);
}

static void apu_reset_dmc_channel(DMC_CHANNEL *dmc, BYTE *regs)
{
    // reset dmc channel
    dmc->remain_bytes   = 0;
    dmc->remain_bits    = 0;
    dmc->sample_buffer  = 0;
    dmc->sample_empty   = 1;
    dmc->buffer_bits    = 0;
    dmc->curdma_addr    = 0xC000;
    dmc->dmc_silence    = 0;
    dmc->dtimer_divider = DCH_DTIMER_DIVIDER;
    dmc->output_value   = 0;
}

static void apu_render_square_channel(SQUARE_CHANNEL *sch, BYTE *regs, int flew)
{
    if ((flew & (1 << 1)))
    {
        // envelope clocked by the frame sequencer
        apu_envelope_unit_clocked(sch->envlop_unit, regs[0x0000]);
    }

    if ((flew & (1 << 2)))
    {
        //++ sweep unit
        // the divider is *first* clocked
        if (--sch->sweepu_divider == 0)
        {
            int negate = (regs[0x0001] & 0x8) ? -1 : 1;
            int shift  = (regs[0x0001] & 0x7);
            int period = ((regs[0x0003] & 0x7) << 8) | regs[0x0002];
            int target = period + negate * (period >> shift);

            if (period < 8 || target > 0x7ff) sch->sweepu_silence = 1;
            else
            {
                if (shift && (regs[0x0001] & (1 << 7)))
                {
                    regs[0x0002]  = target & 0xff;
                    regs[0x0003] &= ~0x7;
                    regs[0x0003] |= target >> 8;
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
            if (sch->length_counter && !sch->sweepu_silence)
            {
                int w = -1;
                switch (regs[0x0000] >> 6)
                {
                case 0: if (sch->schseq_counter == 1                            ) w = 1; break;
                case 1: if (sch->schseq_counter == 1 || sch->schseq_counter == 2) w = 1; break;
                case 2: if (sch->schseq_counter >= 1 && sch->schseq_counter <= 4) w = 1; break;
                case 3: if (sch->schseq_counter == 0 || sch->schseq_counter >= 3) w = 1; break;
                }
                sch->output_value = w * ENVELOPE_VOLUME(sch->envlop_unit);
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

    if (flew & (1 << 0))
    {
        //++ timer generator
        if (--tch->ttimer_divider == 0)
        {
            //-- triangle channel sequencer
            if (tch->length_counter && tch->linear_counter)
            {
                if ((regs[0x0003] & 0x7) + regs[0x0002] < 2)
                {
                    // at the lowest two periods ($400B = 0 and $400A = 0 or 1),
                    // the resulting frequency is so high that the DAC effectively
                    // outputs a value half way between 7 and 8.
                    tch->output_value = (tch->tchseq_counter & 1) ? -1 : 1;
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
            }
            tch->tchseq_counter++;
            tch->tchseq_counter %= 32;
            //-- triangle channel sequencer

            // reload triangle channel timer divider
            tch->ttimer_divider = TCH_TTIMER_DIVIDER;
        }
        //-- timer generator
    }
}

static void apu_render_noise_channel(NOISE_CHANNEL *nch, BYTE *regs, int flew)
{
    if ((flew & (1 << 1)))
    {
        // envelope clocked by the frame sequencer
        apu_envelope_unit_clocked(nch->envlop_unit, regs[0x0000]);
    }

    if ((flew & (1 << 2)))
    {
        //++ length counter
        // when clocked by the frame sequencer, if the halt flag is clear
        // and the counter is non-zero, it is decremented
        if (!(regs[0x0000] & (1 << 5)))
        {
            if (nch->length_counter > 0) nch->length_counter--;
        }
        //-- length counter
    }

    if (flew & (1 << 0))
    {
        //++ timer generator
        if (--nch->ntimer_divider == 0)
        {
            int xor;
            if (regs[0x0002] & (1 << 7))
            {
                xor = (nch->nshift_register ^ (nch->nshift_register >> 6)) & 1;
            }
            else
            {
                xor = (nch->nshift_register ^ (nch->nshift_register >> 1)) & 1;
            }
            nch->nshift_register >>= 1;
            nch->nshift_register  |= (xor << 14);

            if (nch->length_counter)
            {
                if (nch->nshift_register & 1) nch->output_value =  ENVELOPE_VOLUME(nch->envlop_unit);
                else                          nch->output_value = -ENVELOPE_VOLUME(nch->envlop_unit);
            }
            else nch->output_value = 0;

            // reload noise channel timer divider
            nch->ntimer_divider = NCH_NTIMER_DIVIDER;
        }
        //-- timer generator
    }
}

static void apu_render_dmc_channel(DMC_CHANNEL *dch, BYTE *regs, int flew)
{
    if (flew & (1 << 0))
    {
        //++ timer generator
        if (--dch->dtimer_divider == 0)
        {
            APU *apu = container_of(dch, APU, dmc);
            NES *nes = container_of(apu, NES, apu);

            if (dch->sample_empty && dch->remain_bytes > 0)
            {
                nes->cpu.cclk_dma += 4;
                dch->sample_buffer = bus_readb(nes->cbus, dch->curdma_addr);
                dch->sample_empty  = 0;

                // the address is incremented; if it exceeds $FFFF, it is wrapped around to $8000.
                if (++dch->curdma_addr > 0xffff) dch->curdma_addr = 0x8000;

                // the bytes remaining counter is decremented;
                if (--dch->remain_bytes == 0)
                {
                    // if the bytes counter becomes zero and the loop flag is
                    // set, the sample is restarted
                    if (regs[0x0000] & (1 << 6))
                    {
                        // restart
                        nes->apu.dmc.curdma_addr  = 0xC000 + (apu->regs[0x0012] << 6);
                        nes->apu.dmc.remain_bytes = (apu->regs[0x0013] << 4) + 1;
                    }
                    // otherwise if the bytes counter becomes zero and the interrupt
                    // enabled flag is set, the interrupt flag is set.
                    else if (regs[0x0000] & (1 << 7)) regs[0x0005] |= (1 << 7);
                }
            }

            if (dch->remain_bits == 0)
            {
                if (dch->sample_empty) dch->dmc_silence = 1;
                else
                {
                    dch->remain_bits  = dch->sample_buffer;
                    dch->dmc_silence  = 0;
                    dch->sample_empty = 1;
                }
                dch->remain_bits = 8;
            }

            if (!dch->dmc_silence)
            {
                if (dch->buffer_bits & 1)
                {
                    if (dch->output_value <= 123) dch->output_value += 4;
                }
                else
                {
                    if (dch->output_value >=-123) dch->output_value -= 4;
                }
            }

            dch->buffer_bits >>= 1;
            dch->remain_bits--;

            // reload dmc channel timer divider
            dch->dtimer_divider = DCH_DTIMER_DIVIDER;
        }
    }
}

// 函数实现
void apu_init(APU *apu, DWORD extra)
{
    // create adev & request buffer
    apu->adevctxt = adev_create(APU_ABUF_NUM, APU_ABUF_LEN);

    // on power-up, the shift register is loaded with the value 1
    apu->nch.nshift_register = 1;

    // reset apu
    apu_reset(apu);
}

void apu_free(APU *apu)
{
    adev_destroy(apu->adevctxt);
}

void apu_reset(APU *apu)
{
    // after reset, $4015 should be cleared
    apu->regs[0x0015] = 0;

    // reset pclk_frame value
    apu->pclk_frame = 0;

    // reset frame sequencer
    apu->frame_divider = FRAME_DIVIDER;
    apu->frame_counter = 0;

    // reset square channel 1 & 2
    apu_reset_square_channel  (&(apu->sch1), (BYTE*)apu->regs + 0x0000);
    apu_reset_square_channel  (&(apu->sch2), (BYTE*)apu->regs + 0x0004);
    apu_reset_triangle_channel(&(apu->tch ), (BYTE*)apu->regs + 0x0008);
    apu_reset_noise_channel   (&(apu->nch ), (BYTE*)apu->regs + 0x000C);
    apu_reset_dmc_channel     (&(apu->dmc ), (BYTE*)apu->regs + 0x0010);
}

void apu_run_pclk(APU *apu)
{
    int flew = (1 << 0);
    int i    = 2;

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
        if ((flew & (1 << 3)) && !(apu->regs[0x0017] & (1 << 6))) apu->regs[0x0015] |= (1 << 6);

        // reload frame divider
        apu->frame_divider = FRAME_DIVIDER;
    }
    //- frame sequencer

    // render square channel 1 & 2
    apu_render_square_channel  (&(apu->sch1), (BYTE*)apu->regs + 0x0000, flew);
    apu_render_square_channel  (&(apu->sch2), (BYTE*)apu->regs + 0x0004, flew);
    apu_render_triangle_channel(&(apu->tch ), (BYTE*)apu->regs + 0x0008, flew);
    apu_render_noise_channel   (&(apu->nch ), (BYTE*)apu->regs + 0x000C, flew);
    apu_render_dmc_channel     (&(apu->dmc ), (BYTE*)apu->regs + 0x0010, flew);

    // for mixer ouput
    do {
        if (--apu->mixer_divider == 0)
        {
            short sample = (short)( 247 *(apu->sch1.output_value + apu->sch2.output_value)
                + 279 * apu->tch.output_value
                + 162 * apu->nch.output_value
                + 110 * apu->dmc.output_value);
            ((DWORD*)apu->audiobuf->lpdata)[apu->mixer_counter++] = (sample << 16) | (sample << 0);
            apu->mixer_divider = MIXER_DIVIDER;
        }
    } while (--i);
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
        // reading this register clears the frame interrupt flag
        // (but not the DMC interrupt flag).
        if (pm->data[0x0015] & (1 << 6))
        {
            pm->data[0x0015] &= ~(1 << 6);
            byte |= (1 << 6);
        }
        if (pm->data[0x0015] & (1 << 7)     ) byte |= (1 << 7);
        if (nes->apu.sch1.length_counter > 0) byte |= (1 << 0);
        if (nes->apu.sch2.length_counter > 0) byte |= (1 << 1);
        if (nes->apu.tch .length_counter > 0) byte |= (1 << 2);
        if (nes->apu.nch .length_counter > 0) byte |= (1 << 3);
        if (nes->apu.dmc .remain_bytes   > 0) byte |= (1 << 4);
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
        ENVELOPE_RESET(nes->apu.sch1.envlop_unit) = 1; // set square channel's envelop reset flag
        break;

    case 0x0005:
        // if there was a write to the sweep register the reset flag is set
        nes->apu.sch2.sweepu_reset = 1;
        break;

    case 0x0007:
        // unless disabled, write this register immediately reloads
        // the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 1)) nes->apu.sch2.length_counter = length_table[byte >> 3];
        ENVELOPE_RESET(nes->apu.sch2.envlop_unit) = 1; // set square channel's envelop reset flag
        break;

    case 0x000B:
        // unless disabled, write this register immediately reloads
        // the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 2)) nes->apu.tch.length_counter = length_table[byte >> 3];
        nes->apu.tch.linear_haltflg = 1; // when register $400B is written to, the halt flag is set.
        break;

    case 0x000F:
        // unless disabled, write this register immediately reloads
        // the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 3)) nes->apu.nch.length_counter = length_table[byte >> 3];
        ENVELOPE_RESET(nes->apu.nch.envlop_unit) = 1; // set noise channel's envelop reset flag
        break;

    case 0x0010:
        // if the new interrupt enabled status is clear, the interrupt flag is cleared.
        if (!(byte & (1 << 7))) pm->data[0x0015] &= ~(1 << 7);
        break;

    case 0x0011:
        // a write to $4011 sets the counter and DAC to a new value
        nes->apu.dmc.output_value = byte * 2 - 127;
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
        if (!(byte & (1 << 3))) nes->apu.nch .length_counter = 0;
        // if the DMC bit is clear, the DMC bytes remaining will be set to 0
        // and the DMC will silence when it empties.
        if (!(byte & (1 << 4))) nes->apu.dmc .remain_bytes   = 0;
        // if the DMC bit is set, the DMC sample will be restarted only if
        // its bytes remaining is 0
        else
        {
            if (nes->apu.dmc.remain_bytes == 0)
            {
                nes->apu.dmc.curdma_addr  = 0xC000 + (pm->data[0x0012] << 6);
                nes->apu.dmc.remain_bytes = (pm->data[0x0013] << 4) + 1;
            }
        }
        // writing to this register clears the DMC interrupt flag.
        byte &= ~(3 << 6); byte |= (pm->data[0x0015] & (1 << 6));
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

        // interrupt inhibit flag. if set, the frame interrupt flag is cleared
        if (byte & (1 << 6)) pm->data[0x0015] &= ~(1 << 6);

        // if the 5-step is selected the sequencer is immediately clocked once
        if (byte & (1 << 7))
        {
            apu_render_square_channel  (&(nes->apu.sch1), (BYTE*)pm->data + 0x0000, 6);
            apu_render_square_channel  (&(nes->apu.sch2), (BYTE*)pm->data + 0x0004, 6);
            apu_render_triangle_channel(&(nes->apu.tch ), (BYTE*)pm->data + 0x0008, 6);
            apu_render_noise_channel   (&(nes->apu.nch ), (BYTE*)pm->data + 0x000C, 6);
            apu_render_dmc_channel     (&(nes->apu.dmc ), (BYTE*)pm->data + 0x0010, 6);
        }

        // call joypad memrw callback
        NES_PAD_REG_WCB(pm, addr, byte);
        return; // return directly
    }

    // save reg write value
    pm->data[addr] = byte;
}

