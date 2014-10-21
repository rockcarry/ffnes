// 包含头文件
#include "replay.h"
#include "lzw.h"

// 函数实现
void replay_init(REPLAY *rep, char *file, int mode)
{
    // save file path
    if (file) file = strcpy(rep->file, file);
    else rep->file[0] = '\0';

    // save replay mode
    rep->mode = mode;

    // for different mode
    switch (mode)
    {
    case NES_REPLAY_RECORD:
        rep->lzwfp = lzw_fopen(file, "wb");
        break;

    case NES_REPLAY_PLAY:
        rep->lzwfp = lzw_fopen(file, "rb");
        break;
    }
}

void replay_free(REPLAY *rep)
{
    if (rep->lzwfp)
    {
        lzw_fclose(rep->lzwfp);
        rep->lzwfp = NULL;
    }
}

void replay_reset(REPLAY *rep)
{
    replay_free(rep);
    replay_init(rep, rep->file, rep->mode);
}

BYTE replay_run(REPLAY *rep, BYTE data)
{
    int value = 0;
    switch (rep->mode)
    {
    case NES_REPLAY_RECORD:
        lzw_fputc(data, rep->lzwfp);
        break;

    case NES_REPLAY_PLAY:
        value = lzw_fgetc(rep->lzwfp);
        data  = (value == EOF) ? data : value;
        break;
    }
    return data;
}
