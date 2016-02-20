// 包含头文件
extern "C" {
#include "vdev.h"
#include "log.h"
}

typedef struct
{
    int     width;      /* 宽度 */
    int     height;     /* 高度 */
    HWND    hwnd;       /* 窗口句柄 */
    HDC     hdcsrc;
    HDC     hdcdst;
    HBITMAP hbmp;
    void   *pbuf;
    int     stride;

    RECT    rtcur;
    RECT    rtlast;
    RECT    rtview;
    int     full;

    char    textstr[256];
    int     textposx;
    int     textposy;
    DWORD   texttick;
    int     priority;
} VDEV_CONTEXT;

// 接口函数实现
static void* vdev_gdi_create(int w, int h, DWORD extra)
{
    VDEV_CONTEXT *ctxt = (VDEV_CONTEXT*)malloc(sizeof(VDEV_CONTEXT));
    if (!ctxt) {
        log_printf("failed to allocate gdi vdev context !\n");
        exit(0);
    }

    // init vdev context
    memset(ctxt, 0, sizeof(VDEV_CONTEXT));
    ctxt->width  = w;
    ctxt->height = h;
    ctxt->hwnd   = (HWND)extra;
    ctxt->hdcdst = GetDC(ctxt->hwnd);
    ctxt->hdcsrc = CreateCompatibleDC(ctxt->hdcdst);
    if (!ctxt->hdcdst || !ctxt->hdcsrc) {
        log_printf("failed to get dc or create compatible dc !\n");
        exit(0);
    }

    //++ create dibsection, get bitmap buffer address & stride ++//
    BITMAPINFO bmpinfo;
    memset(&bmpinfo, 0, sizeof(bmpinfo));
    bmpinfo.bmiHeader.biSize        =  sizeof(BITMAPINFOHEADER);
    bmpinfo.bmiHeader.biWidth       =  w;
    bmpinfo.bmiHeader.biHeight      = -h;
    bmpinfo.bmiHeader.biPlanes      =  1;
    bmpinfo.bmiHeader.biBitCount    =  32;
    bmpinfo.bmiHeader.biCompression =  BI_RGB;

    ctxt->hbmp = CreateDIBSection(ctxt->hdcsrc, &bmpinfo, DIB_RGB_COLORS, &ctxt->pbuf, NULL, 0);
    if (!ctxt->hbmp) {
        log_printf("failed to create gdi dib section !\n");
        exit(0);
    }
    else SelectObject(ctxt->hdcsrc, ctxt->hbmp);

    BITMAP bitmap;
    GetObject(ctxt->hbmp, sizeof(BITMAP), &bitmap);
    ctxt->stride = bitmap.bmWidthBytes * 8 / 32;
    //-- create dibsection, get bitmap buffer address & stride --//

    return ctxt;
}

static void vdev_gdi_destroy(void *ctxt)
{
    VDEV_CONTEXT *context = (VDEV_CONTEXT*)ctxt;
    DeleteDC    (context->hdcsrc);
    ReleaseDC   (context->hwnd, context->hdcdst);
    DeleteObject(context->hbmp);
    free(context); // free vdev context
}

static void vdev_gdi_bufrequest(void *ctxt, void **buf, int *stride)
{
    VDEV_CONTEXT *context = (VDEV_CONTEXT*)ctxt;
    if (buf   ) *buf    = context->pbuf;
    if (stride) *stride = context->stride;
}

static void vdev_gdi_bufpost(void *ctxt)
{
    VDEV_CONTEXT *context = (VDEV_CONTEXT*)ctxt;

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

            context->rtview.right = x;
            InvalidateRect(context->hwnd, &context->rtview, TRUE);
            context->rtview.left  = sw - x;
            context->rtview.right = sw;
            InvalidateRect(context->hwnd, &context->rtview, TRUE);
        }
        //-- keep picture w/h ratio when stretching --//

        context->rtview.left   = x;
        context->rtview.top    = y;
        context->rtview.right  = dw;
        context->rtview.bottom = dh;
    }

    if (context->texttick > GetTickCount())
    {
        SetBkMode   (context->hdcsrc, TRANSPARENT);
        SetTextColor(context->hdcsrc, RGB(255,255,255));
        TextOut(context->hdcsrc, context->textposx, context->textposy, context->textstr, (int)strlen(context->textstr));
    }
    else context->priority = 0;

    // bitblt picture to window witch stretching
    StretchBlt(context->hdcdst, context->rtview.left, context->rtview.top, context->rtview.right, context->rtview.bottom,
               context->hdcsrc, 0, 0, context->width, context->height, SRCCOPY);

    Sleep(1); // sleep is used to make frame pitch more uniform
}

static void vdev_gdi_textout(void *ctxt, int x, int y, char *text, int time, int priority)
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

// gdi vdev can't support fullscreen mode
static void vdev_gdi_setfullsceen(void *ctxt, int full) { ((VDEV_CONTEXT*)ctxt)->full = full; }
static int  vdev_gdi_getfullsceen(void *ctxt)           { return ((VDEV_CONTEXT*)ctxt)->full; }

// 全局变量定义
VDEV DEV_GDI =
{
    vdev_gdi_create,
    vdev_gdi_destroy,
    vdev_gdi_bufrequest,
    vdev_gdi_bufpost,
    vdev_gdi_textout,
    vdev_gdi_setfullsceen,
    vdev_gdi_getfullsceen,
};

