#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <libavdev/adev.h>
#include <libavdev/vdev.h>
#include <libavdev/idev.h>
#include "nes.h"

typedef struct {
    void *adev, *vdev, *idev;
    NES   nes;
} EMULATOR;

static void my_nes_cb(void *cbctx, int type, uint8_t *buf, int len)
{
    EMULATOR *emulator = cbctx;
    BMP      *bitmap   = NULL;
    static int key_map_pad0[][2] = {
        { 'E', NES_KEY_UP     }, { 'D', NES_KEY_DOWN  }, { 'S', NES_KEY_LEFT    }, { 'F', NES_KEY_RIGHT   },
        { 'J', NES_KEY_A      }, { 'K', NES_KEY_B     }, { 'U', NES_KEY_TURBO_A }, { 'I', NES_KEY_TURBO_B },
        { 'B', NES_KEY_SELECT }, { 'N', NES_KEY_START }, {  0 , 0 }
    };
    switch (type) {
    case 0:
        adev_play(emulator->adev, buf, len, 100);
        break;
    case 1:
        bitmap = vdev_lock(emulator->vdev);
        if (bitmap) memcpy(bitmap->pdata, buf, len);
        vdev_unlock(emulator->vdev);
        break;
    case 2:
        for (int i = 0; key_map_pad0[i][0]; i++) {
            if (idev_getkey(emulator->idev, key_map_pad0[i][0])) *((uint32_t*)buf) |= key_map_pad0[i][1];
        }
        break;
    }
}

int main(int argc, char *argv[])
{
    char     *rom      = "test.nes";
    int       samprate = APU_MIXER_SAMPRATE;
    EMULATOR *emulator = NULL;
    int       i;

    for (i = 1; i < argc; i++) {
        if (strstr(argv[i], "--rom="     ) == argv[i]) rom      = argv[i] + sizeof("--rom=") - 1;
        if (strstr(argv[i], "--samprate=") == argv[i]) samprate = atoi(argv[i] + sizeof("--samprate=") - 1);
    }

    emulator = calloc(1, sizeof(EMULATOR));
    if (!emulator) {
        printf("failed to calloc memory for emulator !\n");
        return -1;
    }
    emulator->adev = adev_init(samprate, 1, samprate / 60, 6);
    emulator->vdev = vdev_init(NES_WIDTH, NES_HEIGHT, NULL, NULL, NULL);
    emulator->idev = (void*)vdev_get(emulator->vdev, "idev", NULL);

    nes_init  (&emulator->nes, rom, my_nes_cb, emulator);
    nes_setrun(&emulator->nes, 1);

    while (strcmp((char*)vdev_get(emulator->vdev, "state", 0), "closed") != 0) usleep(100 * 1000);

    nes_free (&emulator->nes);
    vdev_exit(emulator->vdev, 0);
    adev_exit(emulator->adev);
    free(emulator);
    return 0;
}
