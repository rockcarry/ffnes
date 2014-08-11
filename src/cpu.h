#ifndef _NES_CPU_H_
#define _NES_CPU_H_

// 包含头文件
#include "stdefine.h"
#include "bus.h"

// 类型定义
typedef struct {
    WORD pc;
    BYTE sp;
    BYTE ax;
    BYTE xi;
    BYTE yi;
    BYTE ps;
    int  nmi_last;
    int  nmi_cur;
    int  irq_flag;
    long cycles_emu;
    long cycles_real;
    long cycles_dma;

// private:
    BUS  cbus;       // cpu bus
    BYTE cram[2048]; // cpu ram, 2KB
} CPU;

// 函数声明
void cpu_init (CPU *cpu, BUS cbus);
void cpu_free (CPU *cpu);
void cpu_reset(CPU *cpu);
void cpu_nmi  (CPU *cpu, int nmi);
void cpu_irq  (CPU *cpu, int irq);
void cpu_run  (CPU *cpu, int cycle);

#endif

