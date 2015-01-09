// 包含头文件
extern "C" {
#include "vdev.h"
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
} VDEVGDI;


// 函数实现
void* vdev_gdi_create(int w, int h, DWORD extra)
{
    VDEVGDI *dev = (VDEVGDI*)malloc(sizeof(VDEVGDI));
    if (!dev) return NULL;

    // init vdev context
    memset(dev, 0, sizeof(VDEVGDI));
    dev->width  = w;
    dev->height = h;
    dev->hwnd   = (HWND)extra;
    dev->hdcdst = GetDC(dev->hwnd);
    dev->hdcsrc = CreateCompatibleDC(dev->hdcdst);

    //++ create dibsection, get bitmap buffer address & stride ++//
    BITMAPINFO bmpinfo;
    memset(&bmpinfo, 0, sizeof(bmpinfo));
    bmpinfo.bmiHeader.biSize        =  sizeof(BITMAPINFOHEADER);
    bmpinfo.bmiHeader.biWidth       =  w;
    bmpinfo.bmiHeader.biHeight      = -h;
    bmpinfo.bmiHeader.biPlanes      =  1;
    bmpinfo.bmiHeader.biBitCount    =  32;
    bmpinfo.bmiHeader.biCompression =  BI_RGB;

    dev->hbmp = CreateDIBSection(dev->hdcsrc, &bmpinfo, DIB_RGB_COLORS, &(dev->pbuf), NULL, 0);
    if (dev->hbmp) SelectObject(dev->hdcsrc, dev->hbmp);

    BITMAP bitmap;
    GetObject(dev->hbmp, sizeof(BITMAP), &bitmap);
    dev->stride = bitmap.bmWidthBytes * 8 / 32;
    //-- create dibsection, get bitmap buffer address & stride --//

    return dev;
}

void vdev_gdi_destroy(void *ctxt)
{
    VDEVGDI *dev = (VDEVGDI*)ctxt;
    if (dev->hdcsrc) DeleteDC (dev->hdcsrc);
    if (dev->hdcdst) ReleaseDC(dev->hwnd, dev->hdcdst);
    if (dev->hbmp  ) DeleteObject(dev->hbmp);
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
    RECT    rect = {0};
    int     x    = 0;
    int     y    = 0;
    int     sw, sh, dw, dh;

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

    // bitblt picture to window witch stretching
    StretchBlt(dev->hdcdst, x, y, dw, dh,
               dev->hdcsrc, 0, 0, dev->width, dev->height,
               SRCCOPY);

    Sleep(1); // sleep is used to make frame pitch more uniform
}
