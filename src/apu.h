#ifndef _NES_APU_H_
#define _NES_APU_H_

// 包含头文件
#include "stdefine.h"
#include "adev.h"
#include "mem.h"

// 类型定义
typedef struct {
    BYTE regs[0x20];

    int  frame_interrupt;
    int  frame_divider;
    int  frame_counter;
    int  mixer_divider;
    int  mixer_counter;

    int  sch0_length_counter;
    int  sch0_envlop_divider;
    int  sch0_envlop_counter;
    int  sch0_envlop_volume;
    int  sch0_wavseq_divider;
    int  sch0_wavseq_counter;
    int  sch0_output_value;
    int _4003_write_flag;

// private:
    void     *adevctxt;
    AUDIOBUF *audiobuf;
    long      pclk_frame;
} APU;

// 函数声明
void apu_init (APU *apu, DWORD extra);
void apu_free (APU *apu);
void apu_reset(APU *apu);
void apu_run_pclk(APU *apu, int pclk);

BYTE NES_APU_REG_RCB(MEM *pm, int addr);
void NES_APU_REG_WCB(MEM *pm, int addr, BYTE byte);

#endif

