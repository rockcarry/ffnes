#ifndef _NES_NDB_H_
#define _NES_NDB_H_

// 包含头文件
#include "stdefine.h"

// 常量定义
enum {
    NDB_CPU_RUN_DEBUG,    // cpu run debug
    NDB_CPU_RUN_NSTEPS,   // cpu run n steps
    NDB_CPU_RUN_STEP_IN,  // cpu run step in
    NDB_CPU_RUN_STEP_OUT, // cpu run step out
    NDB_CPU_RUN_STEP_OVER,// cpu run step over
};

enum {
    NDB_DUMP_CPU_REGS0 ,  // dump cpu regs info0
    NDB_DUMP_CPU_REGS1 ,  // dump cpu regs info1
    NDB_DUMP_CPU_STACK0,  // dump cpu stack info0
    NDB_DUMP_CPU_STACK1,  // dump cpu stack info1
    NDB_DUMP_CPU_VECTOR,  // dump cpu rst & nmi & irq vector
    NDB_DUMP_BREAK_POINT0,// dump cpu break points info0
    NDB_DUMP_BREAK_POINT1,// dump cpu break points info1
    NDB_DUMP_WATCH0,      // dump cpu watches info0
    NDB_DUMP_WATCH1,      // dump cpu watches info1
    NDB_DUMP_WATCH2,      // dump cpu watches info2
    NDB_DUMP_WATCH3,      // dump cpu watches info3
    NDB_DUMP_BANKSW,      // dump cpu bank switch info
};

enum {
    NDB_DBT_NO_BRANCH,  // no branch
    NDB_DBT_JMP_DIRECT, // jump directly
    NDB_DBT_JMP_ONCOND, // jump on condition
    NDB_DBT_CALL_SUB,   // call subroutine (JSR)
    NDB_DBT_STOP_DASM,  // stop disassemble
};

enum {
    NDB_DEBUG_MODE_DISABLE,
    NDB_DEBUG_MODE_ENABLE,
    NDB_DEBUG_MODE_RESTART,
};

// 类型声明
struct tagNES;

// 类型定义
typedef struct tagNDB
{
    // pointer to nes
    struct tagNES *nes;

    BOOL  enable;
    BOOL  banksw;
    int   stop;
    int   cond;
    WORD  curpc;       // current pc
    BYTE  curopcode;   // current opcode
    int   nsteps;      // for run n steps
    WORD  bpoints[16]; // totally 16 break points
    WORD  watches[16]; // totally 16 watch variables

    WORD  pcstackbuf[128];
    int   pcstacktop;
    WORD  pcstepout;
    WORD  pcstepover;

    // for save & restore
    BOOL  save_enable;
} NDB;

typedef struct
{
    // map table for pc to instruction n
    WORD pc2instn_maptab[0x8000];
    int  curinstn;

    // instruction list
    struct {
        WORD pc;            // pc
        BYTE len;           // instruction lenght
        BYTE bytes  [ 3];   // instruction bytes
        char asmstr [16];   // asm string
        char comment[32];   // comment
    } instlist[0x8000];
} DASM;

// 函数声明
void ndb_init (NDB *ndb, void *nes);
void ndb_free (NDB *ndb);
void ndb_reset(NDB *ndb);

// set ndb debug mode
void ndb_set_debug(NDB *ndb, BOOL en);

// debug cpu
void ndb_cpu_debug(NDB *ndb);
void ndb_cpu_runto(NDB *ndb, int cond, DWORD dwparam);

// ndb disasm
int  ndb_dasm_one_inst(NDB *ndb, WORD pc, BYTE bytes[3], char *str, char *comment, int *btype, WORD *entry);
void ndb_dasm_nes_rom_begin(NDB *ndb, DASM *dasm);
void ndb_dasm_nes_rom_entry(NDB *ndb, DASM *dasm, WORD entry);
void ndb_dasm_nes_rom_done (NDB *ndb, DASM *dasm);
int  ndb_dasm_pc2instn(NDB *ndb, DASM *dasm, WORD pc);

// break point & watch
BOOL ndb_add_bpoint(NDB *ndb, WORD bpoint);
void ndb_del_bpoint(NDB *ndb, WORD bpoint);
BOOL ndb_add_watch (NDB *ndb, WORD watch );
void ndb_del_watch (NDB *ndb, WORD watch );

// ndb dump info
void ndb_dump_info(NDB *ndb, int type, char *str);
void ndb_dump_ppu (NDB *ndb, void *bmpbuf, int w, int h, int stride);

#endif






