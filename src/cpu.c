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
#define NMI_VECTOR  0xfffa
#define RST_VECTOR  0xfffc
#define IRQ_VECTOR  0xfffe

#define C_FLAG  (1 << 0)
#define Z_FLAG  (1 << 1)
#define I_FLAG  (1 << 2)
#define D_FLAG  (1 << 3)
#define B_FLAG  (1 << 4)
#define R_FLAG  (1 << 5)
#define O_FLAG  (1 << 6)
#define N_FLAG  (1 << 7)

//++ basic ++//
#define RAM         (cpu->cram)
#define PC          (cpu->pc)
#define SP          (cpu->sp)
#define PS          (cpu->ps)
#define AX          (cpu->ax)
#define XI          (cpu->xi)
#define YI          (cpu->yi)
#define PUSH(v)     do { RAM[(SP--) + 0x100] = (v); } while (0)
#define POP()       (RAM[++SP + 0x100])

#define ZN_TAB            (cpu->zntab)
#define SET_FLAG(v)       do { PS |=  (v); } while (0)
#define CLR_FLAG(v)       do { PS &= ~(v); } while (0)
#define SET_ZN_FLAG(v)    do { PS &= ~(Z_FLAG | N_FLAG); PS |= ZN_TAB[v]; } while (0)
#define TST_FLAG(c, v)    do { PS &= ~(v); if (c) PS |= (v); } while (0)
#define CHK_FLAG(v)       (PS & (v))

#define READB(addr)       (bus_readb(cpu->cbus, addr))
#define READW(addr)       (bus_readw(cpu->cbus, addr))

#define ZPRDB(addr)       (RAM[(BYTE)(addr)])
#define ZPRDW(addr)       (*(WORD*)(RAM + (addr)))
#define ZPWRB(addr, byte) do { RAM[(BYTE)(addr)] = byte; } while (0)
#define ZPWRW(addr, word) do { *(WORD*)(RAM + (addr)) = word; } while (0)
//-- basic --//

//++ addressing mode ++//
#define MR_IM() \
do {                    \
    DT = READB(PC++);   \
} while (0)

#define MR_ZP() \
do {                    \
    EA = READB(PC++);   \
    DT = ZPRDB(EA);     \
} while (0)

#define MR_ZX() \
do {                        \
    EA = READB(PC++);       \
    EA = (BYTE)(DT + XI);   \
    DT = ZPRDB(EA);         \
} while (0)

#define MR_ZY() \
do {                        \
    EA = READB(PC++);       \
    EA = (BYTE)(DT + YI);   \
    DT = ZPRDB(EA);         \
} while (0)

#define MR_AB() \
do {                        \
    EA = READW(PC);         \
    PC += 2;                \
    DT = READB(EA);         \
} while (0)

#define MR_AX() \
do {                        \
    EA = READW(PC);         \
    PC += 2;                \
    DT = READB(EA + XI);    \
} while (0)

#define MR_AY() \
do {                        \
    EA = READW(PC);         \
    PC += 2;                \
    DT = READB(EA + YI);    \
} while (0)

#define MR_IX() \
do {                        \
    DT = READB(PC++);       \
    EA = ZPRDW(DT + XI);    \
    DT = READB(EA);         \
} while (0)

#define MR_IY() \
do {                        \
    DT = READB(PC++);       \
    EA = ZPRDW(DT);         \
    DT = READB(EA + YI);    \
} while (0)
//-- addressing mode --//

//++ instruction ++//
#define ORA() \
do {                        \
    AX |= DT;               \
    SET_ZN_FLAG(AX);        \
} while (0)

#define AND() \
do {                        \
    AX &= DT;               \
    SET_ZN_FLAG(AX);        \
} while (0)

#define EOR() \
do {                        \
    AX ^= DT;               \
    SET_ZN_FLAG(AX);        \
} while (0)

#define ADC() \
do {                        \
    WT = AX + DT + (PS & C_FLAG); \
    TST_FLAG(WT > 0xFF, C_FLAG);  \
    TST_FLAG(~(AX^DT) & (AX^WT) & 0x80, O_FLAG ); \
    AX = (BYTE)WT;          \
    SET_ZN_FLAG(AX);        \
} while (0)

#define STA() \
do {                        \
} while (0)
//-- instruction --//

#define BRK() \
do {                        \
    PC++;                   \
    PUSH((PC >> 8) & 0xff); \
    PUSH((PC >> 0) & 0xff); \
    SET_FLAG(B_FLAG);       \
    PUSH(PS);               \
    SET_FLAG(I_FLAG);       \
    PC = READW(IRQ_VECTOR); \
} while (0)

#define JSR() do { \
              } while (0)

// 函数实现
void cpu_init(CPU *cpu)
{
    int i;

    NES *nes  = container_of(cpu, NES, cpu);
    cpu->cbus = nes->cbus;
    cpu->cram = nes->buf_cram;

    cpu->pc = bus_readw(cpu->cbus, RST_VECTOR);
    cpu->sp = 0xff;
    cpu->ax = 0x00;
    cpu->xi = 0x00;
    cpu->yi = 0x00;
    cpu->ps = I_FLAG | R_FLAG;
    cpu->cycles_emu  = 0;
    cpu->cycles_real = 0;
    
    // init zntab
    cpu->zntab[0] = Z_FLAG;
    for (i=1; i<256; i++) {
        cpu->zntab[i] = (i & 0x80) ? N_FLAG : 0;
    }
}

void cpu_reset(CPU *cpu)
{
    cpu->pc  = bus_readw(cpu->cbus, RST_VECTOR);
    cpu->ps |= I_FLAG;
    cpu->cycles_emu  = 0;
    cpu->cycles_real = 0;
}

void cpu_run(CPU *cpu, int ncycle)
{
    BYTE opcode = 0;
    BYTE DT     = 0;
    WORD EA     = 0;
    WORD WT     = 0;

    cpu->cycles_emu  += ncycle;
    ncycle = cpu->cycles_emu - cpu->cycles_real;
    cpu->cycles_real += ncycle;

    while (ncycle > 0)
    {
        // fetch opcode
        opcode = bus_readb(cpu->cbus, cpu->pc++);

        // addressing
        switch ((opcode >> 2) & 0x7)
        {
        case 0: MR_IX(); break; // (indir,x)
        case 1: MR_ZP(); break; // zeropage
        case 2: MR_IM(); break; // immediate
        case 3: MR_AB(); break; // absolute
        case 4: MR_IY(); break; // (indir),y
        case 5: MR_ZX(); break; // zeropage,x
        case 6: MR_AY(); break; // absolute,y
        case 7: MR_AX(); break; // absolute,x
        }

        if ((opcode & 0x3) == 0x1 && opcode != 0x89)
        {
            // excute
            switch (opcode >> 5)
            {
            case 0x00: ORA(); break; // ORA
            case 0x01: AND(); break; // AND
            case 0x02: EOR(); break; // EOR
            case 0x03: ADC(); break; // ADC
            case 0x04: STA(); break; // STA
/*
            case 0x05: LDA(); break; // LDA
            case 0x06: CMP(); break; // CMP
            case 0x07: SBC(); break; // SBC
*/
            }
        }
        else
        {
            switch (opcode)
            {
            case 0x00: BRK(); break;
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

