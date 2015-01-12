#ifndef _NES_VDEV_H_
#define _NES_VDEV_H_

// 包含头文件
#include "stdefine.h"

// 预编译开关
#define USE_DIRECT3D_RENDERING  TRUE

// 函数声明
void* vdev_gdi_create     (int w, int h, DWORD extra);
void  vdev_gdi_destroy    (void *ctxt);
void  vdev_gdi_buf_request(void *ctxt, void **buf, int *stride);
void  vdev_gdi_buf_post   (void *ctxt);
void  vdev_gdi_outtext    (void *ctxt, int x, int y, char *text, int time);

void* vdev_d3d_create     (int w, int h, DWORD extra);
void  vdev_d3d_destroy    (void *ctxt);
void  vdev_d3d_buf_request(void *ctxt, void **buf, int *stride);
void  vdev_d3d_buf_post   (void *ctxt);
void  vdev_d3d_outtext    (void *ctxt, int x, int y, char *text, int time);

#if USE_DIRECT3D_RENDERING
#define vdev_create      vdev_d3d_create
#define vdev_destroy     vdev_d3d_destroy
#define vdev_buf_request vdev_d3d_buf_request
#define vdev_buf_post    vdev_d3d_buf_post
#define vdev_outtext     vdev_d3d_outtext
#else
#define vdev_create      vdev_gdi_create
#define vdev_destroy     vdev_gdi_destroy
#define vdev_buf_request vdev_gdi_buf_request
#define vdev_buf_post    vdev_gdi_buf_post
#define vdev_outtext     vdev_gdi_outtext
#endif

#endif
