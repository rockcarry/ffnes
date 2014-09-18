// 包含头文件
#include <stdio.h>
#include "nes.h"

// 内部函数声明
static int ndb_cal_inst_len(BYTE opcode);
static int ndb_cal_inst_adm(BYTE opcode);

// 函数实现
void ndb_init(NDB *ndb, void *nes)
{
    ndb->nes = nes;
    ndb_reset(ndb);
}

void ndb_free(NDB *ndb)
{
    // do nothing
}

void ndb_reset(NDB *ndb)
{
    ndb->enable     = 0;
    ndb->banksw     = 0;
    ndb->stop       = 0;
    ndb->cond       = 0;
    ndb->pcstacktop = 0;
    memset(ndb->bpoints, 0xff, sizeof(ndb->bpoints));
    memset(ndb->watches, 0xff, sizeof(ndb->watches));
}

void ndb_set_debug(NDB *ndb, int mode)
{
    switch (mode)
    {
    case NDB_DEBUG_MODE_DISABLE:
        ndb->save_enable = ndb->enable;
        ndb->enable      = 0;
        ndb->stop        = 0;
        break;
    case NDB_DEBUG_MODE_ENABLE:
        ndb->enable      = 1;
        break;
    case NDB_DEBUG_MODE_RESTART:
        ndb->pcstacktop  = 0;
        ndb->enable      = ndb->save_enable;
        break;
    }
}

void ndb_cpu_debug(NDB *ndb)
{
    int i;

    // if ndb debug is disabled
    if (!ndb->enable) return;

    // save current pc & current opcode
    ndb->curpc     = ndb->nes->cpu.pc;
    ndb->curopcode = bus_readb(ndb->nes->cbus, ndb->curpc);

    if (ndb->curopcode == 0x00 || ndb->curopcode == 0x20) // BRK & JSR
    {
        if (ndb->pcstacktop < 128) ndb->pcstackbuf[ndb->pcstacktop++] = ndb->curpc + 2;
    }
    else if (ndb->curopcode == 0x40 || ndb->curopcode == 0x60) // RTI & RTS
    {
        if (ndb->pcstacktop > 0) ndb->pcstacktop--;
    }

    switch (ndb->cond)
    {
    case NDB_CPU_RUN_NSTEPS:
        if (ndb->nsteps > 0) ndb->nsteps--;
        ndb->stop = (ndb->nsteps > 0) ? 0 : 1;
        break;

    case NDB_CPU_RUN_STEP_IN:
        ndb->stop = 1;
        break;

    case NDB_CPU_RUN_STEP_OUT:
        if (ndb->pcstepout != 0xffff)
        {
            if (  ndb->curpc == ndb->pcstepout + 0
               || ndb->curpc == ndb->pcstepout + 1)
            {
                ndb->stop = 1;
            }
        }
        break;

    case NDB_CPU_RUN_STEP_OVER:
        if (ndb->pcstepover != 0xffff)
        {
            if (ndb->curpc == ndb->pcstepover) ndb->stop = 1;
        }
        else ndb->stop = 1;
        break;
    }

    //++ for break points
    for (i=0; i<16; i++)
    {
        if (ndb->curpc == ndb->bpoints[i])
        {
            ndb->stop = 1;
            break;
        }
    }
    //-- for break points

    // while loop make ndb spin here
    while (ndb->stop) Sleep(20);
}

void ndb_cpu_runto(NDB *ndb, int cond, DWORD dwparam)
{
    // save for cond
    ndb->cond = cond;

    switch (cond)
    {
    case NDB_CPU_RUN_NSTEPS:
        ndb->nsteps = (int)dwparam;
        if (ndb->nsteps <= 0) ndb->stop = 1;
        else                  ndb->stop = 0;
        break;

    case NDB_CPU_RUN_STEP_OUT:
        if (ndb->pcstacktop > 0)
        {
            ndb->pcstepout = ndb->pcstackbuf[ndb->pcstacktop - 1];
        }
        else ndb->pcstepout = 0xffff;
        ndb->stop = 0;
        break;

    case NDB_CPU_RUN_STEP_OVER:
        if (ndb->curopcode == 0x00 || ndb->curopcode == 0x20)
        {
            ndb->pcstepover = ndb->curpc + ndb_cal_inst_len(ndb->curopcode);
        }
        else ndb->pcstepover = 0xffff;
        ndb->stop = 0;
        break;

    default:
        ndb->stop = 0;
        break;
    }
}

