#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nes.h"
#include "utils.h"

#define CCLK_PEROID    3
#define MIXER_PEROID  (NES_FREQ_PPU / APU_MIXER_SAMPRATE + 1)
#define FRAME_PEROID  (NES_FREQ_PPU / 240)
#define FRAME_MODE(reg) ((reg)[0x0017] & (1 << 7))

static uint8_t s_length_table[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

static uint16_t s_noise_timer_period_table[16] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068   };
static uint16_t s_dmc_timer_period_table  [16] = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54 };
static void defcb(void *cbctx, int type, uint8_t *buf, int len) {}

void apu_init(APU *apu, APU_RENDER_CB callback, void *cbctxt) { apu->callback = callback ? callback : defcb; apu->cbctxt = cbctxt; apu_reset(apu); }
void apu_free(APU *apu) {}

void apu_reset(APU *apu)
{
    memset(apu, 0, offsetof(APU, mixer_divider));
    apu->cclk_divider  = CCLK_PEROID;  // reset cclk divider
    apu->frame_divider = FRAME_PEROID; // reset frame divider
    apu->mixer_divider = MIXER_PEROID; // reset mixer divider
    apu->timer_divider_square1   = 1;
    apu->timer_divider_square2   = 1;
    apu->timer_divider_triangle  = 1;
    apu->timer_divider_noise     = s_noise_timer_period_table[0];
    apu->timer_divider_dmc       = s_dmc_timer_period_table  [0];
    apu->envelope_square1.reload = 1;
    apu->envelope_square2.reload = 1;
    apu->envelope_noise  .reload = 1;
    apu->sweep_square1   .reload = 1;
    apu->sweep_square1   .carry  = 1;
    apu->sweep_square2   .reload = 1;
    apu->noise_shift_register    = 1;
    apu->dmc_sample_empty        = 1;
    apu->dmc_dma_addr            = 0xC000;
}

#define ENVELOPE_LOOP(reg)       ((reg) & (1 << 5))
#define ENVELOPE_DISABLED(reg)   ((reg) & (1 << 4))
#define ENVELOPE_PEROID(reg)    (((reg) & 0xF) + 1)
static void envelop_clocked(uint8_t reg, APU_ENVELOPE *env)
{
    if (env->reload) {
        env->reload  = 0;
        env->counter = 15;
        env->divider = ENVELOPE_PEROID(reg);
    } else {
        if (--env->divider == 0) {
            env->divider = ENVELOPE_PEROID(reg);
            if (env->counter) env->counter--;
            else if (ENVELOPE_LOOP(reg)) env->counter = 15;
        }
    }
    env->output = ENVELOPE_DISABLED(reg) ? (reg & 0xF) : env->counter;
}

#define LINEAR_COUNTER_CTRLFLAG(reg)  ((reg) & (1 << 7))
#define LINEAR_COUNTER_RELOADVAL(reg) ((reg) & 0x7F)
static void linear_counter_clocked(APU *apu)
{
    if (apu->linear_counter_halt) apu->linear_counter_counter = LINEAR_COUNTER_RELOADVAL(apu->regs[0x0008]);
    else if (apu->linear_counter_counter) apu->linear_counter_counter--;
    if (!LINEAR_COUNTER_CTRLFLAG(apu->regs[0x0008])) apu->linear_counter_halt = 0;
}

#define SWEEP_ENABLED(regs)   ((regs)[1] & (1 << 7))
#define SWEEP_PEROID(regs)  ((((regs)[1] >> 4) & 0x7) + 1)
#define SWEEP_NEGATIVE(regs)  ((regs)[1] & (1 << 3))
#define SWEEP_SHIFT(regs)     ((regs)[1] & 0x7)
#define TIMER_PEROID(regs)    ((regs)[2] | ((regs)[3] & 0x7) << 8)
static void sweep_unit_clocked(uint8_t *regs, APU_SWEEP *sweep)
{
    if (sweep->reload) {
        sweep->reload  = 0;
        sweep->divider = SWEEP_PEROID(regs);
    } else if (--sweep->divider == 0) {
        sweep->divider = SWEEP_PEROID(regs);
        int peroid = TIMER_PEROID(regs);
        int shift  = peroid >> SWEEP_SHIFT(regs);
        int target = peroid + (SWEEP_NEGATIVE(regs) ? - shift - sweep->carry : shift);
        sweep->mute = peroid < 8 || target > 0x7FF;
        if (!sweep->mute && SWEEP_ENABLED(regs) && SWEEP_SHIFT(regs)) {
            regs[0x0002]  = target & 0xff;
            regs[0x0003] &= ~0x7;
            regs[0x0003] |= target >> 8;
        }
    }
}

