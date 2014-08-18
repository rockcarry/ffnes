// 包含头文件
#include "vdev.h"

typedef struct
{
    int     width;      /* 宽度 */
    int     height;     /* 高度 */
    int     cdepth;     /* 像素位深度 */
    int     stride;     /* 行宽字节数 */
    void   *pdata;      /* 指向图像数据 */
    BYTE   *ppal;       /* 指向色盘数据 */
    HWND    hwnd;       /* 窗口句柄 */
    HDC     hdcsrc;
    HDC     hdcdst;
    HBITMAP hbmp;
} VDEV;

// 内部函数实现
static void createstdpal(BYTE *pal)
{
    int r, g, b;
    for (r=0; r<256; r+=36)
    {
        for (g=0; g<256; g+=36)
        {
            for (b=0; b<256; b+=85)
            {
                *pal++ = b;
                *pal++ = g;
                *pal++ = r;
                *pal++ = 0;
            }
        }
    }
}

// 函数实现
void* vdev_create(int w, int h, int depth, DWORD extra)
{
    VDEV *dev = malloc(sizeof(VDEV));
    if (dev)
    {
        BITMAP      bitmap  = {0};
        BYTE        buffer[sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 256] = {0};
        BITMAPINFO *bmpinfo = (BITMAPINFO*)buffer;

        bmpinfo->bmiHeader.biSize        =  sizeof(BITMAPINFOHEADER);
        bmpinfo->bmiHeader.biWidth       =  w;
        bmpinfo->bmiHeader.biHeight      = -h;
        bmpinfo->bmiHeader.biPlanes      =  1;
        bmpinfo->bmiHeader.biBitCount    =  depth;
        bmpinfo->bmiHeader.biCompression =  BI_RGB;

        switch (depth)
        {
        case 8:
            createstdpal((BYTE*)bmpinfo->bmiColors);
            break;

        case 16:
            bmpinfo->bmiHeader.biCompression = BI_BITFIELDS;
            ((DWORD*)bmpinfo->bmiColors)[0]  = 0xF800;
            ((DWORD*)bmpinfo->bmiColors)[1]  = 0x07E0;
            ((DWORD*)bmpinfo->bmiColors)[2]  = 0x001F;
            break;
        }

        dev->width  = w;
        dev->height = h;
        dev->cdepth = depth;
        dev->hwnd   = (HWND)extra;
        dev->hdcdst = GetDC(dev->hwnd);
        dev->hdcsrc = CreateCompatibleDC(dev->hdcdst);
        dev->hbmp   = CreateDIBSection(dev->hdcsrc, bmpinfo, DIB_RGB_COLORS, (void**)&(dev->pdata), NULL, 0);

        GetObject(dev->hbmp, sizeof(BITMAP), &bitmap);
        dev->stride = bitmap.bmWidthBytes;
        SelectObject(dev->hdcsrc, dev->hbmp);
    }
    return dev;
}

void vdev_destroy(void *ctxt)
{
    VDEV *dev = (VDEV*)ctxt;
    if (dev == NULL) return;
    if (dev->hdcsrc) DeleteDC (dev->hdcsrc);
    if (dev->hdcdst) ReleaseDC(dev->hwnd, dev->hdcdst);
    if (dev->hbmp  ) DeleteObject(dev->hbmp);
    free(dev);
}

void vdev_setpal(void *ctxt, int i, int n, BYTE *pal)
{
    VDEV *dev = (VDEV*)ctxt;
    if (dev == NULL) return;
    SetDIBColorTable(dev->hdcsrc, i, n, (RGBQUAD*)pal);
}

void vdev_getpal(void *ctxt, int i, int n, BYTE *pal)
{
    VDEV *dev = (VDEV*)ctxt;
    if (dev == NULL) return;
    GetDIBColorTable(dev->hdcsrc, i, n, (RGBQUAD*)pal);
}

void vdev_lock(void *ctxt, void **buf, int *stride)
{
    VDEV *dev = (VDEV*)ctxt;
    if (dev == NULL) return;
    if (buf   ) *buf    = dev->pdata;
    if (stride) *stride = dev->stride;
}

void vdev_unlock(void *ctxt)
{
    RECT  rect = {0};
    VDEV *dev  = (VDEV*)ctxt;
    int   x    = 0;
    int   y    = 0;
    int  sw, sh, dw, dh;

    // check context valid
    if (dev == NULL) return;

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
}
