#ifndef _NES_VDEV_H_
#define _NES_VDEV_H_

// 包含头文件
#include "stdefine.h"

// 函数声明
void* vdev_gdi_create (int w, int h, DWORD extra);
void  vdev_gdi_destroy(void *ctxt);
void  vdev_gdi_lock   (void *ctxt, void **buf, int *stride);
void  vdev_gdi_unlock (void *ctxt);

void* vdev_d3d_create (int w, int h, DWORD extra);
void  vdev_d3d_destroy(void *ctxt);
void  vdev_d3d_lock   (void *ctxt, void **buf, int *stride);
void  vdev_d3d_unlock (void *ctxt);

#ifdef ENABLE_DIRECT3D
#define vdev_create     vdev_d3d_create
#define vdev_destroy    vdev_d3d_destroy
#define vdev_lock       vdev_d3d_lock
#define vdev_unlock     vdev_d3d_unlock
#else
#define vdev_create     vdev_gdi_create
#define vdev_destroy    vdev_gdi_destroy
#define vdev_lock       vdev_gdi_lock
#define vdev_unlock     vdev_gdi_unlock
#endif

#endif