static int ndb_cal_inst_len(BYTE opcode)
{
    static int inst_len_map[32] = { 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,  2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3 };
           int len = inst_len_map[opcode & 0x1f];
    // instruction length for spcial opcode
    switch (opcode)
    {
    case 0x00: case 0x40: case 0x60: case 0x02: case 0x22: case 0x42: case 0x62: len = 1; break;
    case 0x20:                                                                   len = 3; break;
    }
    return len;
}

static int ndb_cal_inst_adm(BYTE opcode)
{
    static int inst_adm_map[32] = { 0, 7, 0, 7, 2, 2, 2, 2, 0, 1, 0, 1, 4, 4, 4, 4, 10, 8, 9, 8, 0, 3, 3, 3, 0, 6, 0, 6, 0, 5, 5, 5 };
           int adm = inst_adm_map[opcode & 0x1f];
    // instruction addressing mode for spcial opcode
    switch (opcode)
    {
    case 0x20:                                             adm =  4; break;
    case 0xa0: case 0xc0: case 0xe0: case 0xa2:            adm =  1; break;
    case 0x04: case 0x44: case 0x64: case 0x89: case 0x0c: adm =  0; break;
    case 0x6c:                                             adm = 12; break;
    case 0x94: case 0xb4:                                  adm =  3; break;
    case 0x9c: case 0xbc:                                  adm =  5; break;
    case 0x96: case 0xb6: case 0x97: case 0xb7:            adm = 11; break;
    case 0x9e: case 0xbe: case 0x9f: case 0xbf:            adm =  6; break;
    }
    return adm;
}

