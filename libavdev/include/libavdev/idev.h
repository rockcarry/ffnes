#ifndef __LIBAVDEV_IDEV__
#define __LIBAVDEV_IDEV__

#include <stdint.h>

typedef int (*PFN_IDEV_MSG_CB)(void *cbctx, int msg, uint32_t param1, uint32_t param2, uint32_t param3);

#ifndef DEFINE_TYPE_IDEV
#define DEFINE_TYPE_IDEV
typedef struct {
    uint32_t  key_bits[8];
    int32_t   mouse_x, mouse_y, mouse_btns;
    PFN_IDEV_MSG_CB callback;
    void           *cbctx;
} IDEV;
#endif

void* idev_init(char *params, PFN_IDEV_MSG_CB callback, void *cbctx);
void  idev_exit(void *ctx);
void  idev_set (void *ctx, char *name, void *data);
long  idev_get (void *ctx, char *name, void *data);

int   idev_getkey  (void *ctx, int key);
void  idev_getmouse(void *ctx, int *x, int *y, int *btns);

#endif
