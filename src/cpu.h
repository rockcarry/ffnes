#ifndef _NES_CPU_H_
#define _NES_CPU_H_

// 包含头文件
#include "stdefine.h"

// 类型定义
typedef struct {
    BYTE pc;
    BYTE sp;
    BYTE ax;
    BYTE xi;
    BYTE yi;
    BYTE ps;
    long cycles;
} CPU;

void cpu_reset(CPU *cpu);

#endif

