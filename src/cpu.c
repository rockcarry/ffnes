// 包含头文件
#include "nes.h"

// 常量定义
#define CPU_NMI_VECTOR  0xfffa
#define CPU_RST_VECTOR  0xfffc
#define CPU_IRQ_VECTOR  0xfffe

#define CPU_C_FLAG  (1 << 0)
#define CPU_Z_FLAG  (1 << 1)
#define CPU_I_FLAG  (1 << 2)
#define CPU_D_FLAG  (1 << 3)
#define CPU_B_FLAG  (1 << 4)
#define CPU_R_FLAG  (1 << 5)
#define CPU_O_FLAG  (1 << 6)
#define CPU_N_FLAG  (1 << 7)

// 函数实现
void cpu_init(CPU *cpu)
{
    NES *nes  = container_of(cpu, NES, cpu);
    cpu->cbus = nes->cbus;

    bus_readw(cpu->cbus, CPU_RST_VECTOR, &(cpu->pc));
    cpu->sp = 0xff;
    cpu->ax = 0x00;
    cpu->xi = 0x00;
    cpu->yi = 0x00;
    cpu->ps = CPU_I_FLAG | CPU_R_FLAG;
}

void cpu_reset(CPU *cpu)
{
    bus_readw(cpu->cbus, CPU_RST_VECTOR, &(cpu->pc));
    cpu->ps |= CPU_I_FLAG;
}

void cpu_run(CPU *cpu, int cycle)
{
}

