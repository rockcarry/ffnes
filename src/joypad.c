// 包含头文件
#include "nes.h"

// 内部函数实现
#define TURBOKEY_THREAD_DELAY  200
static DWORD WINAPI turbokey_thread_proc(LPVOID lpParam)
{
    JOYPAD *jp = (JOYPAD*)lpParam;
    int     i;

    while (1)
    {
        WaitForSingleObject(jp->hTurboEvent, -1);
        if (jp->bExitThread) break;
        if (jp->strobe     ) continue;

        for (i=0; i<4; i++)
        {
            if (jp->pad_data[i] & (NES_KEY_TURBO_A)) jp->pad_data[i] ^= NES_KEY_TURBO_A;
            if (jp->pad_data[i] & (NES_KEY_TURBO_B)) jp->pad_data[i] ^= NES_KEY_TURBO_B;
        }

        // delay some time
        Sleep(TURBOKEY_THREAD_DELAY);
    }
    return 0;
}

// 函数实现
void joypad_init(JOYPAD *jp)
{
    jp->hTurboEvent  = CreateEvent (NULL, TRUE, FALSE, NULL);
    jp->hTurboThread = CreateThread(NULL, 0, turbokey_thread_proc, (LPVOID)jp, 0, 0);
}

void joypad_free(JOYPAD *jp)
{
    jp->bExitThread = TRUE;
    SetEvent(jp->hTurboEvent);
    WaitForSingleObject(jp->hTurboThread, -1);
    CloseHandle(jp->hTurboEvent );
    CloseHandle(jp->hTurboThread);
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
    jp->bTurboFlag   = FALSE;
    ResetEvent(jp->hTurboEvent);
}

void joypad_setkey(JOYPAD *jp, int pad, int key, int value)
{
    BOOL flag = FALSE;
    int  i;

    //++ handle pad connect/disconnect ++//
    if (key == NES_PAD_CONNECT)
    {
        if (value) jp->pad_data[pad] =  NES_PAD_CONNECT;
        else       jp->pad_data[pad] = ~NES_PAD_CONNECT;
        return;
    }
    //-- handle pad connect --//

    //++ handle pad key up/down ++//
    if (value) jp->pad_data[pad] |=  key;
    else       jp->pad_data[pad] &= ~key;
    //-- handle pad key up/down --//

    //++ handle turbo keys ++//
    for (i=0; i<4; i++)
    {
        if (jp->pad_data[i] & (0x3 << 24))
        {
            flag = TRUE;
            break;
        }
    }

    if (jp->bTurboFlag != flag)
    {
        if (flag) SetEvent(jp->hTurboEvent);
        else    ResetEvent(jp->hTurboEvent);
        jp->bTurboFlag = flag;
    }
    //-- handle turbo keys --//
}

BYTE NES_PAD_REG_RCB(MEM *pm, int addr)
{
    NES    *nes = container_of(pm, NES, ppuregs);
    JOYPAD *pad = &(nes->pad);

    if (pad->strobe) pm->data[addr] = 0;
    else
    {
        DWORD conn = 0;
        conn = (pad->pad_data[3] & NES_PAD_CONNECT) >> 26; conn <<= 1;
        conn = (pad->pad_data[2] & NES_PAD_CONNECT) >> 26; conn <<= 1;
        conn = (pad->pad_data[1] & NES_PAD_CONNECT) >> 26; conn <<= 1;
        conn = (pad->pad_data[0] & NES_PAD_CONNECT) >> 26; conn <<= 0;

        switch (addr)
        {
        case 0: // 4016
            if (pad->counter_4016 < 8) {
                pm->data[addr] = (BYTE)(pad->pad_data[0] >> (pad->counter_4016 - 0)) & 0x1;
            }
            else if (pad->counter_4016 < 16) {
                pm->data[addr] = (BYTE)(pad->pad_data[2] >> (pad->counter_4016 - 8)) & 0x1;
            }
            else if (pad->counter_4016 < 20) {
                pm->data[addr] = (BYTE)(conn >> (pad->counter_4016 - 16)) & 0x1;
            }
            else {
                pm->data[addr] = 0;
            }
            if (pad->counter_4016 < 32) pad->counter_4016++;
            break;
        case 1: // 4017
            if (pad->counter_4017 < 8) {
                pm->data[addr] = (BYTE)(pad->pad_data[1] >> (pad->counter_4017 - 0)) & 0x1;
            }
            else if (pad->counter_4017 < 16) {
                pm->data[addr] = (BYTE)(pad->pad_data[3] >> (pad->counter_4017 - 8)) & 0x1;
            }
            else if (pad->counter_4017 < 20) {
                pm->data[addr] = (BYTE)(conn >> (pad->counter_4017 - 16)) & 0x1;
            }
            else {
                pm->data[addr] = 0;
            }
            if (pad->counter_4017 < 32) pad->counter_4017++;
            break;
        }
    }
    return pm->data[addr];
}

void NES_PAD_REG_WCB(MEM *pm, int addr, BYTE byte)
{
    NES    *nes = container_of(pm, NES, ppuregs);
    JOYPAD *pad = &(nes->pad);

    switch (addr)
    {
    case 0: // 4016
        if (byte & 0x1) {
            pad->strobe       = 1;
            pad->counter_4016 = 0;
            pad->counter_4017 = 0;
        }
        else {
            pad->strobe       = 0;
        }
        break;
    }
}

