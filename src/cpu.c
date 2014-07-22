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
#define FREQ_MCLK       21477272
#define FREQ_CPU       (FREQ_MCLK / 12)
#define FREQ_PPU       (FREQ_MCLK / 4 )
#define NTSC_HTOTAL     341
#define NTSC_VTOTAL     262
#define NTSC_FRAMERATE (FREQ_PPU / (NTSC_HTOTAL*NTSC_VTOTAL))

#define IRQ_FLAG (1 << 0)
#define NMI_FLAG (1 << 1)

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
#define MR_IM() do { DT = READB(PC++); } while (0)
#define MR_IA() do { DT = AX;          } while (0)
#define MR_ZP() do { EA = READB(PC++); DT = ZPRDB(EA); } while (0)
#define MR_ZX() do { EA = READB(PC++); EA = (BYTE)(DT + XI); DT = ZPRDB(EA); } while (0)
#define MR_ZY() do { EA = READB(PC++); EA = (BYTE)(DT + YI); DT = ZPRDB(EA); } while (0)
#define MR_AB() do { EA = READW(PC); PC += 2; DT = READB(EA);      } while (0)
#define MR_AX() do { ET = READW(PC); PC += 2; EA = ET + XI; DT = READB(EA); } while (0)
#define MR_AY() do { ET = READW(PC); PC += 2; EA = ET + YI; DT = READB(EA); } while (0)

#define MR_IX() do { DT = READB(PC++); EA = ZPRDW(DT + XI); DT = READB(EA); } while (0)
#define MR_IY() do { DT = READB(PC++); ET = ZPRDW(DT); EA = ET + YI; DT = READB(EA); } while (0)

#define EA_ZP() do { EA = READB(PC++); } while (0)
#define EA_ZX() do { DT = READB(PC++); EA = (BYTE)(DT + XI); } while (0)
#define EA_ZY() do { DT = READB(PC++); EA = (BYTE)(DT + YI); } while (0)
#define EA_AB() do { EA = READW(PC); PC += 2;                } while (0)
#define EA_AX() do { ET = READW(PC); PC += 2; EA = ET + XI;  } while (0)
#define EA_AY() do { ET = READW(PC); PC += 2; EA = ET + YI;  } while (0)
#define EA_IX() do { DT = READB(PC++); EA = ZPRDW(DT + XI);  } while (0)
#define EA_IY() do { DT = READB(PC++); ET = ZPRDW(DT); EA = ET + YI; } while (0)
#define MW_ZP() do { ZPWRB(EA, DT);  } while (0)
#define MW_EA() do { WRITEB(EA, DT); } while (0)
#define	CHECK_EA() do { if ((ET & 0xFF00) != (EA & 0xFF00)) ncycle--; } while (0)
//-- addressing mode --//

//++ instruction ++//
#define ORA() do { AX |= DT; SET_ZN_FLAG(AX); } while (0)
#define AND() do { AX &= DT; SET_ZN_FLAG(AX); } while (0)
#define EOR() do { AX ^= DT; SET_ZN_FLAG(AX); } while (0)

#define ADC() do { \
    WT = AX + DT + (PS & C_FLAG); \
    TST_FLAG(WT > 0xFF, C_FLAG);  \
    TST_FLAG(~(AX^DT) & (AX^WT) & 0x80, O_FLAG); \
    AX = (BYTE)WT;          \
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
    if (PS & C_FLAG) {     \
        TST_FLAG(DT & 0x80, C_FLAG);\
        DT = (DT << 1) | 1; \
    } else {                \
        TST_FLAG(DT & 0x80, C_FLAG);\
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
//-- instruction --//

#define LDA() do { AX = DT; SET_ZN_FLAG(AX); } while (0)
#define LDX() do { XI = DT; SET_ZN_FLAG(XI); } while (0)
#define LDY() do { YI = DT; SET_ZN_FLAG(YI); } while (0)
#define LAX() do { AX = DT; XI = DT; SET_ZN_FLAG(AX); } while (0)
#define LXA() do { AX = XI = ((AX|0xEE) & DT); SET_ZN_FLAG(AX); } while (0)
#define LAS() do { AX = XI = SP = (SP & DT); SET_ZN_FLAG(AX);   } while (0)

#define STA() do { DT = AX;      } while (0)
#define STX() do { DT = XI;      } while (0)
#define STY() do { DT = YI;      } while (0)
#define SAX() do { DT = AX & XI; } while (0)
#define SHA() do { DT = AX & XI & (BYTE)((EA >> 8) + 1); } while (0)
#define SHY() do { DT = YI & (BYTE)((EA>>8)+1);          } while (0)
#define SHX() do { DT = XI & (BYTE)((EA>>8)+1);          } while (0)
#define SHS() do { SP = AX & XI; DT = SP & (BYTE)((EA>>8)+1); } while (0)

#define CPX() do { \
    WT = (WORD)XI - (WORD)DT;   \
    TST_FLAG((WT&0x8000)==0, C_FLAG);   \
    SET_ZN_FLAG((BYTE)WT);      \
} while (0)

