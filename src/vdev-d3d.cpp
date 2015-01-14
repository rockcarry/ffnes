// 包含头文件
#include <d3d9.h>
extern "C" {
#include "vdev.h"
#include "log.h"
}

typedef struct
{
    int   width;      /* 宽度 */
    int   height;     /* 高度 */
    HWND  hwnd;       /* 窗口句柄 */

    LPDIRECT3D9        pD3D;
    LPDIRECT3DDEVICE9  pD3DDev;
    LPDIRECT3DSURFACE9 pSurface;

    RECT  rtcur;
    RECT  rtlast;
    RECT  rtview;

    char  textstr[256];
    int   textposx;
    int   textposy;
    DWORD texttick;
} VDEVD3D;

// 函数实现
void* vdev_d3d_create(int w, int h, DWORD extra)
{
    VDEVD3D *dev = (VDEVD3D*)malloc(sizeof(VDEVD3D));
    if (!dev) {
        log_printf("failed to allocate d3d vdev context !\n");
        exit(0);
    }

    // init d3d vdev context
    memset(dev, 0, sizeof(VDEVD3D));
    dev->width  = w;
    dev->height = h;
    dev->hwnd   = (HWND)extra;

    // create d3d
    dev->pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!dev->pD3D) {
        log_printf("failed to allocate d3d object !\n");
        exit(0);
    }

    D3DDISPLAYMODE d3ddm = {0};
    dev->pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

    D3DPRESENT_PARAMETERS d3dpp = {0};
    d3dpp.BackBufferWidth       = dev->width;
    d3dpp.BackBufferHeight      = dev->height;
    d3dpp.BackBufferFormat      = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount       = 1;
    d3dpp.MultiSampleType       = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality    = 0;
    d3dpp.SwapEffect            = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow         = dev->hwnd;
    d3dpp.Windowed              = TRUE;
    d3dpp.EnableAutoDepthStencil= FALSE;
    d3dpp.PresentationInterval  = d3ddm.RefreshRate < 60 ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_ONE;
    if (FAILED(dev->pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dev->hwnd,
                            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &dev->pD3DDev)))
    {
        log_printf("failed to create d3d device !\n");
        exit(0);
    }
    else {
        // clear direct3d device
        dev->pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);

        // create surface
        if (FAILED(dev->pD3DDev->CreateOffscreenPlainSurface(dev->width, dev->height,
                                    D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &dev->pSurface, NULL)))
        {
            log_printf("failed to create d3d off screen plain surface !\n");
            exit(0);
        }
    }

    return dev;
}

void vdev_d3d_destroy(void *ctxt)
{
    VDEVD3D *dev = (VDEVD3D*)ctxt;
    dev->pSurface->Release();
    dev->pD3DDev->Release();
    dev->pD3D->Release();
    free(dev);
}

void vdev_d3d_buf_request(void *ctxt, void **buf, int *stride)
{
    VDEVD3D *dev = (VDEVD3D*)ctxt;

    // lock texture rect
    D3DLOCKED_RECT d3d_rect;
    dev->pSurface->LockRect(&d3d_rect, NULL, D3DLOCK_DISCARD);

    if (buf   ) *buf    = d3d_rect.pBits;
    if (stride) *stride = d3d_rect.Pitch / 4;
}

void vdev_d3d_buf_post(void *ctxt)
{
    VDEVD3D *dev = (VDEVD3D*)ctxt;

    // unlock texture rect
    dev->pSurface->UnlockRect();

    GetClientRect(dev->hwnd, &dev->rtcur);
    if (  dev->rtlast.right  != dev->rtcur.right
       || dev->rtlast.bottom != dev->rtcur.bottom)
    {
        memcpy(&dev->rtlast, &dev->rtcur, sizeof(RECT));
        memcpy(&dev->rtview, &dev->rtcur, sizeof(RECT));

        int x  = 0;
        int y  = 0;
        int sw, sh, dw, dh;

        sw = dw = dev->rtcur.right;
        sh = dh = dev->rtcur.bottom;

        //++ keep picture w/h ratio when stretching ++//
        if (256 * sh > 240 * sw)
        {
            dh = dw * 240 / 256;
            y  = (sh - dh) / 2;

            dev->rtview.bottom = y;
            InvalidateRect(dev->hwnd, &dev->rtview, TRUE);
            dev->rtview.top    = sh - y;
            dev->rtview.bottom = sh;
            InvalidateRect(dev->hwnd, &dev->rtview, TRUE);
        }
        else
        {
            dw = dh * 256 / 240;
            x  = (sw - dw) / 2;

            dev->rtview.right  = x;
            InvalidateRect(dev->hwnd, &dev->rtview, TRUE);
            dev->rtview.left   = sw - x;
            dev->rtview.right  = sw;
            InvalidateRect(dev->hwnd, &dev->rtview, TRUE);
        }
        //-- keep picture w/h ratio when stretching --//

        dev->rtview.left   = x;
        dev->rtview.top    = y;
        dev->rtview.right  = x + dw;
        dev->rtview.bottom = y + dh;
    }

    IDirect3DSurface9 *pback = NULL;
    if (SUCCEEDED(dev->pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pback)))
    {
        if (dev->texttick > GetTickCount())
        {
            HDC hdc = NULL;
            dev->pSurface->GetDC(&hdc);
            if (hdc)
            {
                SetBkMode   (hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255,255,255));
                TextOut(hdc, dev->textposx, dev->textposy, dev->textstr, (int)strlen(dev->textstr));
                dev->pSurface->ReleaseDC(hdc);
            }
        }

        dev->pD3DDev->StretchRect(dev->pSurface, NULL, pback, NULL, D3DTEXF_LINEAR);
        dev->pD3DDev->Present(NULL, &dev->rtview, NULL, NULL);
        pback->Release();
    }
}

void vdev_d3d_textout(void *ctxt, int x, int y, char *text, int time)
{
    VDEVD3D *dev = (VDEVD3D*)ctxt;
    strncpy(dev->textstr, text, 256);
    dev->textposx = x;
    dev->textposy = y;
    dev->texttick = (time >= 0) ? (GetTickCount() + time) : 0xffffffff;
}

