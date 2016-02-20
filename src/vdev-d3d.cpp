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

    RECT  rtcur;
    RECT  rtlast;
    RECT  rtview;

    char  textstr[256];
    int   textposx;
    int   textposy;
    DWORD texttick;
    int   priority;
} VDEV_CONTEXT;

// 内部函数实现
static void create_device_surface(VDEV_CONTEXT *ctxt)
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
    VDEV_CONTEXT *ctxt = (VDEV_CONTEXT*)malloc(sizeof(VDEV_CONTEXT));
    if (!ctxt) {
        log_printf("failed to allocate d3d vdev context !\n");
        exit(0);
    }

    // init d3d vdev context
    memset(ctxt, 0, sizeof(VDEV_CONTEXT));
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
    VDEV_CONTEXT *context = (VDEV_CONTEXT*)ctxt;
    context->pSurface->Release();
    context->pD3DDev->Release();
    context->pD3D->Release();
    free(context);
}

static void vdev_d3d_bufrequest(void *ctxt, void **buf, int *stride)
{
    VDEV_CONTEXT *context = (VDEV_CONTEXT*)ctxt;

    // lock texture rect
    D3DLOCKED_RECT d3d_rect;
    context->pSurface->LockRect(&d3d_rect, NULL, D3DLOCK_DISCARD);

    if (buf   ) *buf    = d3d_rect.pBits;
    if (stride) *stride = d3d_rect.Pitch / 4;
}

static void vdev_d3d_bufpost(void *ctxt)
{
    VDEV_CONTEXT *context = (VDEV_CONTEXT*)ctxt;

    // unlock texture rect
    context->pSurface->UnlockRect();

    GetClientRect(context->hwnd, &context->rtcur);
    if (  context->rtlast.right  != context->rtcur.right
       || context->rtlast.bottom != context->rtcur.bottom)
    {
        memcpy(&context->rtlast, &context->rtcur, sizeof(RECT));
        memcpy(&context->rtview, &context->rtcur, sizeof(RECT));

        int x  = 0;
        int y  = 0;
        int sw, sh, dw, dh;

        sw = dw = context->rtcur.right;
        sh = dh = context->rtcur.bottom;

        //++ keep picture w/h ratio when stretching ++//
        if (256 * sh > 240 * sw)
        {
            dh = dw * 240 / 256;
            y  = (sh - dh) / 2;

            context->rtview.bottom = y;
            InvalidateRect(context->hwnd, &context->rtview, TRUE);
            context->rtview.top    = sh - y;
            context->rtview.bottom = sh;
            InvalidateRect(context->hwnd, &context->rtview, TRUE);
        }
        else
        {
            dw = dh * 256 / 240;
            x  = (sw - dw) / 2;

            context->rtview.right  = x;
            InvalidateRect(context->hwnd, &context->rtview, TRUE);
            context->rtview.left   = sw - x;
            context->rtview.right  = sw;
            InvalidateRect(context->hwnd, &context->rtview, TRUE);
        }
        //-- keep picture w/h ratio when stretching --//

        context->rtview.left   = x;
        context->rtview.top    = y;
        context->rtview.right  = x + dw;
        context->rtview.bottom = y + dh;
    }

    if (context->texttick > GetTickCount())
    {
        HDC hdc = NULL;
        context->pSurface->GetDC(&hdc);
        if (hdc)
        {
            SetBkMode   (hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255,255,255));
            TextOut(hdc, context->textposx, context->textposy, context->textstr, (int)strlen(context->textstr));
            context->pSurface->ReleaseDC(hdc);
        }
    }
    else context->priority = 0;

    IDirect3DSurface9 *pback = NULL;
    if (SUCCEEDED(context->pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pback)))
    {
        context->pD3DDev->StretchRect(context->pSurface, NULL, pback, NULL, D3DTEXF_LINEAR);
        pback->Release();

        if (SUCCEEDED(context->pD3DDev->Present(NULL, &context->rtview, NULL, NULL)))
        {
            Sleep(1);
        }
    }

    if (context->d3d_mode_changed)
    {
        //++ re-create d3d device and surface
        context->pSurface->Release();   // release
        context->pD3DDev->Release();    // release
        create_device_surface(context); // create
        //-- re-create d3d device and surface

        context->d3d_mode_changed = 0;
    }
}

static void vdev_d3d_textout(void *ctxt, int x, int y, char *text, int time, int priority)
{
    VDEV_CONTEXT *context = (VDEV_CONTEXT*)ctxt;
    if (priority >= context->priority)
    {
        strncpy(context->textstr, text, 256);
        context->textposx = x;
        context->textposy = y;
        context->texttick = (time >= 0) ? (GetTickCount() + time) : 0xffffffff;
        context->priority = priority;
    }
}

static void vdev_d3d_setfullsceen(void *ctxt, int full)
{
    VDEV_CONTEXT *context = (VDEV_CONTEXT*)ctxt;
    if (context->d3dpp.Windowed == !full) return;
    context->d3dpp.Windowed   = !full;
    context->d3d_mode_changed = 1;
}

static int vdev_d3d_getfullsceen(void *ctxt)
{
    return !((VDEV_CONTEXT*)ctxt)->d3dpp.Windowed;
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
