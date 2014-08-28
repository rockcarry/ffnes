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
        ndb->stop = --(*dwparam) ? 0 : 1;
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

    ndb->cond = cond;
    switch (cond)
    {
    case NDB_CPU_RUN_NSTEPS:
        *dwparam = *(DWORD*)param;
        if (*dwparam == 0)
        {
            *dwparam  = 1;
            ndb->stop = 1;
            return;
        }
        break;

    case NDB_CPU_STOP_PCEQU:
        * wparam = *( WORD*)param;
        break;
    }
    ndb->stop = 0;
}

void ndb_cpu_info_str(NDB *ndb, char *str)
{
    static char psflag_chars[8] = {'C', 'Z', 'I', 'D', 'B', '-', 'V', 'N'};
    int  i;

    sprintf(str, "%04X  %02X  %02X  %02X  %02X  ",
        ndb->cpu->pc, ndb->cpu->sp, ndb->cpu->ax, ndb->cpu->xi, ndb->cpu->yi);

    str += strlen(str);
    for (i=7; i>=0; i--)
    {
        if (ndb->cpu->ps & (1 << i)) {
            *str++ = psflag_chars[i];
        }
        else *str++ = '-';
    }
    *str++ = '\0';
}

