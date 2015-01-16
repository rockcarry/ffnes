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
} VDEVGDI;

// 函数实现
void* vdev_gdi_create(int w, int h, DWORD extra)
{
    VDEVGDI *dev = (VDEVGDI*)malloc(sizeof(VDEVGDI));
    if (!dev) {
        log_printf("failed to allocate gdi vdev context !\n");
        exit(0);
    }

    // init vdev context
    memset(dev, 0, sizeof(VDEVGDI));
    dev->width  = w;
    dev->height = h;
    dev->hwnd   = (HWND)extra;
    dev->hdcdst = GetDC(dev->hwnd);
    dev->hdcsrc = CreateCompatibleDC(dev->hdcdst);
    if (!dev->hdcdst || !dev->hdcsrc) {
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

    dev->hbmp = CreateDIBSection(dev->hdcsrc, &bmpinfo, DIB_RGB_COLORS, &dev->pbuf, NULL, 0);
    if (!dev->hbmp) {
        log_printf("failed to create gdi dib section !\n");
        exit(0);
    }
    else SelectObject(dev->hdcsrc, dev->hbmp);

    BITMAP bitmap;
    GetObject(dev->hbmp, sizeof(BITMAP), &bitmap);
    dev->stride = bitmap.bmWidthBytes * 8 / 32;
    //-- create dibsection, get bitmap buffer address & stride --//

    return dev;
}

void vdev_gdi_destroy(void *ctxt)
{
    VDEVGDI *dev = (VDEVGDI*)ctxt;
    DeleteDC    (dev->hdcsrc);
    ReleaseDC   (dev->hwnd, dev->hdcdst);
    DeleteObject(dev->hbmp);
    free(dev); // free vdev context
}

void vdev_gdi_buf_request(void *ctxt, void **buf, int *stride)
{
    VDEVGDI *dev = (VDEVGDI*)ctxt;
    if (buf   ) *buf    = dev->pbuf;
    if (stride) *stride = dev->stride;
}

void vdev_gdi_buf_post(void *ctxt)
{
    VDEVGDI *dev = (VDEVGDI*)ctxt;

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
        dev->rtview.right  = dw;
        dev->rtview.bottom = dh;
    }

    if (dev->texttick > GetTickCount())
    {
        SetBkMode   (dev->hdcsrc, TRANSPARENT);
        SetTextColor(dev->hdcsrc, RGB(255,255,255));
        TextOut(dev->hdcsrc, dev->textposx, dev->textposy, dev->textstr, (int)strlen(dev->textstr));
    }
    else dev->priority = 0;

    // bitblt picture to window witch stretching
    StretchBlt(dev->hdcdst, dev->rtview.left, dev->rtview.top, dev->rtview.right, dev->rtview.bottom,
               dev->hdcsrc, 0, 0, dev->width, dev->height, SRCCOPY);

    Sleep(1); // sleep is used to make frame pitch more uniform
}

void vdev_gdi_textout(void *ctxt, int x, int y, char *text, int time, int priority)
{
    VDEVGDI *dev = (VDEVGDI*)ctxt;
    if (priority >= dev->priority)
    {
        strncpy(dev->textstr, text, 256);
        dev->textposx = x;
        dev->textposy = y;
        dev->texttick = (time >= 0) ? (GetTickCount() + time) : 0xffffffff;
        dev->priority = priority;
    }
}

// gdi vdev can't support fullscreen mode
int  vdev_gdi_getfullsceen(void *ctxt)           { return ((VDEVGDI*)ctxt)->full; }
void vdev_gdi_setfullsceen(void *ctxt, int full) { ((VDEVGDI*)ctxt)->full = full; }

