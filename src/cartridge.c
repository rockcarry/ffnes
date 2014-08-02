// 包含头文件
#include <stdio.h>
#include "cartridge.h"

// 函数实现
BOOL cartridge_load(CARTRIDGE *pcart, char *file)
{
    FILE *fp   = NULL;
    BOOL  bret = FALSE;

    fp = fopen(file, "rb");
    if (!fp) return FALSE;

    // read ines file header
    fread(pcart, INES_HEADER_SIZE, 1, fp);

    // read trainer if exists
    if (cartridge_has_trainer(pcart)) {
        pcart->buf_trainer = malloc(INES_TRAINER_SIZE);
        if (!pcart->buf_trainer) goto done;
        fread(pcart->buf_trainer, INES_TRAINER_SIZE, 1, fp);
    }

    // allocate sram if exists
    if (cartridge_has_sram(pcart)) {
        pcart->buf_sram = malloc(8 * 1024);
        if (!pcart->buf_sram) goto done;
    }

    // read prom & crom
    if (pcart->prom_count > 0)
    {
        pcart->buf_prom = malloc(pcart->prom_count * 16 * 1024);
        if (!pcart->buf_prom) goto done;
        fread(pcart->buf_prom, pcart->prom_count * 16 * 1024, 1, fp);
    }
    if (pcart->crom_count > 0)
    {
        pcart->buf_crom = malloc(pcart->crom_count * 8  * 1024);
        if (!pcart->buf_crom) goto done;
        fread(pcart->buf_crom, pcart->crom_count * 8  * 1024, 1, fp);
    }
    bret = TRUE;

done:
    fclose(fp); // close fp
    if (!bret) cartridge_free(pcart);
    return bret;
}

BOOL cartridge_save(CARTRIDGE *pcart, char *file)
{
    FILE *fp   = NULL;
    BOOL  bret = FALSE;

    fp = fopen(file, "wb");
    if (!fp) return FALSE;

    // write ines file header
    fwrite(pcart, INES_HEADER_SIZE, 1, fp);

    // write trainer if exists
    if (cartridge_has_trainer(pcart)) {
        if (!pcart->buf_trainer) goto done;
        fwrite(pcart->buf_trainer, INES_TRAINER_SIZE, 1, fp);
    }

    if (!pcart->buf_prom || !pcart->buf_crom) goto done;
    fwrite(pcart->buf_prom, pcart->prom_count * 16 * 1024, 1, fp);
    fwrite(pcart->buf_crom, pcart->crom_count * 8  * 1024, 1, fp);
    bret = TRUE;

done:
    fclose(fp);
    return bret;
}

void cartridge_free(CARTRIDGE *pcart)
{
    if (pcart->buf_trainer) free(pcart->buf_trainer);
    if (pcart->buf_sram   ) free(pcart->buf_sram   );
    if (pcart->buf_prom   ) free(pcart->buf_prom   );
    if (pcart->buf_crom   ) free(pcart->buf_crom   );
}

BOOL cartridge_has_sram(CARTRIDGE *pcart)
{
    return (pcart->romctrl_byte1 & (1 << 1));
}

BOOL cartridge_has_trainer(CARTRIDGE *pcart)
{
    return (pcart->romctrl_byte1 & (1 << 2));
}

BOOL cartridge_has_4screen(CARTRIDGE *pcart)
{
    return (pcart->romctrl_byte1 & (1 << 3));
}

int cartridge_get_hvmirroring(CARTRIDGE *pcart)
{
    return (pcart->romctrl_byte1 & (1 << 0));
}

BYTE cartridge_get_mappercode(CARTRIDGE *pcart)
{
    return (((pcart->romctrl_byte1 & 0xf0) >> 4) | ((pcart->romctrl_byte2 & 0xf0) >> 0));
}