int ndb_dasm_one_inst(NDB *ndb, WORD pc, BYTE bytes[3], char *str, char *comment, int *btype, WORD *entry)
{
    static char *s_opcode_strs[256] =
    {
        "BRK", "ORA", " t ", "SLO", "NOP", "ORA", "ASL", "SLO", "PHP", "ORA", "ASL", "ANC", "NOP", "ORA", "ASL", "SLO",
        "BPL", "ORA", " t ", "SLO", "NOP", "ORA", "ASL", "SLO", "CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO",
        "JSR", "AND", " t ", "RLA", "BIT", "AND", "ROL", "RLA", "PLP", "AND", "ROL", "ANC", "BIT", "AND", "ROL", "RLA",
        "BMI", "AND", " t ", "RLA", "NOP", "AND", "ROL", "RLA", "SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA",
        "RTI", "EOR", " t ", "SRE", "NOP", "EOR", "LSR", "SRE", "PHA", "EOR", "LSR", "ASR", "JMP", "EOR", "LSR", "SRE",
        "BVC", "EOR", " t ", "SRE", "NOP", "EOR", "LSR", "SRE", "CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE",
        "RTS", "ADC", " t ", "RRA", "NOP", "ADC", "ROR", "RRA", "PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA",
        "BVS", "ADC", " t ", "RRA", "NOP", "ADC", "ROR", "RRA", "SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA",
        "NOP", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX", "DEY", "NOP", "TXA", "ANE", "STY", "STA", "STX", "SAX",
        "BCC", "STA", " t ", "SHA", "STY", "STA", "STX", "SAX", "TYA", "STA", "TXS", "SHS", "SHY", "STA", "SHX", "SHA",
        "LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX", "TAY", "LDA", "TAX", "LXA", "LDY", "LDA", "LDX", "LAX",
        "BCS", "LDA", " t ", "LAX", "LDY", "LDA", "LDX", "LAX", "CLV", "LDA", "TSX", "LAS", "LDY", "LDA", "LDX", "LAX",
        "CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP", "INY", "CMP", "DEX", "SBX", "CPY", "CMP", "DEC", "DCP",
        "BNE", "CMP", " t ", "DCP", "NOP", "CMP", "DEC", "DCP", "CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP",
        "CPX", "SBC", "NOP", "ISB", "CPX", "SBC", "INC", "ISB", "INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISB",
        "BEQ", "SBC", " t ", "ISB", "NOP", "SBC", "INC", "ISB", "SED", "SBC", "NOP", "ISB", "NOP", "SBC", "INC", "ISB",
    };

    int len, adm;

    bytes[0] = bus_readb(ndb->nes->cbus, pc + 0);
    bytes[1] = bus_readb(ndb->nes->cbus, pc + 1);
    bytes[2] = bus_readb(ndb->nes->cbus, pc + 2);

    // instruction length & addressing mode
    len = ndb_cal_inst_len(bytes[0]);
    adm = ndb_cal_inst_adm(bytes[0]);

    // for different am
    switch (adm)
    {
    case  0: sprintf(str, "%s"             , s_opcode_strs[bytes[0]]                     ); break; // Implied
    case  1: sprintf(str, "%s #%02X"       , s_opcode_strs[bytes[0]], bytes[1]           ); break; // immediate
    case  2: sprintf(str, "%s $%02X"       , s_opcode_strs[bytes[0]], bytes[1]           ); break; // zeropage
    case  3: sprintf(str, "%s $%02X, X"    , s_opcode_strs[bytes[0]], bytes[1]           ); break; // zeropage,x
    case  4: sprintf(str, "%s $%02X%02X"   , s_opcode_strs[bytes[0]], bytes[2], bytes[1] ); break; // absolute
    case  5: sprintf(str, "%s $%02X%02X, X", s_opcode_strs[bytes[0]], bytes[2], bytes[1] ); break; // absolute,x
    case  6: sprintf(str, "%s $%02X%02X, Y", s_opcode_strs[bytes[0]], bytes[2], bytes[1] ); break; // absolute,y
    case  7: sprintf(str, "%s ($%02X, X)"  , s_opcode_strs[bytes[0]], bytes[1]           ); break; // (indir,x)
    case  8: sprintf(str, "%s ($%02X), Y"  , s_opcode_strs[bytes[0]], bytes[1]           ); break; // (indir),y
    case  9: sprintf(str, "%s"             , s_opcode_strs[bytes[0]]                     ); break; // ?
    case 10: sprintf(str, "%s $%04X"       , s_opcode_strs[bytes[0]], pc+2+(char)bytes[1]); break; // relative
    case 11: sprintf(str, "%s $%02X, Y"    , s_opcode_strs[bytes[0]], bytes[1]           ); break; // zeropage,y
    case 12: sprintf(str, "%s ($%02X%02X)" , s_opcode_strs[bytes[0]], bytes[2], bytes[1] ); break; // indirect, JMP ()
    }

    //++ for branch type & entry
    if (TRUE)             { *btype = NDB_DBT_NO_BRANCH ; *entry = 0;                                 } // default
    if (bytes[0] == 0x4c) { *btype = NDB_DBT_JMP_DIRECT; *entry = (bytes[2] << 8) | (bytes[1] << 0); } // JMP
    if (bytes[0] == 0x6c) {                                                                            // JMP () ++
        *entry = (bytes[2] << 8) | (bytes[1] << 0);
        if (*entry > 0x8000) {
            *btype = NDB_DBT_JMP_DIRECT;
            *entry = bus_readw(ndb->nes->cbus, *entry);
        }
    }                                                                                                  // JMP () --
    if (bytes[0] == 0x20) { *btype = NDB_DBT_CALL_SUB  ; *entry = (bytes[2] << 8) | (bytes[1] << 0); } // JSR
    if (adm == 10)        { *btype = NDB_DBT_JMP_ONCOND; *entry = (pc + 2 + (char)bytes[1]);         } // BPL BMI BVC BVS BCC BCS BNE BEQ
    if (bytes[0] == 0x00 || bytes[0] == 0x40 || bytes[0] == 0x60) { *btype = NDB_DBT_STOP_DASM;      } // BRK RTI RTS
    //-- for branch type & entry

    //++ for instruction comment ++//
    // todo..
    //-- for instruction comment --//

    return len;
}

