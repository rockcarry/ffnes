#ifndef _NES_CARTRIDGE_H_
#define _NES_CARTRIDGE_H_

// 包含头文件
#include "stdefine.h"

// 常量定义
#define INES_HEADER_SIZE   16
#define INES_TRAINER_SIZE  512

// 类型定义
#pragma pack(1)
typedef struct {
    char  signature[4];  // must be NES + 0x1A
    BYTE  prom_count;    // 16K PRG-ROM page count
    BYTE  crom_count;    // 8K  CHR-ROM page count
    BYTE  romctrl_byte1; // ROM Control Byte #1
    BYTE  romctrl_byte2; // ROM Control Byte #2
    BYTE  reserved[8];
    BYTE *buf_trainer;
    BYTE *buf_sram;
    BYTE *buf_prom;
    BYTE *buf_crom;
    char  file[MAX_PATH];// nes rom file name
    int   ischrram;      // cartridge is using chr-rom or chr-ram
} CARTRIDGE;
#pragma pack()

// 函数声明
BOOL cartridge_load(CARTRIDGE *pcart, char *file);
BOOL cartridge_save(CARTRIDGE *pcart, char *file);
void cartridge_free(CARTRIDGE *pcart);

BOOL cartridge_has_sram   (CARTRIDGE *pcart);
BOOL cartridge_has_trainer(CARTRIDGE *pcart);

int* cartridge_get_vram_mirroring(CARTRIDGE *pcart);
int  cartridge_get_mappercode    (CARTRIDGE *pcart);

#endif


