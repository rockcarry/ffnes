// 包含头文件
#include "joypad.h"

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
    memset(jp, 0, sizeof(JOYPAD)); // clear it first
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

    if (value) jp->pad_data[pad] |=  key;
    else       jp->pad_data[pad] &= ~key;

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
}

void NES_PAD_REG_RCB(MEM *pm, int addr)
{
}

void NES_PAD_REG_WCB(MEM *pm, int addr)
{
}