#define SQUARE_DUTY(reg) ((reg) >> 6)
static void wave_sequencer_clocked(uint8_t reg, APU_SEQUENCER *seq)
{
    int duty = SQUARE_DUTY(reg);
    switch (duty) {
    case 0: seq->output = (seq->counter > 0 && seq->counter < 2) ? 1 : -1; break;
    case 1: seq->output = (seq->counter > 0 && seq->counter < 3) ? 1 : -1; break;
    case 2: seq->output = (seq->counter > 0 && seq->counter < 5) ? 1 : -1; break;
    case 3: seq->output =!(seq->counter > 0 && seq->counter < 3) ? 1 : -1; break;
    }
    if (++seq->counter == 8) seq->counter = 0;
}

static void dmc_unit_clocked(APU *apu)
{
    NES *nes = container_of(apu, NES, apu);
    if (apu->dmc_sample_empty && apu->dmc_remain_bytes > 0) {
        nes->cpu.cclk_dma += 4;
        apu->dmc_sample_buffer = bus_readb(nes->cbus, apu->dmc_dma_addr);
        apu->dmc_sample_empty  = 0;

        // the address is incremented; if it exceeds $FFFF, it is wrapped around to $8000.
        if (++apu->dmc_dma_addr > 0xffff) apu->dmc_dma_addr = 0x8000;

        // the bytes remaining counter is decremented;
        if (--apu->dmc_remain_bytes == 0) {
            // if the bytes counter becomes zero and the loop flag is set, the sample is restarted
            if (apu->regs[0x0010] & (1 << 6)) {
                apu->dmc_dma_addr     = 0xC000 + (apu->regs[0x0012] << 6);
                apu->dmc_remain_bytes = (apu->regs[0x0013] << 4) + 1;
            }
            // otherwise if the bytes counter becomes zero and the interrupt
            // enabled flag is set, the interrupt flag is set.
            else if (apu->regs[0x0010] & (1 << 7)) apu->regs[0x0015] |= (1 << 7);
        }
    }
    if (apu->dmc_remain_bits == 0) {
        apu->dmc_remain_bits = 8;
        if (apu->dmc_sample_empty) apu->dmc_silence_flag = 1;
        else {
            apu->dmc_sample_empty = 1;
            apu->dmc_silence_flag = 0;
            apu->dmc_shift_buffer = apu->dmc_sample_buffer;
        }
    }
    if (!apu->dmc_silence_flag) {
        if (apu->dmc_shift_buffer & 1) {
            if (apu->dmc_output < 126) apu->dmc_output += 2;
        } else {
            if (apu->dmc_output > 1  ) apu->dmc_output -= 2;
        }
    }
    apu->dmc_shift_buffer >>= 1;
    apu->dmc_remain_bits --;
}

