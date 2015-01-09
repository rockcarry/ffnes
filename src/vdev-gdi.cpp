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
    HBITMAP*hbmps;
    void  **pbufs;
    int     stride;
    int     bufnum;
    int     curnum;
    int     head;
    int     tail;
    HANDLE  bufsem;
    HANDLE  hEvent;
    HANDLE  hThread;
    BOOL    bExit;
} VDEVGDI;

// 内部函数实现
static DWORD WINAPI VDEVThreadProc(LPVOID lpParam)
{
    VDEVGDI *dev = (VDEVGDI*)lpParam;
    RECT    rect = {0};
    int     x    = 0;
    int     y    = 0;
    int sw, sh, dw, dh;

    while (!dev->bExit)
    {
        // wait for video rendering event
        WaitForSingleObject(dev->hEvent, -1);

        if (dev->curnum <= 0) continue; // if no frame to render
        else SelectObject(dev->hdcsrc, dev->hbmps[dev->head]);

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

        // update head & curnum
        if (++dev->head == dev->bufnum) dev->head = 0; dev->curnum--;
        ReleaseSemaphore(dev->bufsem, 1, NULL);
    }
    return 0;
}

// 函数实现
void* vdev_gdi_create(int bufnum, int w, int h, DWORD extra)
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
    dev->bufnum = bufnum;

    //++ create dibsection, get bitmap buffer address & stride ++//
    BITMAPINFO bmpinfo = {0};
    bmpinfo.bmiHeader.biSize        =  sizeof(BITMAPINFOHEADER);
    bmpinfo.bmiHeader.biWidth       =  w;
    bmpinfo.bmiHeader.biHeight      = -h;
    bmpinfo.bmiHeader.biPlanes      =  1;
    bmpinfo.bmiHeader.biBitCount    =  32;
    bmpinfo.bmiHeader.biCompression =  BI_RGB;

    int i;
    dev->hbmps = (HBITMAP*)malloc(sizeof(HBITMAP) * dev->bufnum);
    dev->pbufs = (void**  )malloc(sizeof(void*  ) * dev->bufnum);
    for (i=0; i<dev->bufnum; i++) {
        dev->hbmps[i] = CreateDIBSection(dev->hdcsrc, &bmpinfo, DIB_RGB_COLORS, &(dev->pbufs[i]), NULL, 0);
    }

    BITMAP bitmap = {0};
    GetObject(dev->hbmps[0], sizeof(BITMAP), &bitmap);
    dev->stride = bitmap.bmWidthBytes * 8 / 32;
    //-- create dibsection, get bitmap buffer address & stride --//

    // create video rendering thread
    dev->bufsem  = CreateSemaphore(NULL, bufnum, bufnum, NULL);
    dev->hEvent  = CreateEvent (NULL, FALSE, FALSE, "FFNES_VDEV_EVENT");
    dev->hThread = CreateThread(NULL, 0, VDEVThreadProc, (LPVOID)dev, 0, NULL);
    return dev;
}

void vdev_gdi_destroy(void *ctxt)
{
    VDEVGDI *dev = (VDEVGDI*)ctxt;

    // exit vdev rendering thread
    dev->bExit = TRUE; SetEvent(dev->hEvent);
    WaitForSingleObject(dev->hThread, -1);
    CloseHandle(dev->hThread);
    CloseHandle(dev->hEvent );
    CloseHandle(dev->bufsem);

    // destroy gdi objects & buffers
    if (dev->hdcsrc) DeleteDC (dev->hdcsrc);
    if (dev->hdcdst) ReleaseDC(dev->hwnd, dev->hdcdst);
    if (dev->hbmps )
    {
        int i;
        for (i=0; i<dev->bufnum; i++) {
            DeleteObject(dev->hbmps[i]);
        }
        free(dev->hbmps);
    }
    if (dev->pbufs) free(dev->pbufs);

    free(dev); // free vdev context
}

void vdev_gdi_buf_request(void *ctxt, void **buf, int *stride)
{
    VDEVGDI *dev = (VDEVGDI*)ctxt;
    WaitForSingleObject(dev->bufsem, -1);
    if (buf   ) *buf    = dev->pbufs[dev->tail];
    if (stride) *stride = dev->stride;
}

void vdev_gdi_buf_post(void *ctxt)
{
    VDEVGDI *dev = (VDEVGDI*)ctxt;
    if (++dev->tail == dev->bufnum) dev->tail = 0; dev->curnum++;
}