#define CPY() do { \
    WT = (WORD)YI - (WORD)DT;   \
    TST_FLAG((WT&0x8000)==0, C_FLAG);   \
    SET_ZN_FLAG((BYTE)WT);      \
} while (0)

#define INC() do { DT++; SET_ZN_FLAG(DT); } while (0)
#define DEC() do { DT--; SET_ZN_FLAG(DT); } while (0)
#define INX() do { XI++; SET_ZN_FLAG(XI); } while (0)
#define INY() do { YI++; SET_ZN_FLAG(YI); } while (0)
#define DEX() do { XI--; SET_ZN_FLAG(XI); } while (0)
#define DEY() do { YI--; SET_ZN_FLAG(YI); } while (0)

#define DCP() do { DT--; CMP(); } while (0)
#define ISB() do { DT++; SBC(); } while (0)

#define BIT() do { \
    TST_FLAG((DT & AX) == 0, Z_FLAG);   \
    TST_FLAG( DT & 0x80, N_FLAG);       \
    TST_FLAG( DT & 0x40, O_FLAG);       \
} while (0)

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

#define PHP() do { PUSH(PS | B_FLAG);           } while (0)
#define PLP() do { PS = POP() | R_FLAG;         } while (0)
#define PHA() do { PUSH(AX);                    } while (0)
#define PLA() do { AX = POP(); SET_ZN_FLAG(AX); } while (0)

#define JMP_4C() do { \
    PC = READW(PC);         \
} while (0)

#define JMP_6C() do { \
    WT = READW(PC);         \
    EA = READB(WT);         \
    WT = (WT&0xFF00)|((WT+1)&0x00FF);   \
    PC = EA + (READB(WT) << 8); \
} while (0)

#define REL_JUMP() do { \
    ET = PC;                \
    EA = PC + (char)DT;     \
    PC = EA;                \
    ncycle--;               \
    CHECK_EA();             \
} while (0)

#define BPL() do { if (!(PS & N_FLAG) ) REL_JUMP(); } while (0)
#define BMI() do { if ( (PS & N_FLAG) ) REL_JUMP(); } while (0)
#define BVC() do { if (!(PS & O_FLAG) ) REL_JUMP(); } while (0)
#define BVS() do { if ( (PS & O_FLAG) ) REL_JUMP(); } while (0)
#define BCC() do { if (!(PS & C_FLAG) ) REL_JUMP(); } while (0)
#define BCS() do { if ( (PS & C_FLAG) ) REL_JUMP(); } while (0)
#define BNE() do { if (!(PS & Z_FLAG) ) REL_JUMP(); } while (0)
#define BEQ() do { if ( (PS & Z_FLAG) ) REL_JUMP(); } while (0)

#define CLC() do { PS &= ~C_FLAG; } while (0)
#define SEC() do { PS |=  C_FLAG; } while (0)
#define CLI() do { PS &= ~I_FLAG; } while (0)
#define SEI() do { PS |=  I_FLAG; } while (0)
#define CLV() do { PS &= ~O_FLAG; } while (0)
#define CLD() do { PS &= ~D_FLAG; } while (0)
#define SED() do { PS |=  D_FLAG; } while (0)

#define TXA() do { AX = XI; SET_ZN_FLAG(AX); } while (0)
#define TAX() do { XI = AX; SET_ZN_FLAG(XI); } while (0)
#define TYA() do { AX = YI; SET_ZN_FLAG(AX); } while (0)
#define TAY() do { YI = AX; SET_ZN_FLAG(YI); } while (0)
#define TXS() do { SP = XI;                  } while (0)
#define TSX() do { XI = SP; SET_ZN_FLAG(XI); } while (0)