void ndb_dasm_nes_rom_entry(NDB *ndb, DASM *dasm, WORD entry)
{
    int  len;
    WORD pc;
    int  branchtype;
    WORD branchentry;

    if (entry < 0x8000 || dasm->pc2instn_maptab[entry - 0x8000] != 0) return;

    for (pc=entry; pc<0xfffa; pc+=len)
    {
        // dasm one instruction
        len = ndb_dasm_one_inst(ndb, pc,
            dasm->instlist[dasm->curinstn].bytes,
            dasm->instlist[dasm->curinstn].asmstr,
            dasm->instlist[dasm->curinstn].comment,
            &branchtype, &branchentry);
        dasm->instlist[dasm->curinstn].pc  = pc;
        dasm->instlist[dasm->curinstn].len = len;

        // if this position of pc is already disassemblied
        if (dasm->pc2instn_maptab[pc - 0x8000]) break;

        // mark this pc for disassemble
        dasm->pc2instn_maptab[pc - 0x8000] = 1;
        dasm->curinstn++; // next instn

        // for branchtype == NDB_DBT_STOP_DASM
        if (branchtype == NDB_DBT_STOP_DASM) break;

        //++ for jump, branch or call sub instruction
        if (branchentry > 0x8000 && dasm->pc2instn_maptab[branchentry - 0x8000] == 0)
        {
            switch (branchtype)
            {
            case NDB_DBT_JMP_DIRECT:
                ndb_dasm_nes_rom_entry(ndb, dasm, branchentry);
                goto done;

            case NDB_DBT_JMP_ONCOND:
            case NDB_DBT_CALL_SUB:
                ndb_dasm_nes_rom_entry(ndb, dasm, branchentry);
                break;
            }
        }
        //-- for jump, branch or call sub instruction
    }

done:
    return;
}

static int cmp_inst_item_by_pc(const void *item1, const void *item2)
{
    return *((WORD*)item1) - *((WORD*)item2);
}

void ndb_dasm_nes_rom_begin(NDB *ndb, DASM *dasm)
{
    memset(dasm, 0, sizeof(DASM)); // clear all
}

void ndb_dasm_nes_rom_done(NDB *ndb, DASM *dasm)
{
    int i;

    // quick sort instlist
    qsort(dasm->instlist, dasm->curinstn, sizeof(dasm->instlist[0]), cmp_inst_item_by_pc);

    // make pc to instrution index map table
    for (i=0; i<dasm->curinstn; i++) dasm->pc2instn_maptab[dasm->instlist[i].pc - 0x8000] = i;
}

int ndb_dasm_pc2instn(NDB *ndb, DASM *dasm, WORD pc)
{
    if (pc < 0x8000) return 0;
    else return dasm->pc2instn_maptab[pc - 0x8000];
}

BOOL ndb_add_bpoint(NDB *ndb, WORD bpoint)
{
    int i, free = -1;
    for (i=0; i<16; i++)
    {
        if (ndb->bpoints[i] == bpoint) return TRUE;
        if (ndb->bpoints[i] == 0xffff && free == -1) free = i;
    }
    if (free != -1)
    {
        ndb->bpoints[free] = bpoint;
        return TRUE;
    }
    return FALSE;
}

void ndb_del_bpoint(NDB *ndb, WORD bpoint)
{
    int i;
    for (i=0; i<16; i++)
    {
        if (ndb->bpoints[i] == bpoint) break;
    }
    if (i < 16) ndb->bpoints[i] = 0xffff;
}

