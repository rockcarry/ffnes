// 包含头文件
#include <d3d9.h>
extern "C" {
#include "vdev.h"
}

typedef struct
{
    int   width;      /* 宽度 */
    int   height;     /* 高度 */
    HWND  hwnd;       /* 窗口句柄 */

    LPDIRECT3D9        pD3D;
    LPDIRECT3DDEVICE9  pD3DDev;
    LPDIRECT3DSURFACE9*pSurfaces;

    int     bufnum;
    int     curnum;
    int     head;
    int     tail;
    HANDLE  bufsem;
    HANDLE  hEvent;
    HANDLE  hThread;
    BOOL    bExit;
} VDEVD3D;

// 内部函数实现
static DWORD WINAPI VDEVThreadProc(LPVOID lpParam)
{
    VDEVD3D *dev = (VDEVD3D*)lpParam;

    while (!dev->bExit)
    {
        // wait for video rendering event
        WaitForSingleObject(dev->hEvent, -1);

        // render scene
        if (SUCCEEDED(dev->pD3DDev->BeginScene()))
        {
            IDirect3DSurface9 *pback = NULL;
            dev->pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pback);
            if (pback)
            {
                dev->pD3DDev->StretchRect(dev->pSurfaces[dev->head], NULL, pback, NULL, D3DTEXF_LINEAR);
                pback->Release();
            }

            dev->pD3DDev->EndScene();
            dev->pD3DDev->Present(NULL, NULL, NULL, NULL);

            // update head & curnum
            if (++dev->head == dev->bufnum) dev->head = 0; dev->curnum--;
            ReleaseSemaphore(dev->bufsem, 1, NULL);
        }
    }
    return 0;
}

// 函数实现
void* vdev_d3d_create(int bufnum, int w, int h, DWORD extra)
{
    VDEVD3D *dev = (VDEVD3D*)malloc(sizeof(VDEVD3D));
    if (!dev) return dev;

    // init d3d vdev context
    memset(dev, 0, sizeof(VDEVD3D));
    dev->width  = w;
    dev->height = h;
    dev->hwnd   = (HWND)extra;
    dev->bufnum = bufnum;

    // create d3d
    dev->pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!dev->pD3D) return dev;

    D3DPRESENT_PARAMETERS d3dpp = {0};
    d3dpp.BackBufferWidth       = dev->width;
    d3dpp.BackBufferHeight      = dev->height;
    d3dpp.BackBufferFormat      = D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount       = 1;
    d3dpp.MultiSampleType       = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality    = 0;
    d3dpp.SwapEffect            = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow         = dev->hwnd;
    d3dpp.Windowed              = TRUE;
    d3dpp.EnableAutoDepthStencil= FALSE;
    d3dpp.PresentationInterval  = D3DPRESENT_INTERVAL_DEFAULT; // D3DPRESENT_INTERVAL_IMMEDIATE
    if (SUCCEEDED(dev->pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dev->hwnd,
                  D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, &d3dpp, &(dev->pD3DDev))))
    {
        // clear direct3d device
        dev->pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);

        // create surface
        dev->pSurfaces = (LPDIRECT3DSURFACE9*)malloc(sizeof(LPDIRECT3DSURFACE9) * bufnum);
        for (int i=0; i<bufnum; i++)
        {
            dev->pD3DDev->CreateOffscreenPlainSurface(dev->width, dev->height, D3DFMT_X8R8G8B8,
                          D3DPOOL_DEFAULT, &(dev->pSurfaces[i]), NULL);
        }
    }

    dev->bufsem  = CreateSemaphore(NULL, bufnum, bufnum, NULL);
    dev->hEvent  = CreateEvent (NULL, FALSE, FALSE, "FFNES_VDEV_EVENT");
    dev->hThread = CreateThread(NULL, 0, VDEVThreadProc, (LPVOID)dev, 0, NULL);

    return dev;
}

void vdev_d3d_destroy(void *ctxt)
{
    VDEVD3D *dev = (VDEVD3D*)ctxt;

    // exit vdev rendering thread
    dev->bExit = TRUE; SetEvent(dev->hEvent);
    WaitForSingleObject(dev->hThread, -1);
    CloseHandle(dev->hThread);
    CloseHandle(dev->bufsem);

    if (dev->pSurfaces)
    {
        for (int i=0; i<dev->bufnum; i++)
            dev->pSurfaces[i]->Release();
        free(dev->pSurfaces);
    }
    if (dev->pD3DDev  ) dev->pD3DDev->Release();
    if (dev->pD3D     ) dev->pD3D->Release();
    free(dev);
}

void vdev_d3d_buf_request(void *ctxt, void **buf, int *stride)
{
    VDEVD3D *dev = (VDEVD3D*)ctxt;

    WaitForSingleObject(dev->bufsem, -1);

    // lock texture rect
    D3DLOCKED_RECT d3d_rect;
    dev->pSurfaces[dev->tail]->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);

    if (buf   ) *buf    = d3d_rect.pBits;
    if (stride) *stride = d3d_rect.Pitch / 4;
}

void vdev_d3d_buf_post(void *ctxt)
{
    VDEVD3D *dev = (VDEVD3D*)ctxt;

    // unlock texture rect
    dev->pSurfaces[dev->tail]->UnlockRect();

    if (++dev->tail == dev->bufnum) dev->tail = 0; dev->curnum++;
}

