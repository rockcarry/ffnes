// 包含头文件
#include <stdio.h>
#include "cartridge.h"
#include "log.h"

// 常量定义
#define INES_HEADER_SIZE   16
#define INES_TRAINER_SIZE  512

// 内部函数实现
static void modify_file_ext(char *dst, char *src)
{
    size_t len = strlen(src);
    if (len > 4 && stricmp(src+len-4, ".nes") == 0) len -= 4;
    if (len > MAX_PATH                            ) len -= 4;

    strncpy(dst, src, MAX_PATH);
    dst[len++] = '.';
    dst[len++] = 's';
    dst[len++] = 'r';
    dst[len++] = 'm';
}

// 函数实现
BOOL cartridge_load(CARTRIDGE *pcart, char *file)
{
    FILE *fp = NULL;

    // open nes file
    strncpy(pcart->file, file, MAX_PATH);
    fp = fopen(file, "rb");
    if (!fp)
    {
        memset(pcart, 0, sizeof(CARTRIDGE));
        pcart->signature[0] = 'N';
        pcart->signature[1] = 'E';
        pcart->signature[2] = 'S';
        pcart->signature[3] = 0x1a;
        pcart->prom_count   = 1;
        pcart->crom_count   = 1;
        pcart->buf_prom     = malloc(0x4000);
        pcart->buf_crxm     = malloc(0x2000);
        if (pcart->buf_prom) memset(pcart->buf_prom, 0xea, 0x4000);
        if (pcart->buf_crxm) memset(pcart->buf_crxm, 0x00, 0x2000);
        return FALSE;
    }

    // read ines file header
    fread(pcart, INES_HEADER_SIZE, 1, fp);

    // read trainer if exists
    if (cartridge_has_trainer(pcart)) {
        pcart->buf_trainer = malloc(INES_TRAINER_SIZE);
        if (!pcart->buf_trainer) {
            log_printf("failed to allocate cartridge trainer buffer !\n");
            exit(0);
        }
        else fread(pcart->buf_trainer, INES_TRAINER_SIZE, 1, fp);
    }

    // allocate sram if exists
    if (cartridge_has_sram(pcart)) {
        pcart->buf_sram = malloc(0x2000);
        if (!pcart->buf_sram) {
            log_printf("failed to allocate cartridge sram buffer !\n");
            exit(0);
        }
        else {
            FILE *fp = NULL;
            char sram[MAX_PATH];
            modify_file_ext(sram, file);
            fp = fopen(sram, "rb");
            if (fp) {
                fread(pcart->buf_sram, 0x2000, 1, fp);
                fclose(fp);
            }
            else memset(pcart->buf_sram, 0, 0x2000);
        }
    }

    // read prom data
    if (pcart->prom_count > 0)
    {
        pcart->buf_prom = malloc(pcart->prom_count * 0x4000);
        if (!pcart->buf_prom) {
            log_printf("failed to allocate cartridge prom buffer !\n");
            exit(0);
        }
        else fread(pcart->buf_prom, pcart->prom_count * 0x4000, 1, fp);
    }

    // read crom data
    if (pcart->crom_count) pcart->ischrram = 0;
    else
    {
        pcart->ischrram   = 1;
        pcart->crom_count = 1;
    }

    // allocate buffer for crom/cram
    pcart->buf_crxm = malloc(pcart->crom_count * 0x2000);
    if (!pcart->buf_crxm) {
        log_printf("failed to allocate cartridge crom buffer !\n");
        exit(0);
    }

    // init buffer for crom/cram
    if (pcart->ischrram) memset(pcart->buf_crxm, 0, pcart->crom_count * 0x2000);
    else fread(pcart->buf_crxm, pcart->crom_count * 0x2000, 1, fp);

    // close fp
    fclose(fp);
    return TRUE;
}

BOOL cartridge_save(CARTRIDGE *pcart, char *file)
{
    FILE *fp = NULL;

    fp = fopen(file, "wb");
    if (!fp) return FALSE;

    // write ines file header
    fwrite(pcart, INES_HEADER_SIZE, 1, fp);

    // write trainer if exists
    if (cartridge_has_trainer(pcart)) {
        fwrite(pcart->buf_trainer, INES_TRAINER_SIZE, 1, fp);
    }

    // write prom & crom data
    fwrite(pcart->buf_prom, pcart->prom_count * 0x4000, 1, fp);
    fwrite(pcart->buf_crxm, pcart->crom_count * 0x2000, 1, fp);

    fclose(fp);
    return TRUE;
}

void cartridge_free(CARTRIDGE *pcart)
{
    if (cartridge_has_sram(pcart)) {
        FILE *fp = NULL;
        char sram[MAX_PATH];
        modify_file_ext(sram, pcart->file);
        fp = fopen(sram, "wb");
        if (fp) {
            fwrite(pcart->buf_sram, 0x2000, 1, fp);
            fclose(fp);
        }
    }

    if (pcart->buf_trainer) free(pcart->buf_trainer);
    if (pcart->buf_sram   ) free(pcart->buf_sram   );
    if (pcart->buf_prom   ) free(pcart->buf_prom   );
    if (pcart->buf_crxm   ) free(pcart->buf_crxm   );
}

BOOL cartridge_has_sram(CARTRIDGE *pcart)
{
    return (pcart->romctrl_byte1 & (1 << 1));
}

BOOL cartridge_has_trainer(CARTRIDGE *pcart)
{
    return (pcart->romctrl_byte1 & (1 << 2));
}

int* cartridge_get_vram_mirroring(CARTRIDGE *pcart)
{
    static int vram_mirroring_map[4][4] =
    {
        {0, 0, 1, 1},
        {0, 1, 0, 1},
        {0, 1, 2, 3},
        {0, 0, 0, 0},
    };
    int mirroring = ((pcart->romctrl_byte1 & (1 << 3)) >> 2) | (pcart->romctrl_byte1 & (1 << 0));
    return vram_mirroring_map[mirroring];
}

int  cartridge_get_mappercode(CARTRIDGE *pcart)
{
    return (((pcart->romctrl_byte1 & 0xf0) >> 4) | ((pcart->romctrl_byte2 & 0xf0) >> 0));
}

