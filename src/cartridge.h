#ifndef _NES_CARTRIDGE_H_
#define _NES_CARTRIDGE_H_

#include <stdint.h>

#pragma pack(1)
typedef struct {
    char     signature[4];  // must be NES + 0x1A
    uint8_t  prom_count;    // 16K PRG-ROM page count
    uint8_t  crom_count;    // 8K  CHR-ROM page count
    uint8_t  romctrl_byte1; // ROM Control Byte #1
    uint8_t  romctrl_byte2; // ROM Control Byte #2
    uint8_t  reserved[8];
    uint8_t *buf_trainer;
    uint8_t *buf_sram;
    uint8_t *buf_prom;
    uint8_t *buf_crxm;
    char     file[256];     // nes rom file name
    int      ischrram;      // cartridge is using chr-rom or chr-ram
} CARTRIDGE;
#pragma pack()

int  cartridge_load(CARTRIDGE *pcart, char *file);
int  cartridge_save(CARTRIDGE *pcart, char *file);
void cartridge_free(CARTRIDGE *pcart);

int  cartridge_has_sram   (CARTRIDGE *pcart);
int  cartridge_has_trainer(CARTRIDGE *pcart);

int* cartridge_get_vram_mirroring(CARTRIDGE *pcart);
int  cartridge_get_mappercode    (CARTRIDGE *pcart);

#endif
