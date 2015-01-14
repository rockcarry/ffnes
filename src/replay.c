// 包含头文件
#include "replay.h"
#include "nes.h"
#include "lzw.h"

// 函数实现
void replay_init(REPLAY *rep)
{
    // init replay mode
    strncpy(rep->file, "lastreplay.rpl", MAX_PATH);
    rep->mode  = NES_REPLAY_RECORD;
    rep->lzwfp = lzw_fopen(rep->file, "wb");
}

void replay_free(REPLAY *rep)
{
    if (rep->lzwfp)
    {
        lzw_fclose(rep->lzwfp);
        rep->lzwfp = NULL;
    }
    unlink("lastreplay.rpl");
}

void replay_reset(REPLAY *rep)
{
    // close old lzwfp
    if (rep->lzwfp) lzw_fclose(rep->lzwfp);

    switch (rep->mode)
    {
    case NES_REPLAY_RECORD:
        rep->lzwfp = lzw_fopen(rep->file, "wb");
        break;
    case NES_REPLAY_PLAY:
        rep->lzwfp = lzw_fopen(rep->file, "rb");
        break;
    }
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
        data  = (value == EOF) ? 0 : value;
        break;
    }
    return data;
}

void replay_save(REPLAY *rep, char *file)
{
    NES  *nes = container_of(rep, NES, replay);
    void *fpsrc, *fpdst;
    int   running;
    int   value;

    if (  rep->mode != NES_REPLAY_RECORD
       || strcmp(rep->file, file) == 0)
    {
        return;
    }

    // pause nes thread if running
    running = nes_getrun(nes);
    if (running) nes_setrun(nes, 0);

    // close old lzwfp
    lzw_fclose(rep->lzwfp);

    // open file
    fpsrc = lzw_fopen(rep->file, "rb");
    fpdst = lzw_fopen(file     , "wb");

    // copy file
    while (fpsrc && fpdst)
    {
        value = lzw_fgetc(fpsrc);
        if (value == EOF) break;
        else lzw_fputc(value, fpdst);
    }

    // close file
    lzw_fclose(fpsrc);

    // for new lzwfp
    strncpy(rep->file, file, MAX_PATH);
    rep->lzwfp = fpdst;

    // resume running
    if (running) nes_setrun(nes, 1);
}

void replay_load(REPLAY *rep, char *file)
{
    NES *nes = container_of(rep, NES, replay);

    // open relay file and play
    rep->mode = NES_REPLAY_PLAY;
    strncpy(rep->file, file, MAX_PATH);

    // reset nes
    nes_reset(nes);

    // show text
    nes_textout(nes, 0, 222, "replay", -1, 2);
}

