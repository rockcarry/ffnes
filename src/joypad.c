// 包含头文件
#include "nes.h"

// 函数实现
void joypad_init(JOYPAD *jp)
{
    joypad_reset(jp);
}

void joypad_free(JOYPAD *jp)
{
    // do nothing..
}

void joypad_reset(JOYPAD *jp)
{
    jp->strobe       = FALSE;
    jp->counter_4016 = 0;
    jp->counter_4017 = 0;
    jp->pad_data[0]  = 0;
    jp->pad_data[1]  = 0;
    jp->pad_data[2]  = 0;
    jp->pad_data[3]  = 0;
    jp->divider      = JOYPAD_DIVIDER;
}

void joypad_setkey(JOYPAD *jp, int pad, int key, int value)
{
    //++ handle pad connect/disconnect ++//
    if (key == NES_PAD_CONNECT) {
        if (value) jp->pad_data[pad] =  NES_PAD_CONNECT;
        else       jp->pad_data[pad] = ~NES_PAD_CONNECT;
        return;
    }
    //-- handle pad connect --//

    //++ handle pad key up/down ++//
    if (value) jp->pad_data[pad] |=  key;
    else       jp->pad_data[pad] &= ~key;
    //-- handle pad key up/down --//
}

BYTE NES_PAD_REG_RCB(MEM *pm, int addr)
{
    NES    *nes = container_of(pm, NES, apuregs);
    JOYPAD *pad = &(nes->pad);

    if (pad->strobe) pm->data[addr] = 0;
    else {
        DWORD conn = 0;
        conn = (pad->pad_data[3] & NES_PAD_CONNECT) >> 26; conn <<= 1;
        conn = (pad->pad_data[2] & NES_PAD_CONNECT) >> 26; conn <<= 1;
        conn = (pad->pad_data[1] & NES_PAD_CONNECT) >> 26; conn <<= 1;
        conn = (pad->pad_data[0] & NES_PAD_CONNECT) >> 26; conn <<= 0;

        switch (addr)
        {
        case 0x0016: // 4016
            if (pad->counter_4016 < 8) {
                pm->data[addr] = (BYTE)(pad->pad_data[0] >> (pad->counter_4016 - 0)) & 0x1;
            } else if (pad->counter_4016 < 16) {
                pm->data[addr] = (BYTE)(pad->pad_data[2] >> (pad->counter_4016 - 8)) & 0x1;
            } else if (pad->counter_4016 < 20) {
                pm->data[addr] = (BYTE)(conn >> (pad->counter_4016 - 16)) & 0x1;
            } else {
                pm->data[addr] = 0;
            }
            if (pad->counter_4016 < 32) pad->counter_4016++;
            break;

        case 0x0017: // 4017
            if (pad->counter_4017 < 8) {
                pm->data[addr] = (BYTE)(pad->pad_data[1] >> (pad->counter_4017 - 0)) & 0x1;
            } else if (pad->counter_4017 < 16) {
                pm->data[addr] = (BYTE)(pad->pad_data[3] >> (pad->counter_4017 - 8)) & 0x1;
            } else if (pad->counter_4017 < 20) {
                pm->data[addr] = (BYTE)(conn >> (pad->counter_4017 - 16)) & 0x1;
            } else {
                pm->data[addr] = 0;
            }
            if (pad->counter_4017 < 32) pad->counter_4017++;
            break;
        }
    }
    return pm->data[addr];
}

void joypad_run(JOYPAD *jp)
{
    if (--jp->divider == 0) {
        DWORD *paddata_cur = jp->pad_data;
        DWORD *paddata_end = paddata_cur + 4;
        do {
            if (*paddata_cur & NES_KEY_TURBO_A) *paddata_cur ^= NES_KEY_A;
            if (*paddata_cur & NES_KEY_TURBO_B) *paddata_cur ^= NES_KEY_B;
        } while (++paddata_cur < paddata_end);
        jp->divider = JOYPAD_DIVIDER;
    }
}

void NES_PAD_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    NES    *nes = container_of(pm, NES, apuregs);
    JOYPAD *pad = &(nes->pad);

    switch (addr)
    {
    case 0x0016: // 4016
        if (byte & 0x1) {
            pad->strobe       = 1;
            pad->counter_4016 = 0;
            pad->counter_4017 = 0;
        } else {
            pad->strobe       = 0;
        }
        break;
    }

    // save reg write value
    pm->data[addr] = byte;
}

