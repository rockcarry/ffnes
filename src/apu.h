#ifndef _NES_APU_H_
#define _NES_APU_H_

// 包含头文件
#include "stdefine.h"
#include "adev.h"
#include "mem.h"

// 类型定义
typedef struct
{
    int length_counter;
    int envlop_divider;
    int envlop_counter;
    int envlop_volume;
    int envlop_reset;
    int sweepu_divider;
    int sweepu_value;
    int sweepu_reset;
    int sweepu_silence;
    int wavseq_divider;
    int wavseq_counter;
    int output_value;
} SQUARE_CHANNEL;

typedef struct {
    BYTE regs[0x20];

    int  frame_interrupt;
    int  frame_divider;
    int  frame_counter;

    int  mixer_divider;
    int  mixer_counter;
    int  mixer_table_ss [31];
    int  mixer_table_tnd[203];

    SQUARE_CHANNEL sch1;
    SQUARE_CHANNEL sch2;

// private:
    void     *adevctxt;
    AUDIOBUF *audiobuf;
    long      pclk_frame;
} APU;

// 函数声明
void apu_init (APU *apu, DWORD extra);
void apu_free (APU *apu);
void apu_reset(APU *apu);
void apu_run_pclk(APU *apu);

BYTE NES_APU_REG_RCB(MEM *pm, int addr);
void NES_APU_REG_WCB(MEM *pm, int addr, BYTE byte);

#endif