BOOL ndb_add_watch(NDB *ndb, WORD watch)
{
    int i, free = -1;
    for (i=0; i<16; i++)
    {
        if (ndb->watches[i] == watch) return TRUE;
        if (ndb->watches[i] == 0xffff && free == -1) free = i;
    }
    if (free != -1)
    {
        ndb->watches[free] = watch;
        return TRUE;
    }
    return FALSE;
}

void ndb_del_watch(NDB *ndb, WORD watch)
{
    int i;
    for (i=0; i<16; i++)
    {
        if (ndb->watches[i] == watch) break;
    }
    if (i < 16) ndb->watches[i] = 0xffff;
}

void ndb_del_all_bpoints(NDB *ndb)
{
    memset(ndb->bpoints, 0xff, sizeof(ndb->bpoints));
}

void ndb_del_all_watches(NDB *ndb)
{
    memset(ndb->watches, 0xff, sizeof(ndb->watches));
}

static void ndb_dump_cpu_regs0(NDB *ndb, char *str)
{
    strcpy(str, " pc   ax  xi  yi  sp     ps");
}

static void ndb_dump_cpu_regs1(NDB *ndb, char *str)
{
    static char psflag_chars[8] = {'C', 'Z', 'I', 'D', 'B', '-', 'V', 'N'};
    int  i;

    sprintf(str, "%04X  %02X  %02X  %02X  %02X  ",
        ndb->nes->cpu.pc, ndb->nes->cpu.ax, ndb->nes->cpu.xi, ndb->nes->cpu.yi, ndb->nes->cpu.sp);

    str += strlen(str);
    for (i=7; i>=0; i--)
    {
        if (ndb->nes->cpu.ps & (1 << i)) {
            *str++ = psflag_chars[i];
        }
        else *str++ = '-';
    }
    *str++ = '\0';
}

static void ndb_dump_cpu_stack0(NDB *ndb, char *str)
{
    int  top    = 0x100 + ndb->nes->cpu.sp;
    int  bottom = (top & 0xfff0) + 0xf;

    sprintf(str, "%02X                   stack                   %02X",
        (bottom & 0xff), (bottom & 0xf0));
}

static void ndb_dump_cpu_stack1(NDB *ndb, char *str)
{
    int  top     = 0x100 + ndb->nes->cpu.sp;
    int  bottom  = (top & 0xfff0) + 0xf;
    char byte[8] = {0};
    int  i;

    str[0] = '\0';
    for (i=bottom; i>bottom-16; i--)
    {
        if (i > top) sprintf(byte, "%02X ", ndb->nes->cpu.cram[i]);
        else         sprintf(byte, "-- ");
        strcat (str, byte);
    }
}

static void ndb_dump_cpu_vector(NDB *ndb, char *str)
{
    sprintf(str, "reset: %04X\nnmi  : %04X\nirq  : %04X",
        bus_readw(ndb->nes->cbus, 0xfffc),
        bus_readw(ndb->nes->cbus, 0xfffa),
        bus_readw(ndb->nes->cbus, 0xfffe));
}

static void ndb_dump_break_point(NDB *ndb, int type, char *str)
{
    WORD *bps = ndb->bpoints;
    int   i;

    if (type == NDB_DUMP_BREAK_POINT1) bps += 8;
    for (i=0; i<8; i++)
    {
        if (bps[i] == 0xffff) {
            sprintf(str+i*5, "---- ");
        }
        else sprintf(str+i*5, "%04X ", bps[i]);
    }
}

