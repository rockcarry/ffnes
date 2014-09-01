#ifndef _NES_NDB_H_
#define _NES_NDB_H_

// 包含头文件
#include "stdefine.h"
#include "cpu.h"

// 常量定义
enum {
    NDB_CPU_KEEP_RUNNING, // cpu keep running
    NDB_CPU_RUN_NSTEPS,   // cpu run n instructions then stop
    NDB_CPU_STOP_PCEQU,   // cpu keep running, but when pc == xxxx will stop
};

enum {
    NDB_DUMP_CPU_REGS0 , // dump cpu regs info0
    NDB_DUMP_CPU_REGS1 , // dump cpu regs info1
    NDB_DUMP_CPU_STACK0, // dump cpu stack info0
    NDB_DUMP_CPU_STACK1, // dump cpu stack info1
    NDB_DUMP_CPU_VECTOR, // dump cpu rst & nmi & irq vector
};

// 类型定义
typedef struct
{
    CPU  *cpu;
    int   stop;
    int   cond;
    BYTE  param[32];

    // for saving debugging status
    int save_stop;
    int save_cond;
} NDB;

// 函数声明
void ndb_init (NDB *ndb, CPU *cpu);
void ndb_free (NDB *ndb);
void ndb_reset(NDB *ndb);

// save & restore debugging status
void ndb_save   (NDB *ndb);
void ndb_restore(NDB *ndb);

// debug cpu
void ndb_cpu_debug(NDB *ndb);
void ndb_cpu_runto(NDB *ndb, int cond, void *param);
int  ndb_cpu_dasm (NDB *ndb, WORD pc, BYTE bytes[3], char *str);

// ndb dump info
void ndb_dump_info(NDB *ndb, int type, char *str);

#endif






