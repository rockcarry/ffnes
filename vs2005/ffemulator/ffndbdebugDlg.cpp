// ffndbdebugDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ffemulator.h"
#include "ffndbdebugDlg.h"

// 内部常量定义
static RECT s_rtCpuInfo = { 371, 87, 747, 425 };

// CffndbdebugDlg dialog

IMPLEMENT_DYNAMIC(CffndbdebugDlg, CDialog)

CffndbdebugDlg::CffndbdebugDlg(CWnd* pParent, NES *pnes)
    : CDialog(CffndbdebugDlg::IDD, pParent)
    , m_nCpuStopCond(0)
    , m_strCpuStopNSteps("1")
    , m_strCpuStopToPC("FFFF")
{
    // save nes pointer
    m_pNES = pnes;
}

CffndbdebugDlg::~CffndbdebugDlg()
{
}

void CffndbdebugDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Radio(pDX, IDC_RDO_CPU_KEEP_RUNNING, m_nCpuStopCond    );
    DDX_Text (pDX, IDC_EDT_NSTEPS          , m_strCpuStopNSteps);
    DDX_Text (pDX, IDC_EDT_PC              , m_strCpuStopToPC  );
}

BEGIN_MESSAGE_MAP(CffndbdebugDlg, CDialog)
    ON_WM_DESTROY()
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BTN_NES_RESET    , &CffndbdebugDlg::OnBnClickedBtnNesReset)
    ON_BN_CLICKED(IDC_BTN_NES_RUN_PAUSE, &CffndbdebugDlg::OnBnClickedBtnNesRunPause)
    ON_BN_CLICKED(IDC_BTN_NES_DEBUG_CPU, &CffndbdebugDlg::OnBnClickedBtnNesDebugCpu)
    ON_BN_CLICKED(IDC_BTN_NES_DEBUG_PPU, &CffndbdebugDlg::OnBnClickedBtnNesDebugPpu)
    ON_BN_CLICKED(IDC_BTN_CPU_GOTO     , &CffndbdebugDlg::OnBnClickedBtnCpuGoto)
    ON_BN_CLICKED(IDC_BTN_CPU_STEP     , &CffndbdebugDlg::OnBnClickedBtnCpuStep)
END_MESSAGE_MAP()

// CffndbdebugDlg message handlers
void CffndbdebugDlg::OnOK()
{
    CDialog::OnOK();
    DestroyWindow();
}

void CffndbdebugDlg::OnCancel()
{
    CDialog::OnCancel();
    DestroyWindow();
}

BOOL CffndbdebugDlg::OnInitDialog()
{
    // init debug type
    m_nDebugType = DT_DEBUG_CPU;

    // update button
    CWnd *pwnd = GetDlgItem(IDC_BTN_NES_RUN_PAUSE);
    if (m_pNES->isrunning) {
        pwnd->SetWindowText("running");
    }
    else pwnd->SetWindowText("paused");

    // create dc
    m_cdcDraw.CreateCompatibleDC(NULL);

    // create bitmap
    m_bmpCpuInfo.CreateBitmap(
        s_rtCpuInfo.right - s_rtCpuInfo.left,
        s_rtCpuInfo.bottom - s_rtCpuInfo.top,
        1, 32, NULL);

    // create font
    m_fntDraw.CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH , "Fixedsys");

    // create pen
    m_penDraw.CreatePen(PS_SOLID, 2, RGB(128, 128, 128));

    // setup timer
    SetTimer(NDB_TIMER, 50, NULL);

    // update data false
    UpdateData(FALSE);

    return TRUE;
}

void CffndbdebugDlg::OnDestroy()
{
    CDialog::OnDestroy();

    // kill timer
    KillTimer(NDB_TIMER);

    // delete dc & object
    m_cdcDraw.DeleteDC();
    m_bmpCpuInfo.DeleteObject();
    m_fntDraw.DeleteObject();
    m_penDraw.DeleteObject();

    // delete self
    delete this;
}

void CffndbdebugDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    if (m_nDebugType == DT_DEBUG_CPU)
    {
        int savedc = m_cdcDraw.SaveDC();
        m_cdcDraw.SelectObject(&m_bmpCpuInfo);
        dc.BitBlt(s_rtCpuInfo.left, s_rtCpuInfo.top,
            s_rtCpuInfo.right - s_rtCpuInfo.left,
            s_rtCpuInfo.bottom - s_rtCpuInfo.top,
            &m_cdcDraw, 0, 0, SRCCOPY);
        m_cdcDraw.RestoreDC(savedc);
    }
}

