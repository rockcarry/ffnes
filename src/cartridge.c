#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cartridge.h"

#define INES_HEADER_SIZE   16
#define INES_TRAINER_SIZE  512

static void modify_file_ext(char *dst, int size, char *src)
{
    snprintf(dst, size, "%s", src);
    int  len = strlen(dst);
    char *p  = dst + len - 4;
    if (p > dst && stricmp(p, ".nes") == 0) *p = '\0';
    else if (len + 4 > size - 1) dst[size - 5] = '\0';
    strcat(dst, ".srm");
}

int cartridge_load(CARTRIDGE *pcart, char *file)
{
    memset(pcart, 0, sizeof(CARTRIDGE));

    // open nes file
    strncpy(pcart->file, file, sizeof(pcart->file) - 1);
    FILE *fp = fopen(file, "rb");
    if (!fp) goto _failed;

    // read ines file header
    fread(pcart, INES_HEADER_SIZE, 1, fp);

    // read trainer if exists
    if (cartridge_has_trainer(pcart)) {
        pcart->buf_trainer = malloc(INES_TRAINER_SIZE);
        if (!pcart->buf_trainer) {
            printf("failed to allocate cartridge trainer buffer !\n");
            goto _failed;
        }
        fread(pcart->buf_trainer, INES_TRAINER_SIZE, 1, fp);
    }

    // allocate sram if exists
    if (cartridge_has_sram(pcart)) {
        pcart->buf_sram = calloc(1, 0x2000);
        if (!pcart->buf_sram) {
            printf("failed to allocate cartridge sram buffer !\n");
            goto _failed;
        }

        char sram[sizeof(pcart->file)];
        modify_file_ext(sram, sizeof(sram), file);
        fp = fopen(sram, "rb");
        if (fp) {
            fread(pcart->buf_sram, 0x2000, 1, fp);
            fclose(fp);
        }
    }

    // read prom data
    if (pcart->prom_count > 0) {
        pcart->buf_prom = malloc(pcart->prom_count * 0x4000);
        if (!pcart->buf_prom) {
            printf("failed to allocate cartridge prom buffer !\n");
            goto _failed;
        }
        fread(pcart->buf_prom, pcart->prom_count * 0x4000, 1, fp);
    }

    // read crom data
    if (pcart->crom_count) pcart->ischrram = 0;
    else pcart->ischrram = pcart->crom_count = 1;

    // allocate buffer for crom/cram
    pcart->buf_crxm = malloc(pcart->crom_count * 0x2000);
    if (!pcart->buf_crxm) {
        printf("failed to allocate cartridge crom buffer !\n");
        goto _failed;
    }

    // init buffer for crom/cram
    if (pcart->ischrram) memset(pcart->buf_crxm, 0, pcart->crom_count * 0x2000);
    else fread(pcart->buf_crxm, pcart->crom_count * 0x2000, 1, fp);

    // close fp
    fclose(fp);
    return 0;

_failed:
    free(pcart->buf_trainer); pcart->buf_trainer = NULL;
    free(pcart->buf_sram   ); pcart->buf_sram    = NULL;
    free(pcart->buf_prom   ); pcart->buf_prom    = NULL;
    free(pcart->buf_crxm   ); pcart->buf_crxm    = NULL;
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
    return -1;
}

int cartridge_save(CARTRIDGE *pcart, char *file)
{
    FILE *fp = fopen(file, "wb");
    if (!fp) return -1;

    // write ines file header
    fwrite(pcart, INES_HEADER_SIZE, 1, fp);

    // write trainer if exists
    if (cartridge_has_trainer(pcart)) fwrite(pcart->buf_trainer, INES_TRAINER_SIZE, 1, fp);

    // write prom & crom data
    fwrite(pcart->buf_prom, pcart->prom_count * 0x4000, 1, fp);
    fwrite(pcart->buf_crxm, pcart->crom_count * 0x2000, 1, fp);

    fclose(fp);
    return 0;
}

void cartridge_free(CARTRIDGE *pcart)
{
    if (cartridge_has_sram(pcart)) {
        char sram[sizeof(pcart->file)];
        modify_file_ext(sram, sizeof(sram), pcart->file);
        FILE *fp = fopen(sram, "wb");
        if (fp) {
            fwrite(pcart->buf_sram, 0x2000, 1, fp);
            fclose(fp);
        }
    }

    if (pcart->buf_trainer) free(pcart->buf_trainer);
    if (pcart->buf_sram   ) free(pcart->buf_sram   );
    if (pcart->buf_prom   ) free(pcart->buf_prom   );
    if (pcart->buf_crxm   ) free(pcart->buf_crxm   );
    memset(pcart, 0, sizeof(CARTRIDGE));
}

int cartridge_has_sram(CARTRIDGE *pcart)
{
    return (pcart->romctrl_byte1 & (1 << 1));
}

int cartridge_has_trainer(CARTRIDGE *pcart)
{
    return (pcart->romctrl_byte1 & (1 << 2));
}

int* cartridge_get_vram_mirroring(CARTRIDGE *pcart)
{
    static int vram_mirroring_map[4][4] = {
        {0, 0, 1, 1},
        {0, 1, 0, 1},
        {0, 1, 2, 3},
        {0, 0, 0, 0},
    };
    int mirroring = ((pcart->romctrl_byte1 & (1 << 3)) >> 2) | (pcart->romctrl_byte1 & (1 << 0));
    return vram_mirroring_map[mirroring];
}

int cartridge_get_mappercode(CARTRIDGE *pcart)
{
    return (((pcart->romctrl_byte1 & 0xf0) >> 4) | ((pcart->romctrl_byte2 & 0xf0) >> 0));
}

