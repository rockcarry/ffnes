// 包含头文件
#include "apu.h"
#include "adev.h"

// 内部常量定义
#define APU_ABUF_NUM  16
#define APU_ABUF_LEN  3200

// 函数实现
void apu_init(APU *apu, DWORD extra)
{
    apu->adevctxt = adev_create(APU_ABUF_NUM, APU_ABUF_LEN);
}

void apu_free(APU *apu)
{
    adev_destroy(apu->adevctxt);
}

void apu_reset(APU *apu)
{
}

void apu_render_frame(APU *apu)
{
    AUDIOBUF *paudiobuf = NULL;
    adev_audio_buf_request(apu->adevctxt, &paudiobuf);
    adev_audio_buf_post   (apu->adevctxt,  paudiobuf);
}
