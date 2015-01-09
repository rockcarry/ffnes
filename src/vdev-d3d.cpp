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
    LPDIRECT3DSURFACE9 pSurface;
} VDEVD3D;

// 函数实现
void* vdev_d3d_create(int w, int h, DWORD extra)
{
    VDEVD3D *dev = (VDEVD3D*)malloc(sizeof(VDEVD3D));
    if (!dev) return dev;

    // init d3d vdev context
    memset(dev, 0, sizeof(VDEVD3D));
    dev->width  = w;
    dev->height = h;
    dev->hwnd   = (HWND)extra;

    // create d3d
    dev->pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!dev->pD3D) return dev;

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
    if (SUCCEEDED(dev->pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dev->hwnd,
                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &(dev->pD3DDev))))
    {
        // clear direct3d device
        dev->pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);

        // create surface
        dev->pD3DDev->CreateOffscreenPlainSurface(dev->width, dev->height, D3DFMT_X8R8G8B8,
                        D3DPOOL_DEFAULT, &(dev->pSurface), NULL);
    }

    return dev;
}

void vdev_d3d_destroy(void *ctxt)
{
    VDEVD3D *dev = (VDEVD3D*)ctxt;
    if (dev->pSurface) dev->pSurface->Release();
    if (dev->pD3DDev ) dev->pD3DDev->Release();
    if (dev->pD3D    ) dev->pD3D->Release();
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
    RECT    rect = {0};
    int     x    = 0;
    int     y    = 0;
    int     sw, sh, dw, dh;

    // unlock texture rect
    dev->pSurface->UnlockRect();

    GetClientRect(dev->hwnd, &rect);
    sw = dw = rect.right;
    sh = dh = rect.bottom;

    //++ keep picture w/h ratio when stretching ++//
    if (256 * sh > 240 * sw)
    {
        dh = dw * 240 / 256;
        y  = (sh - dh) / 2;

        rect.bottom = y;
        InvalidateRect(dev->hwnd, &rect, TRUE);
        rect.top    = sh - y;
        rect.bottom = sh;
        InvalidateRect(dev->hwnd, &rect, TRUE);
    }
    else
    {
        dw = dh * 256 / 240;
        x  = (sw - dw) / 2;

        rect.right  = x;
        InvalidateRect(dev->hwnd, &rect, TRUE);
        rect.left   = sw - x;
        rect.right  = sw;
        InvalidateRect(dev->hwnd, &rect, TRUE);
    }
    //-- keep picture w/h ratio when stretching --//

    rect.left   = x;
    rect.top    = y;
    rect.right  = x + dw;
    rect.bottom = y + dh;

    IDirect3DSurface9 *pback = NULL;
    dev->pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pback);
    if (pback)
    {
        dev->pD3DDev->StretchRect(dev->pSurface, NULL, pback, NULL, D3DTEXF_LINEAR);
        pback->Release();
    }
    dev->pD3DDev->Present(NULL, &rect, NULL, NULL);
}