void apu_run(APU *apu)
{
    int fle = 0, xor;

    if (--apu->frame_divider == 0) {
        apu->frame_divider = FRAME_PEROID;

        if (FRAME_MODE(apu->regs)) {
            switch (apu->frame_sequencer) {
            case 0: fle |= (1 << 0); break; //  e
            case 1: fle |= (3 << 0); break; // le
            case 2: fle |= (1 << 0); break; //  e
            case 4: fle |= (3 << 0); break; // le
            }
            if (++apu->frame_sequencer >= 5) apu->frame_sequencer = 0;
        } else {
            switch (apu->frame_sequencer) {
            case 0: fle |= (1 << 0); break; //  e
            case 1: fle |= (3 << 0); break; // le
            case 2: fle |= (1 << 0); break; //  e
            case 3: fle |= (7 << 0); break; //fle
            }
            if (++apu->frame_sequencer >= 4) apu->frame_sequencer = 0;
        }

        if (fle & (1 << 0)) {
            envelop_clocked(apu->regs[0x0000], &apu->envelope_square1);
            envelop_clocked(apu->regs[0x0004], &apu->envelope_square2);
            envelop_clocked(apu->regs[0x000C], &apu->envelope_noise  );
            linear_counter_clocked(apu);
        }

        if (fle & (1 << 1)) {
            sweep_unit_clocked(apu->regs + 0x0000, &apu->sweep_square1);
            sweep_unit_clocked(apu->regs + 0x0004, &apu->sweep_square2);

            if (!(apu->regs[0x0000] & (1 << 5)) && apu->length_counter_square1 ) apu->length_counter_square1 --;
            if (!(apu->regs[0x0004] & (1 << 5)) && apu->length_counter_square2 ) apu->length_counter_square2 --;
            if (!(apu->regs[0x0008] & (1 << 7)) && apu->length_counter_triangle) apu->length_counter_triangle--;
            if (!(apu->regs[0x000C] & (1 << 5)) && apu->length_counter_noise   ) apu->length_counter_noise   --;
        }

        if ((fle & (1 << 2)) && !(apu->regs[0x0017] & (1 << 6))) apu->regs[0x0015] |= (1 << 6); // frame interrupt
    }

    if (--apu->cclk_divider == 0) {
        apu->cclk_divider = CCLK_PEROID;

        if (--apu->timer_divider_square1 == 0) {
            apu->timer_divider_square1  = (TIMER_PEROID(apu->regs + 0x0000) + 1) * 2;
            wave_sequencer_clocked(apu->regs[0x0000], &apu->wave_sequencer_square1);
        }

        if (--apu->timer_divider_square2 == 0) {
            apu->timer_divider_square2  = (TIMER_PEROID(apu->regs + 0x0004) + 1) * 2;
            wave_sequencer_clocked(apu->regs[0x0004], &apu->wave_sequencer_square2);
        }

        if (--apu->timer_divider_triangle == 0) {
            apu->timer_divider_triangle = (TIMER_PEROID(apu->regs + 0x0008) + 1) * 1;
            if (apu->wave_sequencer_triangle.counter < 16) apu->wave_sequencer_triangle.output = 8 - apu->wave_sequencer_triangle.counter;
            else apu->wave_sequencer_triangle.output = apu->wave_sequencer_triangle.counter - 23;
            if (++apu->wave_sequencer_triangle.counter == 32) apu->wave_sequencer_triangle.counter = 0;
        }

        if (--apu->timer_divider_noise == 0) {
            apu->timer_divider_noise = s_noise_timer_period_table[apu->regs[0x000E] & 0xF];
            if (apu->regs[0x000E] & (1 << 7)) {
                xor = (apu->noise_shift_register ^ (apu->noise_shift_register >> 6)) & 1;
            } else {
                xor = (apu->noise_shift_register ^ (apu->noise_shift_register >> 1)) & 1;
            }
            apu->noise_shift_register >>= 1;
            apu->noise_shift_register  |= (xor << 14);
        }

        if (--apu->timer_divider_dmc == 0) {
            apu->timer_divider_dmc = s_dmc_timer_period_table[apu->regs[0x0010] & 0xF];
            dmc_unit_clocked(apu);
        }
    }

    if (--apu->mixer_divider == 0) {
        apu->mixer_divider = MIXER_PEROID;
        int square1  = (!apu->sweep_square1.mute && apu->length_counter_square1) ? apu->wave_sequencer_square1.output * apu->envelope_square1.output : 0;
        int square2  = (!apu->sweep_square2.mute && apu->length_counter_square2) ? apu->wave_sequencer_square2.output * apu->envelope_square2.output : 0;
        int triangle = ( apu->linear_counter_counter && apu->length_counter_triangle) ? apu->wave_sequencer_triangle.output : 0;
        int noise    = ((apu->noise_shift_register & 1) && apu->length_counter_noise) ? apu->envelope_noise.output : 0;
        int dmc      = ( apu->dmc_output );
        int pcm      = 2 * (246 / 2 * (square1 + square2) + 279 * triangle + 162 * (noise - 7) + 110 * (dmc - 63));
        apu->mixer_buffer[apu->mixer_counter++] = pcm;
        if (apu->mixer_counter == sizeof(apu->mixer_buffer) / sizeof(int16_t)) {
            apu->mixer_counter = 0;
            apu->callback(apu->cbctxt, 0, (uint8_t*)apu->mixer_buffer, sizeof(apu->mixer_buffer));
        }
    }
}

uint8_t NES_APU_REG_RCB(MEM *pm, int addr)
{
    NES    *nes  = container_of(pm, NES, apuregs);
    uint8_t byte = 0;
    switch (addr) {
    case 0x0015: // apu status register
        // reading this register clears the frame interrupt flag (but not clear the DMC interrupt flag).
        if (pm->data[0x0015] & (1 << 6)) {
            pm->data[0x0015] &= ~(1 << 6);
            byte |= (1 << 6);
        }
        if (nes->apu.length_counter_square1  > 0) byte |= (1 << 0);
        if (nes->apu.length_counter_square2  > 0) byte |= (1 << 1);
        if (nes->apu.length_counter_triangle > 0) byte |= (1 << 2);
        if (nes->apu.length_counter_noise    > 0) byte |= (1 << 3);
        if (nes->apu.dmc_remain_bytes        > 0) byte |= (1 << 4);
        if (pm->data[0x0015] & (1 << 7)) byte |= (1 << 7);
        return byte;

    case 0x0016: case 0x0017:
        return NES_PAD_REG_RCB(pm, addr);

    default:
        return pm->data[addr];
    }
}

