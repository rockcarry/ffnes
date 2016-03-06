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
    D3DPRESENT_PARAMETERS d3dpp;

    int   d3d_mode_changed;
    RECT  save_window_rect;
    int   full_width;
    int   full_height;

    RECT  rtlast;
    RECT  rtview;

    char  textstr[256];
    int   textposx;
    int   textposy;
    DWORD texttick;
    int   priority;
} DEVD3DCTXT;

// 内部函数实现
static void create_device_surface(DEVD3DCTXT *ctxt)
{
    if (ctxt->d3dpp.Windowed)
    {
        ctxt->d3dpp.BackBufferWidth  = ctxt->width;
        ctxt->d3dpp.BackBufferHeight = ctxt->height;
        SetWindowLong(ctxt->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW|WS_VISIBLE);
        MoveWindow   (ctxt->hwnd, ctxt->save_window_rect.left, ctxt->save_window_rect.top,
            ctxt->save_window_rect.right - ctxt->save_window_rect.left,
            ctxt->save_window_rect.bottom - ctxt->save_window_rect.top, TRUE);
    }
    else
    {
        ctxt->d3dpp.BackBufferWidth  = ctxt->full_width;
        ctxt->d3dpp.BackBufferHeight = ctxt->full_height;
        SetWindowLong(ctxt->hwnd, GWL_STYLE, WS_POPUP|WS_VISIBLE);
        GetWindowRect(ctxt->hwnd, &ctxt->save_window_rect);
        MoveWindow   (ctxt->hwnd, 0, 0, ctxt->full_width, ctxt->full_height, TRUE);
    }

    if (FAILED(ctxt->pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, ctxt->hwnd,
                            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &ctxt->d3dpp, &ctxt->pD3DDev)))
    {
        log_printf("failed to create d3d device !\n");
        exit(0);
    }
    else {
        // clear direct3d device
        ctxt->pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);

        // create surface
        if (FAILED(ctxt->pD3DDev->CreateOffscreenPlainSurface(ctxt->width, ctxt->height,
                                    D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &ctxt->pSurface, NULL)))
        {
            log_printf("failed to create d3d off screen plain surface !\n");
            exit(0);
        }
        else
        {
            D3DLOCKED_RECT d3d_rect;
            ctxt->pSurface->LockRect(&d3d_rect, NULL, D3DLOCK_DISCARD);
            memset(d3d_rect.pBits, 0, d3d_rect.Pitch * ctxt->height);
            ctxt->pSurface->UnlockRect();
        }
    }
}

// 接口函数实现
static void* vdev_d3d_create(int w, int h, DWORD extra)
{
    DEVD3DCTXT *ctxt = (DEVD3DCTXT*)malloc(sizeof(DEVD3DCTXT));
    if (!ctxt) {
        log_printf("failed to allocate d3d vdev context !\n");
        exit(0);
    }

    // init d3d vdev context
    memset(ctxt, 0, sizeof(DEVD3DCTXT));
    ctxt->width  = w;
    ctxt->height = h;
    ctxt->hwnd   = (HWND)extra;

    // save current window rect
    GetWindowRect(ctxt->hwnd, &ctxt->save_window_rect);

    // create d3d
    ctxt->pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!ctxt->pD3D) {
        log_printf("failed to allocate d3d object !\n");
        exit(0);
    }

    //++ enum adapter modes
    D3DDISPLAYMODE d3dmode = {0};
    int            nummode = 0;
    int            curdistance = 0x7fffffff;
    int            mindistance = 0x7fffffff;
    nummode = ctxt->pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
    while (nummode--)
    {
        ctxt->pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, nummode, &d3dmode);
        curdistance = (640 - d3dmode.Width ) * (640 - d3dmode.Width )
            + (480 - d3dmode.Height) * (480 - d3dmode.Height);
        if (curdistance < mindistance)
        {
            ctxt->full_width  = d3dmode.Width;
            ctxt->full_height = d3dmode.Height;
            mindistance = curdistance;
        }
    }
    //-- enum adapter modes

    // fill d3dpp struct
    ctxt->pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3dmode);
    ctxt->d3dpp.BackBufferFormat      = D3DFMT_X8R8G8B8;
    ctxt->d3dpp.BackBufferCount       = 1;
    ctxt->d3dpp.MultiSampleType       = D3DMULTISAMPLE_NONE;
    ctxt->d3dpp.MultiSampleQuality    = 0;
    ctxt->d3dpp.SwapEffect            = D3DSWAPEFFECT_COPY;
    ctxt->d3dpp.hDeviceWindow         = ctxt->hwnd;
    ctxt->d3dpp.Windowed              = TRUE;
    ctxt->d3dpp.EnableAutoDepthStencil= FALSE;
    ctxt->d3dpp.PresentationInterval  = d3dmode.RefreshRate < 60 ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_ONE;

    // create d3d device and surface
    create_device_surface(ctxt);

    return ctxt;
}