#define ANC() do { \
    AX &= DT;               \
    SET_ZN_FLAG(AX);        \
    TST_FLAG(PS & N_FLAG, C_FLAG);  \
} while (0)

#define ASR() do { \
    DT &= AX;               \
    TST_FLAG(DT & 0x01, C_FLAG );   \
    AX  = DT >> 1;          \
    SET_ZN_FLAG(AX);        \
} while (0)

#define ARR() do { \
    DT &= AX;               \
    AX  = (DT >> 1)|((PS&C_FLAG) << 7); \
    SET_ZN_FLAG(AX);        \
    TST_FLAG(AX & 0x40, C_FLAG);        \
    TST_FLAG((AX>>6)^(AX>>5), O_FLAG ); \
} while (0)

#define ANE() do { \
    AX = (AX|0xEE) & XI & DT;   \
    SET_ZN_FLAG(AX);        \
} while (0)

#define SBX() do { \
    WT = (AX&XI) - DT;      \
    TST_FLAG(WT < 0x100, C_FLAG);   \
    XI = WT & 0xFF;         \
    SET_ZN_FLAG(AX);        \
} while (0)

#define NOP() do {} while (0)
#define UDF() do {} while (0)

static BYTE CPU_CYCLE_TAB[256] =
{
    7, 6, 0, 0, 3, 3, 5, 0, 3, 2, 2, 0, 2, 4, 6, 0,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 2, 4, 7, 7,
    6, 2, 0, 8, 3, 3, 5, 5, 4, 2, 2, 0, 4, 4, 6, 6,
    2, 2, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 2, 4, 7, 7,
    6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 0, 3, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 2, 4, 7, 7,
    6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 0, 5, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 2, 4, 7, 7,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 0, 4, 4, 4, 4,
    2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,
    2, 5, 0, 0, 4, 4, 6, 0, 2, 4, 2, 0, 2, 4, 7, 0,
    2, 6, 2, 5, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 5,
    2, 5, 0, 5, 4, 4, 6, 5, 2, 4, 2, 5, 2, 4, 7, 5,
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
    cpu->intr_flag   = 0;
    cpu->cycles_emu  = 0;
    cpu->cycles_real = 0;
}

void cpu_reset(CPU *cpu)
{
    cpu->pc  = bus_readw(cpu->cbus, RST_VECTOR);
    cpu->ps |= I_FLAG;
    cpu->intr_flag   = 0;
    cpu->cycles_emu  = 0;
    cpu->cycles_real = 0;
}

void cpu_nmi(CPU *cpu)
{
    cpu->intr_flag |= NMI_FLAG;
}