void NES_APU_REG_WCB(MEM *pm, int addr, uint8_t byte)
{
    NES *nes = container_of(pm, NES, apuregs);
    switch (addr) {
    case 0x0001: nes->apu.sweep_square1.reload = 1; break;
    case 0x0005: nes->apu.sweep_square2.reload = 1; break;

    case 0x0003:
        // unless disabled, write this register immediately reloads the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 0)) nes->apu.length_counter_square1 = s_length_table[byte >> 3];
        nes->apu.envelope_square1.reload = 1;
        break;

    case 0x0007:
        // unless disabled, write this register immediately reloads the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 1)) nes->apu.length_counter_square2 = s_length_table[byte >> 3];
        nes->apu.envelope_square2.reload = 1;
        break;

    case 0x000B:
        // unless disabled, write this register immediately reloads the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 2)) nes->apu.length_counter_triangle= s_length_table[byte >> 3];
        nes->apu.linear_counter_halt = 1; // when register $400B is written to, the linear counter halt flag is set.
        break;

    case 0x000F:
        // unless disabled, write this register immediately reloads the counter with the value from a lookup table
        if (pm->data[0x0015] & (1 << 3)) nes->apu.length_counter_noise   = s_length_table[byte >> 3];
        nes->apu.envelope_noise.reload = 1;
        break;

    case 0x0010:
        // if the new interrupt enabled status is clear, the interrupt flag is cleared.
        if (!(byte & (1 << 7))) pm->data[0x0015] &= ~(1 << 7);
        break;

    case 0x0011:
        // a write to $4011 sets the counter and DAC to a new value
        nes->apu.dmc_output = byte & 0x7F;
        break;

    case 0x0014:
        //++ for sprite dma
        for (int i = 0; i < 256; i++) {
            nes->ppu.sprram[nes->ppu.regs[0x0003]++] = bus_readb(nes->cbus, byte * 256 + i);
        }
        nes->cpu.cclk_dma += 512;
        //-- for sprite dma
        break;

    case 0x0015:
        if (!(byte & (1 << 0))) { nes->apu.length_counter_square1  = 0; }
        if (!(byte & (1 << 1))) { nes->apu.length_counter_square2  = 0; }
        if (!(byte & (1 << 2))) { nes->apu.length_counter_triangle = 0; }
        if (!(byte & (1 << 3))) { nes->apu.length_counter_noise    = 0; }
        if (!(byte & (1 << 4))) { nes->apu.dmc_remain_bytes        = 0; }
        else if (nes->apu.dmc_remain_bytes == 0) {
            /* if dma reader has no more sample bytes to fetch, the dmc sample is restarted */
            nes->apu.dmc_dma_addr     = 0xC000 + (nes->apu.regs[0x0012] << 6);
            nes->apu.dmc_remain_bytes = (nes->apu.regs[0x0013] << 4) + 1;
        }
        // writing to this register clears the DMC interrupt flag.
        byte &= ~(3 << 6); byte |= (pm->data[0x0015] & (1 << 6));
        break;

    case 0x0016:
        // call joypad memrw callback
        NES_PAD_REG_WCB(pm, addr, byte);
        break;

    case 0x0017:
        // reset frame divider & sequencer
        nes->apu.frame_divider   = FRAME_PEROID;
        nes->apu.frame_sequencer = 0;

        // if the 5-step is selected the sequencer is immediately clocked once
        if (byte & (1 << 7)) {
            envelop_clocked(nes->apu.regs[0x0000], &nes->apu.envelope_square1);
            envelop_clocked(nes->apu.regs[0x0004], &nes->apu.envelope_square2);
            envelop_clocked(nes->apu.regs[0x000C], &nes->apu.envelope_noise  );
            linear_counter_clocked(&nes->apu);
        }

        // interrupt inhibit flag. if set, the frame interrupt flag is cleared
        if (byte & (1 << 6)) pm->data[0x0015] &= ~(1 << 6);
        break;
    }
    pm->data[addr] = byte;
}

