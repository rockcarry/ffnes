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
#define FREQ_MCLK   26601712
#define CPU_FREQ    (FREQ_MCLK / 15)

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

#define SET_FLAG(v)        do { PS |=  (v); } while (0)
#define CLR_FLAG(v)        do { PS &= ~(v); } while (0)
#define SET_ZN_FLAG(v)     do { \
                               PS &= ~(Z_FLAG | N_FLAG); \
                               if (!v) PS |= Z_FLAG;     \
                               PS |= v & (1 << 7);       \
                           } while (0)
#define TST_FLAG(c, v)     do { PS &= ~(v); if (c) PS |= (v); } while (0)
#define CHK_FLAG(v)        (PS & (v))

#define READB(addr)        (bus_readb(cpu->cbus, addr))
#define READW(addr)        (bus_readw(cpu->cbus, addr))

#define WRITEB(addr, byte) (bus_writeb(cpu->cbus, addr, byte))
#define WRITEW(addr, word) (bus_writew(cpu->cbus, addr, word))

#define ZPRDB(addr)        (RAM[(BYTE)(addr)])
#define ZPRDW(addr)        (*(WORD*)(RAM + (addr)))
#define ZPWRB(addr, byte)  do { RAM[(BYTE)(addr)] = byte; } while (0)
#define ZPWRW(addr, word)  do { *(WORD*)(RAM + (addr)) = word; } while (0)
//-- basic --//

//++ addressing mode ++//
#define MR_IM() do { \
    DT = READB(PC++);   \
} while (0)

#define MR_IA() do { \
    DT = AX;            \
} while (0)

#define MR_ZP() do { \
    EA = READB(PC++);   \
    DT = ZPRDB(EA);     \
} while (0)

#define MR_ZX() do { \
    EA = READB(PC++);       \
    EA = (BYTE)(DT + XI);   \
    DT = ZPRDB(EA);         \
} while (0)

#define MR_ZY() do { \
    EA = READB(PC++);       \
    EA = (BYTE)(DT + YI);   \
    DT = ZPRDB(EA);         \
} while (0)

#define MR_AB() do { \
    EA = READW(PC);         \
    PC += 2;                \
    DT = READB(EA);         \
} while (0)

#define MR_AX() do { \
    EA = READW(PC);         \
    PC += 2;                \
    DT = READB(EA + XI);    \
} while (0)

#define MR_AY() do { \
    EA = READW(PC);         \
    PC += 2;                \
    DT = READB(EA + YI);    \
} while (0)

#define MR_IX() do { \
    DT = READB(PC++);       \
    EA = ZPRDW(DT + XI);    \
    DT = READB(EA);         \
} while (0)

#define MR_IY() do { \
    DT = READB(PC++);       \
    EA = ZPRDW(DT);         \
    DT = READB(EA + YI);    \
} while (0)

#define EA_ZP() do { \
    EA = READB(PC++);       \
} while (0)

#define EA_ZX() do { \
    DT = READB(PC++);       \
    EA = (BYTE)(DT + XI);   \
} while (0)

#define EA_ZY() do { \
    DT = READB(PC++);       \
    EA = (BYTE)(DT + YI);   \
} while (0)

#define EA_AB() do { \
    EA  = READW(PC);        \
    PC += 2;                \
} while (0)

#define EA_AX() do { \
    EA  = READW(PC);        \
    PC += 2;                \
    EA += XI;               \
} while (0)

#define EA_AY() do { \
    EA  = READW(PC);        \
    PC += 2;                \
    EA += YI;               \
} while (0)

#define EA_IX() do { \
    DT = READB(PC++);       \
    EA = ZPRDW(DT + XI);    \
} while (0)

#define EA_IY() do { \
    DT  = READB(PC++);      \
    EA  = ZPRDW( DT );      \
    EA += YI;               \
} while (0)

#define MW_ZP()  do { ZPWRB(EA, DT);  } while (0)
#define MW_EA()  do { WRITEB(EA, DT); } while (0)
//-- addressing mode --//

//++ instruction ++//
#define NOP() do { \
} while (0)

#define ORA() do { \
    AX |= DT;               \
    SET_ZN_FLAG(AX);        \
} while (0)

#define AND() do { \
    AX &= DT;               \
    SET_ZN_FLAG(AX);        \
} while (0)

#define EOR() do { \
    AX ^= DT;               \
    SET_ZN_FLAG(AX);        \
} while (0)

#define ADC() do { \
    WT = AX + DT + (PS & C_FLAG); \
    TST_FLAG(WT > 0xFF, C_FLAG);  \
    TST_FLAG(~(AX^DT) & (AX^WT) & 0x80, O_FLAG); \
    AX = (BYTE)WT;          \
    SET_ZN_FLAG(AX);        \
} while (0)

#define STA() do { \
    DT = AX;                \
} while (0)

