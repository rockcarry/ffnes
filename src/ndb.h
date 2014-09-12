#ifndef _NES_NDB_H_
#define _NES_NDB_H_

// 包含头文件
#include "stdefine.h"
#include "cpu.h"

// 常量定义
enum {
    NDB_CPU_KEEP_RUNNING, // cpu keep running
    NDB_CPU_RUN_NSTEPS,   // cpu run n instructions then stop
    NDB_CPU_RUN_BPOINTS,  // cpu run to break points
};

enum {
    NDB_DUMP_CPU_REGS0 ,  // dump cpu regs info0
    NDB_DUMP_CPU_REGS1 ,  // dump cpu regs info1
    NDB_DUMP_CPU_STACK0,  // dump cpu stack info0
    NDB_DUMP_CPU_STACK1,  // dump cpu stack info1
    NDB_DUMP_CPU_VECTOR,  // dump cpu rst & nmi & irq vector
    NDB_DUMP_BREAK_POINT0,// dump cpu rst & nmi & irq vector
    NDB_DUMP_BREAK_POINT1,// dump cpu rst & nmi & irq vector
    NDB_DUMP_WATCH0,      // dump cpu rst & nmi & irq vector
    NDB_DUMP_WATCH1,      // dump cpu rst & nmi & irq vector
    NDB_DUMP_WATCH2,      // dump cpu rst & nmi & irq vector
    NDB_DUMP_WATCH3,      // dump cpu rst & nmi & irq vector
};

enum {
    NDB_DBT_NO_BRANCH,  // no branch
    NDB_DBT_JMP_DIRECT, // jump directly
    NDB_DBT_JMP_ONCOND, // jump on condition
    NDB_DBT_CALL_SUB,   // call subroutine (JSR)
    NDB_DBT_STOP_DASM,  // stop disassembly
};

// 类型定义
typedef struct
{
    CPU  *cpu;
    BOOL  enable;
    BOOL  banksw;
    int   stop;
    int   cond;
    WORD  curpc;       // current pc
    BYTE  params [32]; // params for cond stop
    WORD  bpoints[16]; // totally 16 break points
    WORD  watches[16]; // totally 16 watch variables

    // for save & restore
    BOOL  save_enable;
    int   save_stop;
    int   save_cond;
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
void ndb_init (NDB *ndb, CPU *cpu);
void ndb_free (NDB *ndb);
void ndb_reset(NDB *ndb);
void ndb_debug(NDB *ndb, BOOL en);

// save & restore
void ndb_save   (NDB *ndb);
void ndb_restore(NDB *ndb);

// debug cpu
void ndb_cpu_debug(NDB *ndb);
void ndb_cpu_runto(NDB *ndb, int cond, void *param);

// ndb disasm
int  ndb_dasm_one_inst(NDB *ndb, WORD pc, BYTE bytes[3], char *str, char *comment, int *btype, WORD *entry);
void ndb_dasm_nes_rom (NDB *ndb, DASM *dasm);
int  ndb_dasm_pc2instn(NDB *ndb, DASM *dasm, WORD pc);

// break point & watch
BOOL ndb_add_bpoint(NDB *ndb, WORD bpoint);
void ndb_del_bpoint(NDB *ndb, WORD bpoint);
BOOL ndb_add_watch (NDB *ndb, WORD watch );
void ndb_del_watch (NDB *ndb, WORD watch );

// ndb dump info
void ndb_dump_info(NDB *ndb, int type, char *str);

#endif






