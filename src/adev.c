// 包含头文件
#include "adev.h"

// 内部类型定义
typedef struct
{
    HWAVEOUT hWaveOut;
    WAVEHDR *pWaveHdr;
    HANDLE   bufsem;
    int      bufnum;
    int      buflen;
    long     head;
    long     tail;
} ADEV;

// 内部函数实现
static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    ADEV *dev = (ADEV*)dwInstance;
    switch (uMsg)
    {
    case WOM_DONE:
        if (++dev->head == dev->bufnum) dev->head = 0;
        ReleaseSemaphore(dev->bufsem, 1, NULL);
        break;
    }
}

// 函数实现
void* adev_create(int bufnum, int buflen)
{
    ADEV        *dev = NULL;
    WAVEFORMATEX wfx = {0};
    BYTE        *pwavbuf;
    int          i;

    // allocate adev context
    dev = malloc(sizeof(ADEV));
    if (!dev) return NULL;

    dev->bufnum   = bufnum;
    dev->buflen   = buflen;
    dev->head     = 0;
    dev->tail     = 0;
    dev->pWaveHdr = (WAVEHDR*)malloc(bufnum * (sizeof(WAVEHDR) + buflen));
    dev->bufsem   = CreateSemaphore(NULL, bufnum, bufnum, NULL);

    // init for audio
    wfx.cbSize          = sizeof(wfx);
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample  = 16;    // 16bit
    wfx.nSamplesPerSec  = 44100; // 44.1k
//  wfx.nSamplesPerSec  = 48000; // 48.0k
    wfx.nChannels       = 2;     // stereo
    wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
    waveOutOpen(&(dev->hWaveOut), WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD)dev, CALLBACK_FUNCTION);

    // init wavebuf
    memset(dev->pWaveHdr, 0, bufnum * (sizeof(WAVEHDR) + buflen));
    pwavbuf = (BYTE*)(dev->pWaveHdr + bufnum);
    for (i=0; i<bufnum; i++) {
        dev->pWaveHdr[i].lpData         = (LPSTR)(pwavbuf + i * buflen);
        dev->pWaveHdr[i].dwBufferLength = buflen;
        waveOutPrepareHeader(dev->hWaveOut, &(dev->pWaveHdr[i]), sizeof(WAVEHDR));
    }

    return dev;
}

void adev_destroy(void *ctxt)
{
    int i;
    ADEV *dev = (ADEV*)ctxt;
    if (dev == NULL) return;

    // unprepare
    for (i=0; i<dev->bufnum; i++) {
        waveOutUnprepareHeader(dev->hWaveOut, &(dev->pWaveHdr[i]), sizeof(WAVEHDR));
    }

    if (dev->hWaveOut) waveOutClose(dev->hWaveOut);
    if (dev->pWaveHdr) free(dev->pWaveHdr);
    if (dev->bufsem  ) CloseHandle(dev->bufsem);
    free(dev);
}

void adev_audio_buf_request(void *ctxt, AUDIOBUF **ppab)
{
    ADEV *dev = (ADEV*)ctxt;
    if (dev == NULL) return;

    WaitForSingleObject(dev->bufsem, -1);
    *ppab = (AUDIOBUF*)&(dev->pWaveHdr[dev->tail]);
}

void adev_audio_buf_post(void *ctxt, AUDIOBUF *pab)
{
    ADEV *dev = (ADEV*)ctxt;
    if (dev == NULL) return;

    waveOutWrite(dev->hWaveOut, (LPWAVEHDR)pab, sizeof(WAVEHDR));
    if (++dev->tail == dev->bufnum) dev->tail = 0;
}