static void ndb_dump_watch(NDB *ndb, int type, char *str)
{
    WORD *wts = ndb->watches;
    BYTE  byte;
    int   i;

    if (type == NDB_DUMP_WATCH2 || type == NDB_DUMP_WATCH3) wts += 8;

    if (type == NDB_DUMP_WATCH0 || type == NDB_DUMP_WATCH2)
    {
        sprintf(str, "addr "); str += 5;
        for (i=0; i<8; i++)
        {
            if (wts[i] == 0xffff) sprintf(str+i*5, "---- ");
            else sprintf(str+i*5, "%04X ", wts[i]);
        }
    }
    else
    {
        sprintf(str, "data "); str += 5;
        for (i=0; i<8; i++)
        {
            if (wts[i] == 0xffff) sprintf(str+i*5, " --  ");
            else
            {
                byte = bus_readb_norwcb(ndb->nes->cbus, wts[i]);
                sprintf(str+i*5, " %02X  ", byte);
            }
        }
    }
}

static void ndb_dump_banksw(NDB *ndb, char *str)
{
    sprintf(str, "mapper%03d:\r\n8000 %2d\r\nC000 %2d", ndb->nes->mmc.number, ndb->nes->mmc.bank8000, ndb->nes->mmc.bankc000);
}

void ndb_dump_info(NDB *ndb, int type, char *str)
{
    switch (type)
    {
    case NDB_DUMP_CPU_REGS0   : ndb_dump_cpu_regs0  (ndb, str); break;
    case NDB_DUMP_CPU_REGS1   : ndb_dump_cpu_regs1  (ndb, str); break;
    case NDB_DUMP_CPU_STACK0  : ndb_dump_cpu_stack0 (ndb, str); break;
    case NDB_DUMP_CPU_STACK1  : ndb_dump_cpu_stack1 (ndb, str); break;
    case NDB_DUMP_CPU_VECTOR  : ndb_dump_cpu_vector (ndb, str); break;
    case NDB_DUMP_BREAK_POINT0:
    case NDB_DUMP_BREAK_POINT1: ndb_dump_break_point(ndb, type, str); break;
    case NDB_DUMP_WATCH0      :
    case NDB_DUMP_WATCH1      :
    case NDB_DUMP_WATCH2      :
    case NDB_DUMP_WATCH3      : ndb_dump_watch      (ndb, type, str); break;
    case NDB_DUMP_BANKSW      : ndb_dump_banksw     (ndb, str); break;
    }
}

static void render_name_table(void *bmp, int stride, BYTE *vram, BYTE *chrom, BYTE *pal0, BYTE *pal1, int div)
{
    BYTE  *ntab = vram + 0x0000;
    BYTE  *atab = vram + 0x03c0;
    DWORD *dwbuf;
    BYTE   adata;
    BYTE   tdata;
    BYTE   cdatl;
    BYTE   cdath;
    BYTE   r, g, b;
    int    ax, ay, sx, sy, tx, ty;
    int    n, s, i, j;
    int    maxatabsize = (div == 1) ? 64  : (64   / div);
    int    maxntabsize = (div == 1) ? 960 : (1024 / div);
    int    maxscanline = (div == 1) ? 240 : (256  / div);

    for (n=0; n<maxatabsize; n++)
    {
        adata = atab[n];
        ax = (n & 0x7) * 32;
        ay = (n >>  3) * 32;

        for (s=0; s<4; s++)
        {
            sx = ax + (s & 0x1) * 16;
            sy = ay + (s >>  1) * 16;
            dwbuf = (DWORD*)bmp + sy * stride + sx;

            for (i=0; i<16; i++)
            {
                for (j=0; j<16; j++)
                {
                    *dwbuf++ = (adata & 0x3) << 2;
                }
                dwbuf -= 16;
                dwbuf += stride;
            }
            adata >>= 2;
        }
    }

    for (n=0; n<maxntabsize; n++)
    {
        tx = (n & 0x1f) * 8;
        ty = (n >>   5) * 8;
        tdata = ntab[n];
        dwbuf = (DWORD*)bmp + ty * stride + tx;

        for (i=0; i<8; i++)
        {
            cdatl = chrom[tdata * 16 + i + 0];
            cdath = chrom[tdata * 16 + i + 8];
            for (j=0; j<8; j++)
            {
                *dwbuf += ((cdath >> 7) << 1) | ((cdatl >> 7) << 0);
                 dwbuf ++;
                cdath <<= 1;
                cdatl <<= 1;
            }
            dwbuf -= 8;
            dwbuf += stride;
        }
    }

    dwbuf = (DWORD*)bmp;
    for (i=0; i<maxscanline; i++)
    {
        for (j=0; j<256; j++)
        {
            r = pal1[(pal0[*dwbuf] & 0x3f) * 3 + 2];
            g = pal1[(pal0[*dwbuf] & 0x3f) * 3 + 1];
            b = pal1[(pal0[*dwbuf] & 0x3f) * 3 + 0];
            *dwbuf++ = RGB(r, g, b);
        }
        dwbuf -= 256;
        dwbuf += stride;
    }
}

