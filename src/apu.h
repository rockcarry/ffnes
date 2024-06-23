#ifndef _NES_APU_H_
#define _NES_APU_H_

#include "mem.h"

#define APU_MIXER_SAMPRATE 48000
typedef void (*APU_RENDER_CB)(void *cbctx, int type, uint8_t *buf, int len);

typedef struct {
    int divider, counter, reload, output;
} APU_ENVELOPE;

typedef struct {
    int divider, reload, carry, mute;
} APU_SWEEP;

typedef struct {
    int counter, output;
} APU_SEQUENCER;

typedef struct {
    uint8_t regs[0x20];

    int     cclk_divider; // divider for pclk -> cclk, 1 * cclk = 3 * pclk
    int     timer_divider_square1;  // timer divider of square1 channel
    int     timer_divider_square2;  // timer divider of square2 channel
    int     timer_divider_triangle; // timer divider of triangle channel
    int     timer_divider_noise;    // timer divider of noise channel
    int     timer_divider_dmc;      // timer divider of dmc channel

    int     frame_divider;          // frame divider
    int     frame_sequencer;        // frame sequencer

    APU_ENVELOPE  envelope_square1; // envelope unit of square1 channel
    APU_ENVELOPE  envelope_square2; // envelope unit of square2 channel
    APU_ENVELOPE  envelope_noise;   // envelope unit of noise channel

    APU_SEQUENCER wave_sequencer_square1;  // sequencer of square1 channel
    APU_SEQUENCER wave_sequencer_square2;  // sequencer of square2 channel
    APU_SEQUENCER wave_sequencer_triangle; // sequencer of triangle channel

    APU_SWEEP     sweep_square1;    // sweep unit of square1 channel
    APU_SWEEP     sweep_square2;    // sweep unit of square2 channel

    int      length_counter_square1; // length counter of square1 channel
    int      length_counter_square2; // length counter of square2 channel
    int      length_counter_triangle;// length counter of triangle channel
    int      length_counter_noise;   // length counter of noise channel

    int      linear_counter_counter; // linear counter
    int      linear_counter_halt;    // linear counter

    int      noise_shift_register;   // noise channel shift register
    int      dmc_sample_empty;       // dmc channel sample empty
    int      dmc_silence_flag;       // dmc channel silence flag
    int      dmc_sample_buffer;      // dmc channel sample buffer
    int      dmc_remain_bytes;       // dmc channel remain bytes
    int      dmc_dma_addr;           // dmc channel dma address
    int      dmc_remain_bits;        // dmc channel remain bits
    int      dmc_shift_buffer;       // dmc channel shift buffer
    int      dmc_output;

    int      mixer_divider;
    int      mixer_counter;
    int16_t  mixer_buffer[APU_MIXER_SAMPRATE / 60];

    APU_RENDER_CB callback;
    void         *cbctxt;
} APU;

void apu_init (APU *apu, APU_RENDER_CB callback, void *cbctxt);
void apu_free (APU *apu);
void apu_reset(APU *apu);
void apu_run  (APU *apu); // run 1 pclk

static __inline int apu_pin_irq(APU *apu) { return (apu->regs[0x0015] & 0xC0) ? 0 : 1; }

uint8_t NES_APU_REG_RCB(MEM *pm, int addr);
void    NES_APU_REG_WCB(MEM *pm, int addr, uint8_t byte);

#endif

