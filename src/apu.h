#ifndef _NES_APU_H_
#define _NES_APU_H_

// 包含头文件
#include "stdefine.h"
#include "mem.h"
#include "adev.h"

// 类型定义
typedef struct
{
    int length_counter;
    int envlop_unit[4];
    int sweepu_divider;
    int sweepu_value;
    int sweepu_reset;
    int sweepu_silence;
    int stimer_divider;
    int schseq_counter;
    int output_value;
} SQUARE_CHANNEL;

typedef struct
{
    int length_counter;
    int linear_haltflg;
    int linear_counter;
    int ttimer_divider;
    int tchseq_counter;
    int output_value;
} TRIANGLE_CHANNEL;

typedef struct
{
    int length_counter;
    int envlop_unit[4];
    int ntimer_divider;
    int nshift_register;
    int output_value;
} NOISE_CHANNEL;

typedef struct
{
    int remain_bytes;
    int remain_bits;
    int sample_buffer;
    int sample_empty;
    int buffer_bits;
    int curdma_addr;
    int dmc_silence;
    int dtimer_divider;
    int output_value;
} DMC_CHANNEL;

typedef struct {
    BYTE regs[0x20];

    int  frame_divider;
    int  frame_counter;

    int  mixer_divider;
    int  mixer_counter;

    SQUARE_CHANNEL   sch1;
    SQUARE_CHANNEL   sch2;
    TRIANGLE_CHANNEL tch;
    NOISE_CHANNEL    nch;
    DMC_CHANNEL      dmc;

// private:
    ADEV     *adev;
    void     *actxt;
    AUDIOBUF *audiobuf;
    int       pclk_frame;
} APU;

// 函数声明
void apu_init (APU *apu, DWORD extra, ADEV *pdev);
void apu_free (APU *apu);
void apu_reset(APU *apu);
void apu_run_pclk(APU *apu);

BYTE NES_APU_REG_RCB(MEM *pm, int addr);
void NES_APU_REG_WCB(MEM *pm, int addr, BYTE byte);

#endif

