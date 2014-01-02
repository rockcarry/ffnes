#ifndef _TEST_

// 包含头文件
#include <stdlib.h>
#include "nes.h"

// 函数声明
BOOL nes_create(NES *nes)
{
    //++ cbus mem map ++//
    // create cpu ram
    nes->cram.type = MEM_RAM;
    nes->cram.size = 0x0800;
    mem_create(&(nes->cram));

    // create ppu regs
    nes->ppuregs.type = MEM_REG;
    nes->ppuregs.size = 0x0008;
    mem_create(&(nes->ppuregs));

    // create ppu regs
    nes->apuregs.type = MEM_REG;
    nes->apuregs.size = 0x0018;
    mem_create(&(nes->apuregs));

    // create expansion rom
    nes->erom.type = MEM_ROM;
    nes->erom.size = 0x1FE8;
    mem_create(&(nes->erom));

    // create sram
    nes->sram.type = MEM_RAM;
    nes->sram.size = 0x2000;
    mem_create(&(nes->sram));

    // create PRG-ROM 0
    nes->prgrom0.type = MEM_ROM;
    nes->prgrom0.size = 0x4000;
    mem_create(&(nes->prgrom0));

    // create PRG-ROM 1
    nes->prgrom1.type = MEM_ROM;
    nes->prgrom1.size = 0x4000;
    mem_create(&(nes->prgrom1));

    // init nes cbus
    bus_setmap(nes->cbus, 0, 0x0000, 0x1FFF, &(nes->cram   ));
    bus_setmap(nes->cbus, 1, 0x2000, 0x3FFF, &(nes->ppuregs));
    bus_setmap(nes->cbus, 2, 0x4000, 0x4017, &(nes->apuregs));
    bus_setmap(nes->cbus, 3, 0x4018, 0x5FFF, &(nes->erom   ));
    bus_setmap(nes->cbus, 4, 0x6000, 0x7FFF, &(nes->sram   ));
    bus_setmap(nes->cbus, 5, 0x8000, 0xBFFF, &(nes->prgrom0));
    bus_setmap(nes->cbus, 6, 0xC000, 0xFFFF, &(nes->prgrom1));
    bus_setmap(nes->cbus, 7, 0x0000, 0x0000, NULL           );
    //-- cbus mem map --//

    //++ pbus mem map ++//
    // create pattern table #0
    nes->pattab0.type = MEM_ROM;
    nes->pattab0.size = 0x1000;
    mem_create(&(nes->pattab0));

    // create pattern table #1
    nes->pattab1.type = MEM_ROM;
    nes->pattab1.size = 0x1000;
    mem_create(&(nes->pattab1));

    // create vram0
    nes->vram0.type = MEM_RAM;
    nes->vram0.size = 0x0400;
    mem_create(&(nes->vram0));

    // create vram1
    nes->vram1.type = MEM_RAM;
    nes->vram1.size = 0x0400;
    mem_create(&(nes->vram1));

    // create vram2
    nes->vram2.type = MEM_RAM;
    nes->vram2.size = 0x0400;
    mem_create(&(nes->vram2));

    // create vram3
    nes->vram3.type = MEM_RAM;
    nes->vram3.size = 0x0400;
    mem_create(&(nes->vram3));

    // init nes cbus
    bus_setmap(nes->pbus, 0, 0x0000, 0x0FFF, &(nes->pattab0));
    bus_setmap(nes->pbus, 1, 0x1000, 0x1FFF, &(nes->pattab1));
    bus_setmap(nes->pbus, 2, 0x2000, 0x23FF, &(nes->vram0  ));
    bus_setmap(nes->pbus, 3, 0x2400, 0x27FF, &(nes->vram1  ));
    bus_setmap(nes->pbus, 4, 0x2800, 0x2BFF, &(nes->vram2  ));
    bus_setmap(nes->pbus, 5, 0x2C00, 0x2FFF, &(nes->vram3  ));
    //-- pbus mem map --//

    return FALSE;
}

void nes_destroy(NES *nes)
{
    mem_destroy(&(nes->cram   ));
    mem_destroy(&(nes->ppuregs));
    mem_destroy(&(nes->apuregs));
    mem_destroy(&(nes->erom   ));
    mem_destroy(&(nes->sram   ));
    mem_destroy(&(nes->prgrom0));
    mem_destroy(&(nes->prgrom1));

    mem_destroy(&(nes->pattab0));
    mem_destroy(&(nes->pattab1));
    mem_destroy(&(nes->vram0  ));
    mem_destroy(&(nes->vram1  ));
    mem_destroy(&(nes->vram2  ));
    mem_destroy(&(nes->vram3  ));
}

#else
#include <windows.h>
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPreInst, LPSTR lpszCmdLine, int nCmdShow)
{
    return 0;
}
#endif
