#ifndef __LIVAVDEV_ADEV__
#define __LIVAVDEV_ADEV__

enum {
    ADEV_CMD_DATA_PLAY,
    ADEV_CMD_DATA_RECORD,
};
typedef void (*PFN_ADEV_CALLBACK)(void *ctxt, int cmd, void *buf, int len);

void* adev_init(int out_samprate, int out_chnum, int out_frmsize, int out_frmnum);
void  adev_exit(void *ctx);
int   adev_play(void *ctx, void *buf, int len, int waitms);
void  adev_set (void *ctx, char *name, void *data); // name: "pause", "resume", "reset", "callback", "cbctx"
long  adev_get (void *ctx, char *name, void *data);

int adev_record(void *ctx, int start, int rec_samprate, int rec_chnum, int rec_frmsize, int rec_frmnum);

#endif
