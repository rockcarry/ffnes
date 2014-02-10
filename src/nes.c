#ifndef _TEST_

// 包含头文件
#include "nes.h"

// 函数实现
BOOL nes_init(NES *nes, char *file)
{
    // clear it
    memset(nes, 0, sizeof(NES));

    // load cartridge first
    if (!cartridge_load(&(nes->cart), file)) {
        return FALSE;
    }

    //++ cbus mem map ++//
    // create cpu ram
    nes->cram.type = MEM_RAM;
    nes->cram.size = NES_CRAM_SIZE;
    nes->cram.data = nes->buf_cram;

    // create ppu regs
    nes->ppuregs.type = MEM_REG;
    nes->ppuregs.size = NES_PPUREGS_SIZE;
    nes->ppuregs.data = nes->buf_ppuregs;
    nes->ppuregs.r_callback = DEF_PPU_REG_RBC;
    nes->ppuregs.w_callback = DEF_PPU_REG_WBC;

    // create ppu regs
    nes->apuregs.type = MEM_REG;
    nes->apuregs.size = NES_APUREGS_SIZE;
    nes->apuregs.data = nes->buf_apuregs;

    // create expansion rom
    nes->erom.type = MEM_ROM;
    nes->erom.size = NES_EROM_SIZE;
    nes->erom.data = nes->buf_erom;

    // create sram
    nes->sram.type = MEM_RAM;
    nes->sram.size = NES_SRAM_SIZE;
    nes->sram.data = nes->cart.buf_sram;

    // create PRG-ROM 0
    nes->prgrom0.type = MEM_ROM;
    nes->prgrom0.size = NES_PRGROM0_SIZE;
    nes->prgrom0.data = nes->cart.buf_prom;

    // create PRG-ROM 1
    nes->prgrom1.type = MEM_ROM;
    nes->prgrom1.size = NES_PRGROM1_SIZE;
    if (nes->cart.prom_count <= 1) {
        nes->prgrom1.data = nes->cart.buf_prom;
    }
    else nes->prgrom1.data = nes->cart.buf_prom + NES_PRGROM0_SIZE;

    // init nes cbus
    bus_setmem(nes->cbus, 0, 0x0000, 0x1FFF, &(nes->cram   ));
    bus_setmem(nes->cbus, 1, 0x2000, 0x3FFF, &(nes->ppuregs));
    bus_setmem(nes->cbus, 2, 0x4000, 0x4017, &(nes->apuregs));
    bus_setmem(nes->cbus, 3, 0x4018, 0x5FFF, &(nes->erom   ));
    bus_setmem(nes->cbus, 4, 0x6000, 0x7FFF, &(nes->sram   ));
    bus_setmem(nes->cbus, 5, 0x8000, 0xBFFF, &(nes->prgrom0));
    bus_setmem(nes->cbus, 6, 0xC000, 0xFFFF, &(nes->prgrom1));
    bus_setmem(nes->cbus, 7, 0x0000, 0x0000, NULL           );
    //-- cbus mem map --//

    //++ pbus mem map ++//
    // create pattern table #0
    nes->pattab0.type = MEM_ROM;
    nes->pattab0.size = NES_PATTAB0_SIZE;
    nes->pattab0.data = nes->cart.buf_crom;

    // create pattern table #1
    nes->pattab1.type = MEM_ROM;
    nes->pattab1.size = NES_PATTAB1_SIZE;
    if (nes->cart.crom_count <= 1) {
        nes->pattab1.data = nes->cart.buf_crom;
    }
    else nes->pattab1.data = nes->cart.buf_crom + NES_PATTAB0_SIZE;

    // create vram0
    nes->vram0.type = MEM_RAM;
    nes->vram0.size = NES_VRAM0_SIZE;
    nes->vram0.data = nes->buf_vram0;

    // create vram1
    nes->vram1.type = MEM_RAM;
    nes->vram1.size = NES_VRAM1_SIZE;
    nes->vram1.data = nes->buf_vram1;

    // create vram2
    nes->vram2.type = MEM_RAM;
    nes->vram2.size = NES_VRAM2_SIZE;
    nes->vram2.data = nes->buf_vram2;

    // create vram3
    nes->vram3.type = MEM_RAM;
    nes->vram3.size = NES_VRAM2_SIZE;
    nes->vram3.data = nes->buf_vram3;

    // create color palette
    nes->palette.type = MEM_RAM;
    nes->palette.size = NES_PALETTE_SIZE;
    nes->palette.data = nes->buf_palette;

    // init nes cbus
    bus_setmir(nes->pbus, 0, 0x4000, 0xFFFF, 0x3FFF);
    bus_setmir(nes->pbus, 1, 0x3000, 0x3EFF, 0x2EFF);
    bus_setmir(nes->pbus, 2, 0x3F00, 0x3FFF, 0x3F1F);

    bus_setmem(nes->pbus, 3, 0x0000, 0x0FFF, &(nes->pattab0));
    bus_setmem(nes->pbus, 4, 0x1000, 0x1FFF, &(nes->pattab1));
    bus_setmem(nes->pbus, 5, 0x2000, 0x23FF, &(nes->vram0  ));
    bus_setmem(nes->pbus, 6, 0x2400, 0x27FF, &(nes->vram1  ));
    bus_setmem(nes->pbus, 7, 0x2800, 0x2BFF, &(nes->vram2  ));
    bus_setmem(nes->pbus, 8, 0x2C00, 0x2FFF, &(nes->vram3  ));

    // palette
    // 0x3F00 - 0x3F0F: image palette
    // 0x3F10 - 0x3F1F: sprite palette
    // 0x3F00, 0x3F04, 0x3F08, 0x3F0C, 0x3F10, 0x3F14, 0x3F18, 0x3F1C mirring and store the background color
    bus_setmem(nes->pbus, 9, 0x3F00, 0x3F1F, &(nes->palette));
    //-- pbus mem map --//
}

void nes_free(NES *nes)
{
    // free cartridge
    cartridge_free(&(nes->cart));
}

#else
#include <windows.h>
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPreInst, LPSTR lpszCmdLine, int nCmdShow)
{
    return 0;
}
#endif
