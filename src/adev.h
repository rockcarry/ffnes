#ifndef _NES_ADEV_H_
#define _NES_ADEV_H_

// 包含头文件
#include "stdefine.h"

// 类型定义
typedef struct {
    BYTE  *lpdata;
    DWORD  buflen;
} AUDIOBUF;

// 函数声明
void* adev_create (int bufnum, int buflen);
void  adev_destroy(void *ctxt);
void  adev_buf_request(void *ctxt, AUDIOBUF **ppab);
void  adev_buf_post   (void *ctxt, AUDIOBUF  * pab);

#endif
