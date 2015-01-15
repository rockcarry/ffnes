// 包含头文件
#include "replay.h"
#include "nes.h"

// 函数实现
void replay_init(REPLAY *rep)
{
    // init replay mode
    rep->mode = NES_REPLAY_RECORD;
    rep->fp   = fopen("lastreplay.tmp", "wb+");
}

void replay_free(REPLAY *rep)
{
    if (rep->fp)
    {
        fclose(rep->fp);
        rep->fp = NULL;
    }
    unlink("lastreplay.tmp");
}

void replay_reset(REPLAY *rep)
{
    // close old fp
    if (rep->fp) fclose(rep->fp);

    switch (rep->mode)
    {
    case NES_REPLAY_RECORD:
        rep->fp = fopen("lastreplay.tmp", "wb+");
        break;
    case NES_REPLAY_PLAY:
        rep->fp = fopen("lastreplay.tmp", "rb");
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

