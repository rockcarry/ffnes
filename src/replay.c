// 包含头文件
#include "replay.h"

// 函数实现
void replay_init(REPLAY *rep, char *file, int mode)
{
    rep->mode = mode;
    switch (mode)
    {
    case NES_REPLAY_RECORD:
        rep->fp = fopen(file, "wb");
        break;

    case NES_REPLAY_PLAY:
        rep->fp = fopen(file, "rb");
        break;
    }
}

void replay_free(REPLAY *rep)
{
    if (rep->fp)
    {
        if (rep->mode == NES_REPLAY_RECORD)
        {
            if (rep->counter)
            {
                fputc(rep->counter, rep->fp);
                fputc(rep->value  , rep->fp);
            }
        }
        fclose(rep->fp);
        rep->fp = NULL;
    }
}

void replay_reset(REPLAY *rep)
{
    if (rep->fp)
    {
        fseek(rep->fp, 0, SEEK_SET);
        rep->counter = 0;
        rep->value   = 0;
    }
}

BYTE replay_run(REPLAY *rep, BYTE data)
{
    switch (rep->mode)
    {
    case NES_REPLAY_RECORD:
        if (rep->counter == 0)
        {
            rep->value   = data;
            rep->counter = 1;
        }
        else if (  rep->counter == 0xff
                || rep->value   != data )
        {
            fputc(rep->counter, rep->fp);
            fputc(rep->value  , rep->fp);
            rep->value   = data;
            rep->counter = 1;
        }
        else rep->counter++;
        break;

    case NES_REPLAY_PLAY:
        if (rep->counter == 0)
        {
            // endof file directly return data
            if (feof(rep->fp)) return data;

            rep->counter = fgetc(rep->fp);
            rep->value   = fgetc(rep->fp);
        }
        data = rep->value;
        rep->counter--;
        break;
    }

    return data;
}