#define LDA() do { \
    AX = DT;                \
    SET_ZN_FLAG(AX);        \
} while (0)

#define CMP() do { \
    WT = (WORD)AX - (WORD)DT; \
    TST_FLAG((WT & 0x8000) == 0, C_FLAG); \
    SET_ZN_FLAG((BYTE)WT);  \
} while (0)

#define SBC() do { \
    WT = AX - DT - (~PS & C_FLAG);  \
    TST_FLAG((AX^DT) & (AX^WT) & 0x80, O_FLAG); \
    TST_FLAG(WT < 0x100, C_FLAG);   \
    AX = (BYTE)WT;          \
    SET_ZN_FLAG(AX);        \
} while (0)

#define SLO() do { \
    TST_FLAG(DT & 0x80, C_FLAG);    \
    DT <<= 1;               \
    AX  |= DT;              \
    SET_ZN_FLAG(AX);        \
} while (0)

#define RLA() do { \
    if (PS & C_FLAG ) {     \
        TST_FLAG(DT & 0x80, C_FLAG);\
        DT = (DT << 1) | 1; \
    } else {                \
        TST_FLAG( DT&0x80, C_FLAG );\
        DT <<= 1;           \
    }                       \
    AX &= DT;               \
    SET_ZN_FLAG(AX);        \
} while (0)

#define SRE() do { \
    TST_FLAG(DT & 0x01, C_FLAG);    \
    DT >>= 1;               \
    AX  ^= DT;              \
    SET_ZN_FLAG(AX);        \
} while (0)

#define RRA() do { \
    if (PS & C_FLAG) {      \
        TST_FLAG(DT & 0x01, C_FLAG);\
        DT = (DT >> 1) | 0x80;      \
    } else {                \
        TST_FLAG(DT & 0x01, C_FLAG);\
        DT >>= 1;           \
    }                       \
    ADC();                  \
} while (0)

#define ASL() do { \
    TST_FLAG(DT & 0x80, C_FLAG );   \
    DT <<= 1;               \
    SET_ZN_FLAG(DT);        \
} while (0)

#define ROL() do { \
    if (PS & C_FLAG ) {     \
        TST_FLAG(DT & 0x80, C_FLAG);\
        DT = (DT << 1) | 0x01;      \
    } else {                \
        TST_FLAG(DT & 0x80, C_FLAG);\
        DT <<= 1;           \
    }                       \
    SET_ZN_FLAG(DT);        \
} while (0)

#define LSR() do { \
    TST_FLAG(DT & 0x01, C_FLAG);    \
    DT >>= 1;               \
    SET_ZN_FLAG(DT);        \
} while (0)

#define ROR() do { \
    if (PS & C_FLAG) {      \
        TST_FLAG(DT & 0x01, C_FLAG);\
        DT = (DT >> 1) | 0x80;      \
    } else {                \
        TST_FLAG(DT & 0x01, C_FLAG);\
        DT >>= 1;           \
    }                       \
    SET_ZN_FLAG(DT);        \
} while (0)

#define DCP() do { \
    DT--;                   \
    CMP();                  \
} while (0)

#define ISB() do { \
    DT++;                   \
    SBC();                  \
} while (0)

//-- instruction --//

#define BRK() do { \
    PC++;                   \
    PUSH((PC >> 8) & 0xff); \
    PUSH((PC >> 0) & 0xff); \
    SET_FLAG(B_FLAG);       \
    PUSH(PS);               \
    SET_FLAG(I_FLAG);       \
    PC = READW(IRQ_VECTOR); \
} while (0)

#define JSR() do { \
    EA = READW(PC);         \
    PC++;                   \
    PUSH((PC >> 8) & 0xff); \
    PUSH((PC >> 0) & 0xff); \
    PC = EA;                \
} while (0)

#define RTI() do { \
    PS  = POP() | R_FLAG;   \
    PC  = POP() << 0;       \
    PC |= POP() << 8;       \
} while (0)

#define RTS() do { \
    PC  = POP();            \
    PC |= POP() << 8;       \
    PC++;                   \
} while (0)

#define PHP() do { \
    PUSH(PS | B_FLAG);      \
} while (0)

#define PLP() do { \
    PS = POP() | R_FLAG;    \
} while (0)

#define PHA() do { \
    PUSH(AX);               \
} while (0)

#define PLA() do { \
    AX = POP(); SET_ZN_FLAG(AX);    \
} while (0)

static BYTE CPU_CYCLE_TAB[256] =
{
    7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 2, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
    2, 2, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,
    2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
};

