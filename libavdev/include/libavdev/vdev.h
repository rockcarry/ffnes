#ifndef __LIBAVDEV_VDEV__
#define __LIBAVDEV_VDEV__

#include <stdint.h>

#ifndef DEFINE_TYPE_BITMAP
#define DEFINE_TYPE_BITMAP
typedef void (*PFN_SETPIXEL)(void *pb, int x, int y, int c);
typedef int  (*PFN_GETPIXEL)(void *pb, int x, int y);
typedef struct {
    int       width;
    int       height;
    int       stride;
    int       cdepth;
    uint8_t  *pdata;
    uint32_t *ppal;
    PFN_SETPIXEL setpixel;
    PFN_GETPIXEL getpixel;
} BMP;
#endif

enum {
    DEV_MSG_VDEV_CLOSE ,
    DEV_MSG_KEY_EVENT  ,
    DEV_MSG_MOUSE_EVENT,
};

typedef int (*PFN_VDEV_MSG_CB)(void *cbctx, int msg, uint32_t param1, uint32_t param2, uint32_t param3);

// params: "fullscreen" - use directdraw fullscreen mode, "inithidden" - init window but not show it
void* vdev_init  (int w, int h, char *params, PFN_VDEV_MSG_CB callback, void *cbctx);
void  vdev_exit  (void *ctx, int close);
BMP * vdev_lock  (void *ctx);
void  vdev_unlock(void *ctx);
void  vdev_set   (void *ctx, char *name, void *data);
long  vdev_get   (void *ctx, char *name, void *data);

#endif