static void draw_color_bar(void *bmp, int stride, int x, int y, int color, BYTE *pal0, BYTE *pal1)
{
    DWORD *dwbuf   = (DWORD*)bmp + y * stride + x;
    DWORD  dwcolor = RGB(pal1[pal0[color] * 3 + 2], pal1[pal0[color] * 3 + 1], pal1[pal0[color] * 3 + 0]);
    int    i, j;
    for (i=0; i<16; i++)
    {
        for (j=0; j<16; j++)
        {
            if (i==0 || i==15 || j==0) *dwbuf++ = RGB(255,255,255);
            else *dwbuf++ = dwcolor;
        }
        dwbuf -= 16;
        dwbuf += stride;
    }
}

void ndb_dump_ppu(NDB *ndb, void *bmpbuf, int w, int h, int stride)
{
    BYTE *tilebkg = ndb->nes->ppu.chrom_bkg;
    BYTE *tilespr = ndb->nes->ppu.chrom_spr;
    BYTE *ntab0   = ndb->nes->vram[0].data;
    BYTE *ntab1   = ndb->nes->vram[1].data;
    BYTE *ntab2   = ndb->nes->vram[2].data;
    BYTE *ntab3   = ndb->nes->vram[3].data;
    BYTE *pal0    = ndb->nes->ppu.palette;
    BYTE *pal1    = DEF_PPU_PAL;
    BYTE  ntabtmp[1024] = {0};
    int   i;

    render_name_table((DWORD*)bmpbuf + 0   * stride + 0  , stride, ntab0, tilebkg, pal0, pal1, 1);
    render_name_table((DWORD*)bmpbuf + 0   * stride + 256, stride, ntab1, tilebkg, pal0, pal1, 1);
    render_name_table((DWORD*)bmpbuf + 240 * stride + 0  , stride, ntab2, tilebkg, pal0, pal1, 1);
    render_name_table((DWORD*)bmpbuf + 240 * stride + 256, stride, ntab3, tilebkg, pal0, pal1, 1);

    //++ make sure tilebkg & tilespr not point to the same
    if (tilespr == tilebkg)
    {
        if (tilespr != ndb->nes->chrrom.data) tilespr = ndb->nes->chrrom.data;
        else tilespr = ndb->nes->chrrom.data + 0x1000;
    }
    //-- make sure tilebkg & tilespr not point to the same
    for (i=0; i<256; i++) ntabtmp[i] = i;
    render_name_table((DWORD*)bmpbuf + 32  * stride + 512 + 5, stride, ntabtmp, tilebkg, pal0, pal1, 4);
    render_name_table((DWORD*)bmpbuf + 128 * stride + 512 + 5, stride, ntabtmp, tilespr, pal0, pal1, 4);

    for (i=0; i<16; i++)
    {
        draw_color_bar(bmpbuf, stride, 512 + 5 + i * 16, 224 + 0 , 0  + i, pal0, pal1);
        draw_color_bar(bmpbuf, stride, 512 + 5 + i * 16, 224 + 48, 16 + i, pal0, pal1);
    }
}