// 函数实现
void cpu_init(CPU *cpu)
{
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
    BYTE opmat  = 0;
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
        opmat  = (opcode & 0x1c) >> 2;

        // calculate new ncycle
        ncycle -= CPU_CYCLE_TAB[opcode];

        //++ STA & 0x89 NOP ++//
        if ((opcode & 0xe3) == 0x81) // STA
        {
            switch (opmat)
            {
            case 0: EA_IX(); STA(); MW_EA(); break;
            case 1: EA_ZP(); STA(); MW_ZP(); break;
            case 3: EA_AB(); STA(); MW_EA(); break;
            case 4: EA_IY(); STA(); MW_EA(); break;
            case 5: EA_ZX(); STA(); MW_ZP(); break;
            case 6: EA_AY(); STA(); MW_ZP(); break;
            case 7: EA_AX(); STA(); MW_ZP(); break;
            case 2: NOP(); break; // 0x89 NOP
            }
            continue;
        }
        //-- STA & 0x89 NOP --//


        //++ ORA, AND, EOR, ADC, LDA, CMP, SBC ++//
        if ((opcode & 0x3) == 0x01)
        {
            // addressing
            switch (opmat)
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

            // excute
            switch (opcode >> 5)
            {
            case 0: ORA(); break; // ORA
            case 1: AND(); break; // AND
            case 2: EOR(); break; // EOR
            case 3: ADC(); break; // ADC
            case 5: LDA(); break; // LDA
            case 6: CMP(); break; // CMP
            case 7: SBC(); break; // SBC
            }
            continue;
        }
        //-- ORA, AND, EOR, ADC, LDA, CMP, SBC --//


        //++ SLO, RLA, SRE, RRA ++//
        if ((opcode & 0x83) == 0x03)
        {
            // addressing
            switch (opmat)
            {
            case 0: MR_IX(); break; // (indir,x)
            case 1: MR_ZP(); break; // zeropage
            case 2: goto other_opcode_handler;
            case 3: MR_AB(); break; // absolute
            case 4: MR_IY(); break; // (indir),y
            case 5: MR_ZX(); break; // zeropage,x
            case 6: MR_AY(); break; // absolute,y
            case 7: MR_AX(); break; // absolute,x
            }

            // excute
            switch ((opcode >> 5) & 0x03)
            {
            case 0: SLO(); break; // SLO
            case 1: RLA(); break; // RLA
            case 2: SRE(); break; // SRE
            case 3: RRA(); break; // RRA
            }

            // addressing
            if (opmat == 1 || opmat == 5) MR_ZP();
            else MW_EA();

            continue;
        }
        //-- SLO, RLA, SRE, RRA --//


        //++ ASL, ROL, LSR, ROR ++//
        if ((opcode & 0x83) == 0x02)
        {
            // addressing
            switch (opmat)
            {
            case 1: MR_ZP(); break; // zeropage
            case 2: MR_IA(); break; // immediate
            case 3: MR_AB(); break; // absolute
            case 5: MR_ZX(); break; // zeropage,x
            case 7: MR_AX(); break; // absolute,x
            default: goto other_opcode_handler;
            }

            // excute
            switch ((opcode >> 5) & 0x03)
            {
            case 0: ASL(); break; // ASL
            case 1: ROL(); break; // ROL
            case 2: LSR(); break; // LSR
            case 3: ROR(); break; // ROR
            }

            // addressing
            switch (opmat)
            {
            case 1:
            case 5:
                MR_ZP();
                break;
            case 2:
                AX = DT;
                break;
            default:
                MW_EA();
                break;
            }
            continue;
        }
        //-- ASL, ROL, LSR, ROR --//

        //++ DCP, ISB ++//
        if ((opcode & 0xc3) == 0xc3)
        {
            // addressing
            switch (opmat)
            {
            case 0: MR_IX(); break; // (indir,x)
            case 1: MR_ZP(); break; // zeropage
            case 2: goto other_opcode_handler;
            case 3: MR_AB(); break; // absolute
            case 4: MR_IY(); break; // (indir),y
            case 5: MR_ZX(); break; // zeropage,x
            case 6: MR_AY(); break; // absolute,y
            case 7: MR_AX(); break; // absolute,x
            }

            // excute
            switch ((opcode >> 5) & 0x01)
            {
            case 0: DCP(); break; // DCP
            case 1: ISB(); break; // ISB
            }

            // addressing
            if (opmat == 1 || opmat == 5) MR_ZP();
            else MW_EA();

            continue;
        }
        //-- DCP, ROL --//

other_opcode_handler:
        // others
        switch (opcode)
        {
        case 0x00: BRK(); break; // BRK
        case 0x20: JSR(); break; // JSR
        case 0x40: RTI(); break; // RTI
        case 0x60: RTS(); break; // RTS
        case 0x08: PHP(); break; // PHP
        case 0x28: PLP(); break; // PLP
        case 0x48: PHA(); break; // PHA
        case 0x68: PLA(); break; // PLA
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
        case 0x24: // BIT
        case 0x2c:
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

    cpu->cycles_real -= ncycle;
}

