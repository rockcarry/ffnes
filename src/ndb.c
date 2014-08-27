// 包含头文件
#include <stdio.h>
#include "ndb.h"

// 函数实现
void ndb_init(NDB *ndb, CPU *cpu)
{
    ndb->cpu = cpu;
    ndb_reset(ndb);
}

void ndb_free(NDB *ndb)
{
    // do nothing
}

void ndb_reset(NDB *ndb)
{
    ndb->stop = 0;
    ndb->cond = 0;
}

void ndb_cpu_debug(NDB *ndb)
{
    WORD  * wparam = (WORD *)ndb->param;
    DWORD *dwparam = (DWORD*)ndb->param;

    switch (ndb->cond)
    {
    case NDB_CPU_KEEP_RUNNING:
        ndb->stop = 0;
        break;

    case NDB_CPU_RUN_NSTEPS:
        ndb->stop = --(*dwparam) ? 1 : 0;
        break;

    case NDB_CPU_STOP_PCEQU:
        ndb->stop = (ndb->cpu->pc == *wparam) ? 1 : 0;
        break;
    }

    // while loop make ndb spin here
    while (ndb->stop) Sleep(20);
}

void ndb_cpu_runto(NDB *ndb, int cond, void *param)
{
    WORD  * wparam = (WORD *)ndb->param;
    DWORD *dwparam = (DWORD*)ndb->param;

    switch (cond)
    {
    case NDB_CPU_RUN_NSTEPS:
        *dwparam = *(DWORD*)param;
        break;

    case NDB_CPU_STOP_PCEQU:
        * wparam = *( WORD*)param;
        break;
    }
    ndb->cond = cond;
    ndb->stop = 0;
}

