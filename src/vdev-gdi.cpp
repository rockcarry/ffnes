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

    RECT    rtlast;
    RECT    rtview;
    int     full;

    char    textstr[256];
    int     textposx;
    int     textposy;
    DWORD   texttick;
    int     priority;
} DEVGDICTXT;

// 接口函数实现
static void* vdev_gdi_create(int w, int h, DWORD extra)
{
    DEVGDICTXT *ctxt = (DEVGDICTXT*)malloc(sizeof(DEVGDICTXT));
    if (!ctxt) {
        log_printf("failed to allocate gdi vdev context !\n");
        exit(0);
    }

    // init vdev context
    memset(ctxt, 0, sizeof(DEVGDICTXT));
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
    DEVGDICTXT *c = (DEVGDICTXT*)ctxt;
    DeleteDC    (c->hdcsrc);
    ReleaseDC   (c->hwnd, c->hdcdst);
    DeleteObject(c->hbmp);
    free(c); // free vdev context
}

static void vdev_gdi_dequeue(void *ctxt, void **buf, int *stride)
{
    DEVGDICTXT *c = (DEVGDICTXT*)ctxt;
    if (buf   ) *buf    = c->pbuf;
    if (stride) *stride = c->stride;
}

static void vdev_gdi_enqueue(void *ctxt)
{
    DEVGDICTXT *c = (DEVGDICTXT*)ctxt;
    RECT        rect;

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

            c->rtview.right = x;
            InvalidateRect(c->hwnd, &c->rtview, TRUE);
            c->rtview.left  = sw - x;
            c->rtview.right = sw;
            InvalidateRect(c->hwnd, &c->rtview, TRUE);
        }
        //-- keep picture w/h ratio when stretching --//

        c->rtview.left   = x;
        c->rtview.top    = y;
        c->rtview.right  = dw;
        c->rtview.bottom = dh;
    }

    if (c->texttick > GetTickCount())
    {
        SetBkMode   (c->hdcsrc, TRANSPARENT);
        SetTextColor(c->hdcsrc, RGB(255,255,255));
        TextOut(c->hdcsrc, c->textposx, c->textposy, c->textstr, (int)strlen(c->textstr));
    }
    else c->priority = 0;

    // bitblt picture to window witch stretching
    StretchBlt(c->hdcdst, c->rtview.left, c->rtview.top, c->rtview.right, c->rtview.bottom,
               c->hdcsrc, 0, 0, c->width, c->height, SRCCOPY);

    Sleep(1); // sleep is used to make frame pitch more uniform
}

static void vdev_gdi_textout(void *ctxt, int x, int y, char *text, int time, int priority)
{
    DEVGDICTXT *c = (DEVGDICTXT*)ctxt;
    if (priority >= c->priority)
    {
        strncpy(c->textstr, text, 256);
        c->textposx = x;
        c->textposy = y;
        c->texttick = (time >= 0) ? (GetTickCount() + time) : 0xffffffff;
        c->priority = priority;
    }
}

// gdi vdev can't support fullscreen mode
static void vdev_gdi_setfullsceen(void *ctxt, int full) { ((DEVGDICTXT*)ctxt)->full = full; }
static int  vdev_gdi_getfullsceen(void *ctxt)           { return ((DEVGDICTXT*)ctxt)->full; }

// 全局变量定义
VDEV DEV_GDI =
{
    vdev_gdi_create,
    vdev_gdi_destroy,
    vdev_gdi_dequeue,
    vdev_gdi_enqueue,
    vdev_gdi_textout,
    vdev_gdi_setfullsceen,
    vdev_gdi_getfullsceen,
};

