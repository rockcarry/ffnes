// 包含头文件
#include "emulator.h"

// 内部常量定义
#define SCREEN_WIDTH    GetSystemMetrics(SM_CXSCREEN)
#define SCREEN_HEIGHT   GetSystemMetrics(SM_CYSCREEN)
#define NES_WIDTH       256
#define NES_HEIGHT      240
#define APP_CLASS_NAME  "ffnes_emulator"
#define APP_WND_TITLE   "ffnes"

// 内部函数实现
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return TRUE;
}

// 函数实现
int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE hPreInst, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wc   = {0};
    MSG      msg  = {0};
    HWND     hwnd = NULL;

    // 注册窗口
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hCurInst;
    wc.hIcon         = LoadIcon  (NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = APP_CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    if (!RegisterClass(&wc)) return FALSE;

    // 创建窗口
    int winwidth  = NES_WIDTH  + 64;
    int winheight = NES_HEIGHT + 64;
    int winxpos   = (SCREEN_WIDTH  - winwidth ) / 2;
    int winypos   = (SCREEN_HEIGHT - winheight) / 2;
    hwnd = CreateWindowEx(
        0,
        APP_CLASS_NAME,
        APP_WND_TITLE,
        WS_OVERLAPPED | WS_SYSMENU,
        winxpos,
        winypos,
        winwidth,
        winheight,
        NULL,
        NULL,
        hCurInst,
        NULL);
    if (!hwnd) return FALSE;

    // 显示窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 进入消息循环
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return TRUE;
}