void CffndbdebugDlg::DrawGrid(int m, int n, int *x, int *y)
{
    for (int i=0; i<n; i++)
    {
        m_cdcDraw.MoveTo(x[0  ], y[i]);
        m_cdcDraw.LineTo(x[m-1], y[i]);
    }

    for (int i=0; i<m; i++)
    {
        m_cdcDraw.MoveTo(x[i], y[0    ]);
        m_cdcDraw.LineTo(x[i], y[n - 1]);
    }
}

void CffndbdebugDlg::DrawCpuInfo()
{
    char cpuinfo[128] = {0};
    RECT rect  = {0, 0, s_rtCpuInfo.right - s_rtCpuInfo.left, s_rtCpuInfo.bottom - s_rtCpuInfo.top };

    ndb_cpu_info_str(&(m_pNES->ndb), cpuinfo);

    // save dc
    int savedc = m_cdcDraw.SaveDC();

    m_cdcDraw.SelectObject(&m_bmpCpuInfo);
    m_cdcDraw.SelectObject(&m_fntDraw);
    m_cdcDraw.SelectObject(&m_penDraw);
    m_cdcDraw.FillSolidRect(&rect, RGB(255, 255, 255));
    m_cdcDraw.DrawEdge     (&rect, EDGE_ETCHED, BF_RECT);

    rect.left += 6; rect.top += 6;
    m_cdcDraw.DrawText(" pc   sp  ax  xi  yi     ps", -1, &rect, 0); rect.top += 20;
    m_cdcDraw.DrawText(cpuinfo                      , -1, &rect, 0);

    int gridx[] = { 1, 43+33*0, 43+33*1, 43+33*2, 43+33*3, 43+33*4, 258 };
    int gridy[] = { 2, 24, 44 };
    DrawGrid(7, 3, gridx, gridy);

    // restore dc
    m_cdcDraw.RestoreDC(savedc);

    // invalidate rect
    InvalidateRect(&s_rtCpuInfo, FALSE);
}

void CffndbdebugDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (m_nDebugType)
    {
    case DT_DEBUG_CPU: DrawCpuInfo(); break;
    default:                          break;
    }

    CDialog::OnTimer(nIDEvent);
}

void CffndbdebugDlg::OnBnClickedBtnNesReset()
{
    nes_reset(m_pNES);
}

void CffndbdebugDlg::OnBnClickedBtnNesRunPause()
{
    CWnd *pwnd = GetDlgItem(IDC_BTN_NES_RUN_PAUSE);
    if (m_pNES->isrunning) {
        nes_pause(m_pNES);
        pwnd->SetWindowText("paused");
    }
    else {
        nes_run(m_pNES);
        pwnd->SetWindowText("running");
    }
}

void CffndbdebugDlg::OnBnClickedBtnNesDebugCpu()
{
    // for debug type
    m_nDebugType = DT_DEBUG_CPU;

    for (int i=IDC_GRP_CPU_CONTROL; i<=IDC_LST_OPCODE; i++)
    {
        CWnd *pwnd = GetDlgItem(i);
        pwnd->ShowWindow(SW_SHOW);
    }
    Invalidate(TRUE);
}

void CffndbdebugDlg::OnBnClickedBtnNesDebugPpu()
{
    // for debug type
    m_nDebugType = DT_DEBUG_PPU;

    for (int i=IDC_GRP_CPU_CONTROL; i<=IDC_LST_OPCODE; i++)
    {
        CWnd *pwnd = GetDlgItem(i);
        pwnd->ShowWindow(SW_HIDE);
    }
    Invalidate(TRUE);
}

void CffndbdebugDlg::OnBnClickedBtnCpuGoto()
{
    DWORD dwparam = 0;

    UpdateData(TRUE);
    switch (m_nCpuStopCond)
    {
    case NDB_CPU_RUN_NSTEPS:
        dwparam = atoi(m_strCpuStopNSteps);
        break;

    case NDB_CPU_STOP_PCEQU:
        dwparam = strtoul(m_strCpuStopToPC, NULL, 16);
        break;
    }
    ndb_cpu_runto(&(m_pNES->ndb), m_nCpuStopCond, &dwparam);
}

void CffndbdebugDlg::OnBnClickedBtnCpuStep()
{
    DWORD nsteps = 1;
    ndb_cpu_runto(&(m_pNES->ndb), NDB_CPU_RUN_NSTEPS, &nsteps);
}

