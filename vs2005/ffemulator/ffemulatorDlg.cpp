// ffemulatorDlg.cpp : implementation file
//
#include "stdafx.h"
#include "ffemulator.h"
#include "ffemulatorDlg.h"
#include "ffndbdebugDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 内部常量定义
#define SCREEN_WIDTH    GetSystemMetrics(SM_CXSCREEN)
#define SCREEN_HEIGHT   GetSystemMetrics(SM_CYSCREEN)
#define NES_WIDTH       256
#define NES_HEIGHT      240

// CffemulatorDlg dialog
CffemulatorDlg::CffemulatorDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CffemulatorDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CffemulatorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CffemulatorDlg, CDialog)
    ON_WM_DESTROY()
    ON_WM_CTLCOLOR()
    ON_WM_KEYUP()
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// CffemulatorDlg message handlers
BOOL CffemulatorDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog. The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);      // Set big icon
    SetIcon(m_hIcon, FALSE);     // Set small icon

    RECT rect = {0};
    int  x, y, w, h;
    MoveWindow(0, 0, NES_WIDTH, NES_HEIGHT, FALSE);
    GetClientRect(&rect);
    w = NES_WIDTH  + (NES_WIDTH  - rect.right );
    h = NES_HEIGHT + (NES_HEIGHT - rect.bottom);
    x = (SCREEN_WIDTH  - w) / 2;
    y = (SCREEN_HEIGHT - h) / 2;
    x = x > 0 ? x : 0;
    y = y > 0 ? y : 0;
    MoveWindow(x, y, w, h, FALSE);

    TCHAR file[MAX_PATH] = {0};
    CFileDialog dlg(TRUE, NULL, NULL, 0, "nes rom file (*.nes)|*.nes||");
    if (dlg.DoModal() != IDOK) OnOK();

    strcpy(file, dlg.GetPathName());
    nes_init(&m_nes, file, (DWORD)GetSafeHwnd());
    nes_run (&m_nes);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CffemulatorDlg::OnDestroy()
{
    CDialog::OnDestroy();

    // free nes
    nes_free(&m_nes);
}

HBRUSH CffemulatorDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    // TODO:  Change any attributes of the DC here
    // TODO:  Return a different brush if the default is not desired
    if (pWnd == this) return (HBRUSH)GetStockObject(BLACK_BRUSH);
    else return hbr;
}

void CffemulatorDlg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // TODO: Add your message handler code here and/or call default
    switch (nChar)
    {
    case 'E': joypad_setkey(&(m_nes.pad), 0, NES_KEY_UP     , 0); break;
    case 'D': joypad_setkey(&(m_nes.pad), 0, NES_KEY_DOWN   , 0); break;
    case 'S': joypad_setkey(&(m_nes.pad), 0, NES_KEY_LEFT   , 0); break;
    case 'F': joypad_setkey(&(m_nes.pad), 0, NES_KEY_RIGHT  , 0); break;
    case 'J': joypad_setkey(&(m_nes.pad), 0, NES_KEY_A      , 0); break;
    case 'K': joypad_setkey(&(m_nes.pad), 0, NES_KEY_B      , 0); break;
    case 'U': joypad_setkey(&(m_nes.pad), 0, NES_KEY_TURBO_A, 0); break;
    case 'I': joypad_setkey(&(m_nes.pad), 0, NES_KEY_TURBO_B, 0); break;
    case 'B': joypad_setkey(&(m_nes.pad), 0, NES_KEY_SELECT , 0); break;
    case 'N': joypad_setkey(&(m_nes.pad), 0, NES_KEY_START  , 0); break;
    }
    CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CffemulatorDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // ctrl+d pressed
    if (GetKeyState(VK_CONTROL) < 0 && nChar == 'D')
    {
        CDialog *dlg = (CDialog*)FindWindow(NULL, "ffndb");
        if (!dlg) {
            dlg = new CffndbdebugDlg(this, &m_nes);
            dlg->Create(IDD_FFNDBDEBUG_DIALOG, GetDesktopWindow());
            RECT rect = {0};
            this->GetWindowRect(&rect);
            this->MoveWindow(0, 0, rect.right - rect.left, rect.bottom - rect.top);
            dlg ->CenterWindow();
        }
        dlg->ShowWindow(SW_SHOW);
        return;
    }

    switch (nChar)
    {
    case 'E': joypad_setkey(&(m_nes.pad), 0, NES_KEY_UP     , 1); break;
    case 'D': joypad_setkey(&(m_nes.pad), 0, NES_KEY_DOWN   , 1); break;
    case 'S': joypad_setkey(&(m_nes.pad), 0, NES_KEY_LEFT   , 1); break;
    case 'F': joypad_setkey(&(m_nes.pad), 0, NES_KEY_RIGHT  , 1); break;
    case 'J': joypad_setkey(&(m_nes.pad), 0, NES_KEY_A      , 1); break;
    case 'K': joypad_setkey(&(m_nes.pad), 0, NES_KEY_B      , 1); break;
    case 'U': joypad_setkey(&(m_nes.pad), 0, NES_KEY_TURBO_A, 1); break;
    case 'I': joypad_setkey(&(m_nes.pad), 0, NES_KEY_TURBO_B, 1); break;
    case 'B': joypad_setkey(&(m_nes.pad), 0, NES_KEY_SELECT , 1); break;
    case 'N': joypad_setkey(&(m_nes.pad), 0, NES_KEY_START  , 1); break;
    }
    CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
}
