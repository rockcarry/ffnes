// 包含头文件
#include "adev.h"
#include "log.h"

// 内部类型定义
typedef struct
{
    HWAVEOUT hWaveOut;
    WAVEHDR *pWaveHdr;

    int      bufnum;
    int      buflen;
    int      head;
    int      tail;
    HANDLE   bufsem;
} ADEV_CONTEXT;

// 内部函数实现
static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)dwInstance;
    switch (uMsg)
    {
    case WOM_DONE:
        if (++c->head == c->bufnum) c->head = 0;
        ReleaseSemaphore(c->bufsem, 1, NULL);
        break;
    }
}

// 接口函数实现
static void* adev_waveout_create(int bufnum, int buflen)
{
    ADEV_CONTEXT *ctxt = NULL;
    WAVEFORMATEX  wfx  = {0};
    BYTE         *pwavbuf;
    int           i;

    // allocate adev context
    ctxt = malloc(sizeof(ADEV_CONTEXT));
    if (!ctxt) {
        log_printf("failed to allocate adev context !\n");
        exit(0);
    }

    ctxt->bufnum   = bufnum;
    ctxt->buflen   = buflen;
    ctxt->head     = 0;
    ctxt->tail     = 0;
    ctxt->pWaveHdr = (WAVEHDR*)malloc(bufnum * (sizeof(WAVEHDR) + buflen));
    ctxt->bufsem   = CreateSemaphore(NULL, bufnum, bufnum, NULL);
    if (!ctxt->pWaveHdr || !ctxt->bufsem) {
        log_printf("failed to allocate waveout buffer and waveout semaphore !\n");
        exit(0);
    }

    // init for audio
    wfx.cbSize          = sizeof(wfx);
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample  = 16;    // 16bit
    wfx.nSamplesPerSec  = 44100; // 44.1k
    wfx.nChannels       = 1;     // mono
    wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
    waveOutOpen(&(ctxt->hWaveOut), WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD)ctxt, CALLBACK_FUNCTION);

    // init wavebuf
    memset(ctxt->pWaveHdr, 0, bufnum * (sizeof(WAVEHDR) + buflen));
    pwavbuf = (BYTE*)(ctxt->pWaveHdr + bufnum);
    for (i=0; i<bufnum; i++) {
        ctxt->pWaveHdr[i].lpData         = (LPSTR)(pwavbuf + i * buflen);
        ctxt->pWaveHdr[i].dwBufferLength = buflen;
        waveOutPrepareHeader(ctxt->hWaveOut, &(ctxt->pWaveHdr[i]), sizeof(WAVEHDR));
    }

    return ctxt;
}

static void adev_waveout_destroy(void *ctxt)
{
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    int i;

    // unprepare wave header & close waveout device
    for (i=0; i<c->bufnum; i++) {
        waveOutUnprepareHeader(c->hWaveOut, &(c->pWaveHdr[i]), sizeof(WAVEHDR));
    }
    waveOutClose(c->hWaveOut);

    // close semaphore
    CloseHandle(c->bufsem);

    // free memory
    free(c->pWaveHdr);
    free(c);
}

static void adev_waveout_dequeue(void *ctxt, AUDIOBUF **ppab)
{
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    WaitForSingleObject(c->bufsem, -1);
    *ppab = (AUDIOBUF*)&(c->pWaveHdr[c->tail]);
}

static void adev_waveout_enqueue(void *ctxt)
{
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    waveOutWrite(c->hWaveOut, c->pWaveHdr + c->tail, sizeof(WAVEHDR));
    if (++c->tail == c->bufnum) c->tail = 0;
}

// 全局变量定义
ADEV DEV_WAVEOUT =
{
    adev_waveout_create,
    adev_waveout_destroy,
    adev_waveout_dequeue,
    adev_waveout_enqueue,
};


