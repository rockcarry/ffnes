#ifndef _NES_JOYPAD_H_
#define _NES_JOYPAD_H_

#include "mem.h"

#define NES_KEY_ALL     0x030000ff
#define NES_KEY_A       (1 << 0)
#define NES_KEY_B       (1 << 1)
#define NES_KEY_SELECT  (1 << 2)
#define NES_KEY_START   (1 << 3)
#define NES_KEY_UP      (1 << 4)
#define NES_KEY_DOWN    (1 << 5)
#define NES_KEY_LEFT    (1 << 6)
#define NES_KEY_RIGHT   (1 << 7)
#define NES_KEY_TURBO_A (1 << 24)
#define NES_KEY_TURBO_B (1 << 25)
#define NES_PAD_CONNECT (1 << 26)

// joypad_run called every ppu frame, means joypad_run run at 60Hz.
// JOYPAD_DIVIDER is used to make a 60Hz / 4 = 15Hz freqency clk.
// the joypad_run will update key state for turbo function.
#define JOYPAD_DIVIDER  4

typedef struct {
    int      strobe;
    int      counter_4016;
    int      counter_4017;
    uint32_t pad_data[4];
    int      divider;
} JOYPAD;

void joypad_init  (JOYPAD *jp);
void joypad_free  (JOYPAD *jp);
void joypad_reset (JOYPAD *jp);
void joypad_run   (JOYPAD *jp);
void joypad_setkey(JOYPAD *jp, int pad, int key, int value);

uint8_t NES_PAD_REG_RCB(MEM *pm, int addr);
void    NES_PAD_REG_WCB(MEM *pm, int addr, uint8_t byte);

#endif
