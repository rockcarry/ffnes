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
    cpu->cycles_emu  = 0;
    cpu->cycles_real = 0;
}

void cpu_reset(CPU *cpu)
{
    bus_readw(cpu->cbus, CPU_RST_VECTOR, &(cpu->pc));
    cpu->ps |= CPU_I_FLAG;
    cpu->cycles_emu  = 0;
    cpu->cycles_real = 0;
}

void cpu_run(CPU *cpu, int ncycle)
{
    BYTE opcode = 0;

    cpu->cycles_emu  += ncycle;
    ncycle = cpu->cycles_emu - cpu->cycles_real;
    cpu->cycles_real += ncycle;

    while (ncycle > 0)
    {
        // fetch opcode
        bus_readb(cpu->cbus, cpu->pc++, &opcode);

        if (  opcode == 0x14 || opcode == 0x34 || opcode == 0x54 || opcode == 0x74 || opcode == 0xd4 || opcode == 0xf4
           || opcode == 0x1a || opcode == 0x3a || opcode == 0x5a || opcode == 0x7a || opcode == 0xda || opcode == 0xfa
           || opcode == 0x1c || opcode == 0x3c || opcode == 0x5c || opcode == 0x7c || opcode == 0xdc || opcode == 0xfc
           || opcode == 0x80 || opcode == 0x82 || opcode == 0x89
           || opcode == 0x04 || opcode == 0x44 || opcode == 0x64
           || opcode == 0xc2 || opcode == 0xe2
           || opcode == 0x0c || opcode == 0xea)
        {
            // nop
            // todo...
        }

        if ((opcode & 0x1f) == 0x12 || (opcode & 0x9f) == 0x02)
        {
            // undefined, may jams the machine
            // todo...
        }

        // addressing
        switch ((opcode >> 2) & 0x7)
        {
        case 0:
            // (indir,x)
            break;
        case 1:
            // zeropage
            break;
        case 2:
            // immediate
            break;
        case 3:
            // absolute
            break;
        case 4:
            // (indir),y
            break;
        case 5:
            // zeropage,x
            break;
        case 6:
            // absolute,y
            break;
        case 7:
            // absolute,x
            break;
        }

        if ((opcode & 0x3) == 0x1)
        {
            // excute
            switch (opcode >> 5)
            {
            case 0x00: // ORA
                break;
            case 0x01: // AND
                break;
            case 0x02: // EOR
                break;
            case 0x03: // ADC
                break;
            case 0x04: // STA
                break;
            case 0x05: // LDA
                break;
            case 0x06: // CMP
                break;
            case 0x07: // SBC
                break;
            }
        }
    }
    cpu->cycles_real -= ncycle;
}

