// 包含头文件
#include "mem.h"

// 函数实现
void mem_read(MEM *pm, int addr, BYTE *byte)
{
    *byte = pm->data[addr % pm->size];
}

void mem_write(MEM *pm, int addr, BYTE byte)
{
    if (pm->type != MEM_ROM) {
        pm->data[addr % pm->size] = byte;
    }
}

