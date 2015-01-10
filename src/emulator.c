// 包含头文件
#include "emulator.h"
#include "nes.h"

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
    NES *nes = (NES*)GetWindowLong(hwnd, GWL_USERDATA);
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        switch (wparam)
        {
        case 'E': nes_joypad(nes, 0, NES_KEY_UP     , 1); break;
        case 'D': nes_joypad(nes, 0, NES_KEY_DOWN   , 1); break;
        case 'S': nes_joypad(nes, 0, NES_KEY_LEFT   , 1); break;
        case 'F': nes_joypad(nes, 0, NES_KEY_RIGHT  , 1); break;
        case 'J': nes_joypad(nes, 0, NES_KEY_A      , 1); break;
        case 'K': nes_joypad(nes, 0, NES_KEY_B      , 1); break;
        case 'U': nes_joypad(nes, 0, NES_KEY_TURBO_A, 1); break;
        case 'I': nes_joypad(nes, 0, NES_KEY_TURBO_B, 1); break;
        case 'B': nes_joypad(nes, 0, NES_KEY_SELECT , 1); break;
        case 'N': nes_joypad(nes, 0, NES_KEY_START  , 1); break;
        }
        break;
    case WM_KEYUP:
        switch (wparam)
        {
        case 'E': nes_joypad(nes, 0, NES_KEY_UP     , 0); break;
        case 'D': nes_joypad(nes, 0, NES_KEY_DOWN   , 0); break;
        case 'S': nes_joypad(nes, 0, NES_KEY_LEFT   , 0); break;
        case 'F': nes_joypad(nes, 0, NES_KEY_RIGHT  , 0); break;
        case 'J': nes_joypad(nes, 0, NES_KEY_A      , 0); break;
        case 'K': nes_joypad(nes, 0, NES_KEY_B      , 0); break;
        case 'U': nes_joypad(nes, 0, NES_KEY_TURBO_A, 0); break;
        case 'I': nes_joypad(nes, 0, NES_KEY_TURBO_B, 0); break;
        case 'B': nes_joypad(nes, 0, NES_KEY_SELECT , 0); break;
        case 'N': nes_joypad(nes, 0, NES_KEY_START  , 0); break;
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return TRUE;
}

static BOOL ShowOpenFileDialog(HWND hwnd, char *filename)
{
    OPENFILENAME ofn = {0};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = hwnd;
    ofn.lpstrFilter  = "nes rom file (*.nes).\0*.nes\0";
    ofn.lpstrFile    = filename;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = "open nes rom file:";
    ofn.Flags        = OFN_FILEMUSTEXIST | OFN_EXPLORER;
    return GetOpenFileName(&ofn);
}

// 函数实现
int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE hPreInst, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wc             = {0};
    MSG      msg            = {0};
    HWND     hwnd           = NULL;
    RECT     rect           = {0};
    char     file[MAX_PATH] = {0};
    int      w, h, x, y;
    NES      nes;

    // 注册窗口
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hCurInst;
    wc.hIcon         = LoadIcon  (NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = APP_CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    if (!RegisterClass(&wc)) return FALSE;

    // 创建窗口
    hwnd = CreateWindow(
        APP_CLASS_NAME,
        APP_WND_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NES_WIDTH,
        NES_HEIGHT,
        NULL,
        NULL,
        hCurInst,
        NULL);
    if (!hwnd) return FALSE;

    if (!ShowOpenFileDialog(hwnd, file))
    {
        nCmdShow = SW_HIDE;
        PostQuitMessage(0);
    }

    GetClientRect(hwnd, &rect);
    w = NES_WIDTH  + (NES_WIDTH  - rect.right );
    h = NES_HEIGHT + (NES_HEIGHT - rect.bottom);
    x = (SCREEN_WIDTH  - w) / 2;
    y = (SCREEN_HEIGHT - h) / 2;
    x = x > 0 ? x : 0;
    y = y > 0 ? y : 0;

    // 显示窗口
    MoveWindow(hwnd, x, y, w, h, FALSE);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // init nes
    SetWindowLong(hwnd, GWL_USERDATA, (LONG)&nes);
    nes_init(&nes, file, (DWORD)hwnd);
    nes_run (&nes);

    // 进入消息循环
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage (&msg);
    }

    // free nes
    nes_free(&nes);
    return TRUE;
}
