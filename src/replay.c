// 包含头文件
#include "replay.h"
#include "nes.h"

// 函数实现
void replay_init(REPLAY *rep)
{
    // init replay mode
    rep->mode   = NES_REPLAY_RECORD;
    rep->tmpnam = tmpnam(NULL);
    rep->fp     = fopen(rep->tmpnam, "wb+");
}

void replay_free(REPLAY *rep)
{
    if (rep->fp)
    {
        fclose(rep->fp);
        rep->fp = NULL;
    }
    unlink(rep->tmpnam);
}

void replay_reset(REPLAY *rep)
{
    // close old fp
    if (rep->fp) fclose(rep->fp);

    switch (rep->mode)
    {
    case NES_REPLAY_RECORD:
        rep->fp = fopen(rep->tmpnam, "wb+");
        break;
    case NES_REPLAY_PLAY:
        rep->fp = fopen(rep->tmpnam, "rb");
        fseek(rep->fp, 0, SEEK_END);
        rep->total = ftell(rep->fp);
        fseek(rep->fp, 0, SEEK_SET);
        break;
    }
}

BYTE replay_run(REPLAY *rep, BYTE data)
{
    int value = 0;
    switch (rep->mode)
    {
    case NES_REPLAY_RECORD:
        fputc(data, rep->fp);
        break;

    case NES_REPLAY_PLAY:
        value = fgetc(rep->fp);
        data  = (value == EOF) ? 0 : value;
        break;
    }
    return data;
}

int replay_progress(REPLAY *rep)
{
    // if record mode
    if (rep->mode == NES_REPLAY_RECORD) return 1;

    // if play mode
    rep->curpos = ftell(rep->fp);
    return (rep->curpos < rep->total);
}