static void vdev_d3d_destroy(void *ctxt)
{
    DEVD3DCTXT *c = (DEVD3DCTXT*)ctxt;
    c->pSurface->Release();
    c->pD3DDev->Release();
    c->pD3D->Release();
    free(c);
}

static void vdev_d3d_bufrequest(void *ctxt, void **buf, int *stride)
{
    DEVD3DCTXT *c = (DEVD3DCTXT*)ctxt;

    // lock texture rect
    D3DLOCKED_RECT d3d_rect;
    c->pSurface->LockRect(&d3d_rect, NULL, D3DLOCK_DISCARD);

    if (buf   ) *buf    = d3d_rect.pBits;
    if (stride) *stride = d3d_rect.Pitch / 4;
}

static void vdev_d3d_bufpost(void *ctxt)
{
    DEVD3DCTXT *c = (DEVD3DCTXT*)ctxt;
    RECT        rect;

    // unlock texture rect
    c->pSurface->UnlockRect();

    GetClientRect(c->hwnd, &rect);
    if (  c->rtlast.right  != rect.right
       || c->rtlast.bottom != rect.bottom)
    {
        memcpy(&c->rtlast, &rect, sizeof(RECT));
        memcpy(&c->rtview, &rect, sizeof(RECT));

        int x  = 0;
        int y  = 0;
        int sw, sh, dw, dh;

        sw = dw = rect.right;
        sh = dh = rect.bottom;

        //++ keep picture w/h ratio when stretching ++//
        if (256 * sh > 240 * sw)
        {
            dh = dw * 240 / 256;
            y  = (sh - dh) / 2;

            c->rtview.bottom = y;
            InvalidateRect(c->hwnd, &c->rtview, TRUE);
            c->rtview.top    = sh - y;
            c->rtview.bottom = sh;
            InvalidateRect(c->hwnd, &c->rtview, TRUE);
        }
        else
        {
            dw = dh * 256 / 240;
            x  = (sw - dw) / 2;

            c->rtview.right  = x;
            InvalidateRect(c->hwnd, &c->rtview, TRUE);
            c->rtview.left   = sw - x;
            c->rtview.right  = sw;
            InvalidateRect(c->hwnd, &c->rtview, TRUE);
        }
        //-- keep picture w/h ratio when stretching --//

        c->rtview.left   = x;
        c->rtview.top    = y;
        c->rtview.right  = x + dw;
        c->rtview.bottom = y + dh;
    }

    if (c->texttick > GetTickCount())
    {
        HDC hdc = NULL;
        c->pSurface->GetDC(&hdc);
        if (hdc)
        {
            SetBkMode   (hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255,255,255));
            TextOut(hdc, c->textposx, c->textposy, c->textstr, (int)strlen(c->textstr));
            c->pSurface->ReleaseDC(hdc);
        }
    }
    else c->priority = 0;

    IDirect3DSurface9 *pback = NULL;
    if (SUCCEEDED(c->pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pback)))
    {
        c->pD3DDev->StretchRect(c->pSurface, NULL, pback, NULL, D3DTEXF_LINEAR);
        pback->Release();

        if (SUCCEEDED(c->pD3DDev->Present(NULL, &c->rtview, NULL, NULL)))
        {
            Sleep(1);
        }
    }

    if (c->d3d_mode_changed)
    {
        //++ re-create d3d device and surface
        c->pSurface->Release();   // release
        c->pD3DDev->Release();    // release
        create_device_surface(c); // create
        //-- re-create d3d device and surface

        c->d3d_mode_changed = 0;
    }
}

static void vdev_d3d_textout(void *ctxt, int x, int y, char *text, int time, int priority)
{
    DEVD3DCTXT *c = (DEVD3DCTXT*)ctxt;
    if (priority >= c->priority)
    {
        strncpy(c->textstr, text, 256);
        c->textposx = x;
        c->textposy = y;
        c->texttick = (time >= 0) ? (GetTickCount() + time) : 0xffffffff;
        c->priority = priority;
    }
}

static void vdev_d3d_setfullsceen(void *ctxt, int full)
{
    DEVD3DCTXT *c = (DEVD3DCTXT*)ctxt;
    if (c->d3dpp.Windowed == !full) return;
    c->d3dpp.Windowed   = !full;
    c->d3d_mode_changed = 1;
}

static int vdev_d3d_getfullsceen(void *ctxt)
{
    return !((DEVD3DCTXT*)ctxt)->d3dpp.Windowed;
}

// 全局变量定义
VDEV DEV_D3D =
{
    vdev_d3d_create,
    vdev_d3d_destroy,
    vdev_d3d_bufrequest,
    vdev_d3d_bufpost,
    vdev_d3d_textout,
    vdev_d3d_setfullsceen,
    vdev_d3d_getfullsceen,
};