void cpu_run(CPU *cpu, int ncycle)
{
    BYTE opcode = 0;
    BYTE opmat  = 0;
    BYTE opopt  = 0;
    BYTE DT     = 0;
    WORD EA     = 0;
    WORD ET     = 0;
    WORD WT     = 0;

    cpu->cycles_emu  += ncycle;
    ncycle = cpu->cycles_emu - cpu->cycles_real;
    cpu->cycles_real += ncycle;

    while (ncycle > 0)
    {
        // fetch opcode
        opcode = bus_readb(cpu->cbus, cpu->pc++);
        opmat  = (opcode & 0x1c) >> 2;
        opopt  = (opcode >> 5);

        // calculate new ncycle
        ncycle -= CPU_CYCLE_TAB[opcode];

        //++ ORA, AND, EOR, ADC, LDA, CMP, SBC ++//
        if ((opcode & 0x3) == 0x01 && opopt != 4)
        {
            // addressing
            switch (opmat)
            {
            case 0: MR_IX(); break; // (indir,x)
            case 1: MR_ZP(); break; // zeropage
            case 2: MR_IM(); break; // immediate
            case 3: MR_AB(); break; // absolute
            case 4: MR_IY(); CHECK_EA(); break; // (indir),y
            case 5: MR_ZX(); break; // zeropage,x
            case 6: MR_AY(); CHECK_EA(); break; // absolute,y
            case 7: MR_AX(); CHECK_EA(); break; // absolute,x
            }

            // excute
            switch (opopt)
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
            switch (opopt & 0x03)
            {
            case 0: SLO(); break; // SLO
            case 1: RLA(); break; // RLA
            case 2: SRE(); break; // SRE
            case 3: RRA(); break; // RRA
            }

            // addressing
            switch (opmat)
            {
            case 1:
            case 5: MR_ZP(); break;
            default:MW_EA(); break;
            }
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
            switch (opopt & 0x03)
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
            case 5: MR_ZP(); break;
            case 2: AX = DT; break;
            default:MW_EA(); break;
            }
            continue;
        }
        //-- ASL, ROL, LSR, ROR --//

other_opcode_handler:
        // others
        switch (opcode)
        {
        case 0x81: EA_IX(); STA(); MW_EA(); break; // STA
        case 0x85: EA_ZP(); STA(); MW_ZP(); break; // STA
        case 0x89: NOP(); break;                   // NOP
        case 0x8d: EA_AB(); STA(); MW_EA(); break; // STA
        case 0x91: EA_IY(); STA(); MW_EA(); break; // STA
        case 0x95: EA_ZX(); STA(); MW_ZP(); break; // STA
        case 0x99: EA_AY(); STA(); MW_ZP(); break; // STA
        case 0x9d: EA_AX(); STA(); MW_ZP(); break; // STA

        case 0xa3: MR_IX(); LAX(); break; // LAX
        case 0xa7: MR_ZP(); LAX(); break; // LAX
        case 0xaf: MR_AB(); LAX(); break; // LAX
        case 0xb3: MR_IY(); LAX(); CHECK_EA(); break; // LAX
        case 0xb7: MR_ZY(); LAX(); break; // LAX
        case 0xbf: MR_AY(); LAX(); CHECK_EA(); break; // LAX

        case 0xa2: MR_IM(); LDX(); break; // LDX
        case 0xa6: MR_ZP(); LDX(); break; // LDX
        case 0xae: MR_AB(); LDX(); break; // LDX
        case 0xb6: MR_ZY(); LDX(); break; // LDX
        case 0xbe: MR_AY(); LDX(); CHECK_EA(); break; // LDX

        case 0xa0: MR_IM(); LDY(); break; // LDY
        case 0xa4: MR_ZP(); LDY(); break; // LDY
        case 0xac: MR_AB(); LDY(); break; // LDY
        case 0xb4: MR_ZX(); LDY(); break; // LDY
        case 0xbc: MR_AX(); LDY(); CHECK_EA(); break; // LDY

        case 0xab: MR_IM(); LXA(); break; // LXA
        case 0xbb: MR_AY(); LAS(); CHECK_EA(); break; // LAS

        case 0x83: MR_IX(); SAX(); MW_EA(); break; // SAX
        case 0x87: MR_ZP(); SAX(); MW_ZP(); break; // SAX
        case 0x8f: MR_AB(); SAX(); MW_EA(); break; // SAX
        case 0x97: MR_ZY(); SAX(); MW_ZP(); break; // SAX
        case 0x86: EA_ZP(); STX(); MW_ZP(); break; // STX
        case 0x8e: EA_AB(); STX(); MW_EA(); break; // STX
        case 0x96: EA_ZY(); STX(); MW_ZP(); break; // STX
        case 0x84: EA_ZP(); STY(); MW_ZP(); break; // STY
        case 0x8c: EA_AB(); STY(); MW_EA(); break; // STY
        case 0x94: EA_ZX(); STY(); MW_ZP(); break; // STY

        case 0x93: MR_IY(); SHA(); MW_EA(); break; // SHA
        case 0x9f: MR_AY(); SHA(); MW_EA(); break; // SHA y)
        case 0x9b: MR_AY(); SHS(); MW_EA(); break; // SHS
        case 0x9c: MR_AX(); SHY(); MW_EA(); break; // SHY
        case 0x9e: MR_AY(); SHX(); MW_EA(); break; // SHX y)

        case 0xc0: MR_IM(); CPY(); break; // CPY
        case 0xc4: MR_ZP(); CPY(); break; // CPY
        case 0xcc: MR_AB(); CPY(); break; // CPY
        case 0xe0: MR_IM(); CPX(); break; // CPX
        case 0xe4: MR_ZP(); CPX(); break; // CPX
        case 0xec: MR_AB(); CPX(); break; // CPX

        case 0xe6: MR_ZP(); INC(); MW_ZP(); break; // INC
        case 0xee: MR_AB(); INC(); MW_EA(); break; // INC
        case 0xf6: MR_ZX(); INC(); MW_ZP(); break; // INC
        case 0xfe: MR_AX(); INC(); MW_EA(); break; // INC
        case 0xc6: MR_ZP(); DEC(); MW_ZP(); break; // DEC
        case 0xce: MR_AB(); DEC(); MW_EA(); break; // DEC
        case 0xd6: MR_ZX(); DEC(); MW_ZP(); break; // DEC
        case 0xde: MR_AX(); DEC(); MW_EA(); break; // DEC

        case 0xe8: INX(); break; // INX
        case 0xc8: INY(); break; // INY
        case 0xca: DEX(); break; // DEX
        case 0x88: DEY(); break; // DEY

        case 0xc3: MR_IX(); DCP(); MW_EA(); break; // DCP
        case 0xc7: MR_ZP(); DCP(); MW_ZP(); break; // DCP
        case 0xcf: MR_AB(); DCP(); MW_EA(); break; // DCP
        case 0xd3: MR_IY(); DCP(); MW_EA(); break; // DCP
        case 0xd7: MR_ZX(); DCP(); MW_ZP(); break; // DCP
        case 0xdb: MR_AY(); DCP(); MW_EA(); break; // DCP
        case 0xdf: MR_AX(); DCP(); MW_EA(); break; // DCP

        case 0xe3: MR_IX(); ISB(); MW_EA(); break; // ISB
        case 0xe7: MR_ZP(); ISB(); MW_ZP(); break; // ISB
        case 0xef: MR_AB(); ISB(); MW_EA(); break; // ISB
        case 0xf3: MR_IY(); ISB(); MW_EA(); break; // ISB
        case 0xf7: MR_ZX(); ISB(); MW_ZP(); break; // ISB
        case 0xfb: MR_AY(); ISB(); MW_EA(); break; // ISB
        case 0xff: MR_AX(); ISB(); MW_EA(); break; // ISB

        case 0x24: MR_ZP(); BIT(); break; // BIT
        case 0x2c: MR_AB(); BIT(); break; // BIT

        case 0x00: BRK(); break; // BRK
        case 0x20: JSR(); break; // JSR
        case 0x40: RTI(); break; // RTI
        case 0x60: RTS(); break; // RTS
        case 0x08: PHP(); break; // PHP
        case 0x28: PLP(); break; // PLP
        case 0x48: PHA(); break; // PHA
        case 0x68: PLA(); break; // PLA

        case 0x4c: JMP_4C(); break; // JMP
        case 0x6c: JMP_6C(); break; // JMP ()

        case 0x10: MR_IM(); BPL(); break; // BPL
        case 0x30: MR_IM(); BMI(); break; // BMI
        case 0x50: MR_IM(); BVC(); break; // BVC
        case 0x70: MR_IM(); BVS(); break; // BVS
        case 0x90: MR_IM(); BCC(); break; // BCC
        case 0xb0: MR_IM(); BCS(); break; // BCS
        case 0xd0: MR_IM(); BNE(); break; // BNE
        case 0xf0: MR_IM(); BEQ(); break; // BEQ

        case 0x18: CLC(); break; // CLC
        case 0x38: SEC(); break; // SEC
        case 0x58: CLI(); break; // CLI
        case 0x78: SEI(); break; // SEI
        case 0xb8: CLV(); break; // CLV
        case 0xd8: CLD(); break; // CLD
        case 0xf8: SED(); break; // SED

        case 0x8a: TXA(); break; // TXA
        case 0xaa: TAX(); break; // TAX
        case 0x98: TYA(); break; // TYA
        case 0xa8: TAY(); break; // TAY
        case 0x9a: TXS(); break; // TXS
        case 0xba: TSX(); break; // TSX

        case 0x0b: MR_IM(); ANC(); break; // ANC
        case 0x2b: MR_IM(); ANC(); break; // ANC
        case 0x4b: MR_IM(); ASR(); break; // ASR
        case 0x6b: MR_IM(); ARR(); break; // ARR
        case 0x8b: MR_IM(); ANE(); break; // ANE
        case 0xcb: MR_IM(); SBX(); break; // SBX
        case 0xea: NOP(); break; // NOP
        default:   UDF(); break; // undefined
        }

        if (cpu->intr_flag & NMI_FLAG) {
            cpu->intr_flag &= ~NMI_FLAG;
            PUSH((PC >> 8) & 0xff);
            PUSH((PC >> 0) & 0xff);
            CLR_FLAG(B_FLAG);
            PUSH(PS);
            SET_FLAG(I_FLAG);
            PC = READW(NMI_VECTOR);
        }
    }

    cpu->cycles_real -= ncycle;
}

