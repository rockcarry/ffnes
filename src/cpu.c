// 包含头文件
#include "nes.h"

/*
6510 Instructions by Addressing Modes

off- ++++++++++ Positive ++++++++++  ---------- Negative ----------
set  00      20      40      60      80      a0      c0      e0      mode

+00  BRK     JSR     RTI     RTS     NOP*    LDY     CPY     CPX     Impl/immed
+01  ORA     AND     EOR     ADC     STA     LDA     CMP     SBC     (indir,x)
+02   t       t       t       t      NOP*t   LDX     NOP*t   NOP*t     ? /immed
+03  SLO*    RLA*    SRE*    RRA*    SAX*    LAX*    DCP*    ISB*    (indir,x)
+04  NOP*    BIT     NOP*    NOP*    STY     LDY     CPY     CPX     Zeropage
+05  ORA     AND     EOR     ADC     STA     LDA     CMP     SBC     Zeropage
+06  ASL     ROL     LSR     ROR     STX     LDX     DEC     INC     Zeropage
+07  SLO*    RLA*    SRE*    RRA*    SAX*    LAX*    DCP*    ISB*    Zeropage

+08  PHP     PLP     PHA     PLA     DEY     TAY     INY     INX     Implied
+09  ORA     AND     EOR     ADC     NOP*    LDA     CMP     SBC     Immediate
+0a  ASL     ROL     LSR     ROR     TXA     TAX     DEX     NOP     Accu/impl
+0b  ANC**   ANC**   ASR**   ARR**   ANE**   LXA**   SBX**   SBC*    Immediate
+0c  NOP*    BIT     JMP     JMP ()  STY     LDY     CPY     CPX     Absolute
+0d  ORA     AND     EOR     ADC     STA     LDA     CMP     SBC     Absolute
+0e  ASL     ROL     LSR     ROR     STX     LDX     DEC     INC     Absolute
+0f  SLO*    RLA*    SRE*    RRA*    SAX*    LAX*    DCP*    ISB*    Absolute

+10  BPL     BMI     BVC     BVS     BCC     BCS     BNE     BEQ     Relative
+11  ORA     AND     EOR     ADC     STA     LDA     CMP     SBC     (indir),y
+12   t       t       t       t       t       t       t       t         ?
+13  SLO*    RLA*    SRE*    RRA*    SHA**   LAX*    DCP*    ISB*    (indir),y
+14  NOP*    NOP*    NOP*    NOP*    STY     LDY     NOP*    NOP*    Zeropage,x
+15  ORA     AND     EOR     ADC     STA     LDA     CMP     SBC     Zeropage,x
+16  ASL     ROL     LSR     ROR     STX  y) LDX  y) DEC     INC     Zeropage,x
+17  SLO*    RLA*    SRE*    RRA*    SAX* y) LAX* y) DCP*    ISB*    Zeropage,x

+18  CLC     SEC     CLI     SEI     TYA     CLV     CLD     SED     Implied
+19  ORA     AND     EOR     ADC     STA     LDA     CMP     SBC     Absolute,y
+1a  NOP*    NOP*    NOP*    NOP*    TXS     TSX     NOP*    NOP*    Implied
+1b  SLO*    RLA*    SRE*    RRA*    SHS**   LAS**   DCP*    ISB*    Absolute,y
+1c  NOP*    NOP*    NOP*    NOP*    SHY**   LDY     NOP*    NOP*    Absolute,x
+1d  ORA     AND     EOR     ADC     STA     LDA     CMP     SBC     Absolute,x
+1e  ASL     ROL     LSR     ROR     SHX**y) LDX  y) DEC     INC     Absolute,x
+1f  SLO*    RLA*    SRE*    RRA*    SHA**y) LAX* y) DCP*    ISB*    Absolute,x

        ROR intruction is available on MC650x microprocessors after
        June, 1976.

        Legend:

        t       Jams the machine
        *t      Jams very rarely
        *       Undocumented command
        **      Unusual operation
        y)      indexed using Y instead of X
        ()      indirect instead of absolute

Note that the NOP instructions do have other addressing modes than the
implied addressing. The NOP instruction is just like any other load
instruction, except it does not store the result anywhere nor affects the
flags.
*/


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

        if ((opcode & 0x3) == 0x1 && opcode != 0x89)
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
        else
        {
            switch (opcode)
            {
            case 0x00: // BRK
                break;
            case 0x20: // JSR
                break;
            case 0x40: // RTI
                break;
            case 0x60: // RTS
                break;
            case 0xa0: // LDY
            case 0xa4:
            case 0xac:
            case 0xb4:
            case 0xbc:
                break;
            case 0xc0: // CPY
            case 0xc4:
            case 0xcc:
                break;
            case 0xe0: // CPX
            case 0xe4:
            case 0xec:
                break;
            case 0xa2: // LDX
            case 0xa6:
            case 0xae:
                break;
            case 0xb6: // LDX  y)
            case 0xbe: // LDX  y)
                break;
            case 0x03: // SLO
            case 0x07:
            case 0x0f:
            case 0x13:
            case 0x17:
            case 0x1f:
                break;
            case 0x23: // SLO
            case 0x27:
            case 0x2f:
            case 0x33:
            case 0x37:
            case 0x3b:
            case 0x3f:
                break;
            case 0x43: // SRE
            case 0x47:
            case 0x4f:
            case 0x53:
            case 0x57:
            case 0x5b:
            case 0x5f:
                break;
            case 0x63: // RRA
            case 0x67:
            case 0x6f:
            case 0x73:
            case 0x77:
            case 0x7b:
            case 0x7f:
                break;
            case 0x83: // SAX
            case 0x87:
            case 0x8f:
                break;
            case 0x97: // SAX y)
                break;
            case 0xa3: // LAX
            case 0xa7:
            case 0xaf:
            case 0xb3:
                break;
            case 0xb7: // LAX y)
            case 0xbf:
                break;
            case 0xc3: // DCP
            case 0xc7:
            case 0xcf:
            case 0xd3:
            case 0xd7:
            case 0xdb:
            case 0xdf:
                break;
            case 0xe3: // ISB
            case 0xe7:
            case 0xef:
            case 0xf3:
            case 0xf7:
            case 0xfb:
            case 0xff:
                break;
            case 0x24: // BIT
            case 0x2c:
                break;
            case 0x84: // STY
            case 0x8c:
            case 0x94:
                break;
            case 0x06: // ASL
            case 0x0a:
            case 0x0e:
            case 0x16:
            case 0x1e:
                break;
            case 0x26: // ROL
            case 0x2a:
            case 0x2e:
            case 0x36:
            case 0x3e:
                break;
            case 0x46: // LSR
            case 0x4a:
            case 0x4e:
            case 0x56:
            case 0x5e:
                break;
            case 0x66: // ROR
            case 0x6a:
            case 0x6e:
            case 0x76:
            case 0x7e:
                break;
            case 0x86: // STX
            case 0x8e:
                break;
            case 0x96: // STX y)
                break;
            case 0xc6: // DEC
            case 0xce:
            case 0xd6:
            case 0xde:
                break;
            case 0xe6: // INC
            case 0xee:
            case 0xf6:
            case 0xfe:
                break;
            case 0x08: // PHP
                break;
            case 0x28: // PLP
                break;
            case 0x48: // PHA
                break;
            case 0x68: // PLA
                break;
            case 0x88: // DEY
                break;
            case 0xa8: // TAY
                break;
            case 0xc8: // INY
                break;
            case 0xe8: // INX
                break;
            case 0x8a: // TXA
                break;
            case 0xaa: // TAX
                break;
            case 0xca: // DEX
                break;
            case 0xea: // NOP
                break;
            case 0x0b: // ANC
            case 0x2b:
                break;
            case 0x4b: // ASR
                break;
            case 0x6b: // ARR
                break;
            case 0x8b: // ANE
                break;
            case 0xab: // LXA
                break;
            case 0xcb: // SBX
                break;
            case 0xeb: // SBC
                break;
            case 0x4c: // JMP
                break;
            case 0x6c: // JMP ()
                break;
            case 0x10: // BPL
                break;
            case 0x30: // BMI
                break;
            case 0x50: // BVC
                break;
            case 0x70: // BVS
                break;
            case 0x90: // BCC
                break;
            case 0xb0: // BCS
                break;
            case 0xd0: // BNE
                break;
            case 0xf0: // BEQ
                break;
            case 0x93: // SHA
                break;
            case 0x18: // CLC
                break;
            case 0x38: // SEC
                break;
            case 0x58: // CLI
                break;
            case 0x78: // SEI
                break;
            case 0x98: // TYA
                break;
            case 0xb8: // CLV
                break;
            case 0xd8: // CLD
                break;
            case 0xf8: // SED
                break;
            case 0x9a: // TXS
                break;
            case 0xba: // TSX
                break;
            case 0x9b: // SHS
                break;
            case 0xbb: // LAS
                break;
            case 0x9c: // SHY
                break;
            case 0x9e: // SHX y)
                break;
            case 0x9f: // SHA y)
                break;
            }
        }
    }
    cpu->cycles_real -= ncycle;
}

