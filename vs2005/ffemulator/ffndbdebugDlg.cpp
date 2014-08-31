// ffndbdebugDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ffemulator.h"
#include "ffndbdebugDlg.h"

// 内部常量定义
static RECT s_rtCpuInfo = { 362, 87, 750, 425 };

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
    RECT rect  = {0, 0, s_rtCpuInfo.right - s_rtCpuInfo.left, s_rtCpuInfo.bottom - s_rtCpuInfo.top };

    // save dc
    int savedc = m_cdcDraw.SaveDC();

    m_cdcDraw.SelectObject(&m_bmpCpuInfo);
    m_cdcDraw.SelectObject(&m_fntDraw);
    m_cdcDraw.SelectObject(&m_penDraw);
    m_cdcDraw.FillSolidRect(&rect, RGB(255, 255, 255));

    // draw cpu pc & regs info
    {
        char cpuinfo[128] = {0};
        ndb_cpu_info_str(&(m_pNES->ndb), cpuinfo);
        rect.left += 6; rect.top += 6;
        m_cdcDraw.DrawText(" pc   sp  ax  xi  yi     ps", -1, &rect, 0); rect.top += 22;
        m_cdcDraw.DrawText(cpuinfo                      , -1, &rect, 0);

        int gridx[] = { 3, 45+32*0, 45+32*1, 45+32*2, 45+32*3, 45+32*4, 256 };
        int gridy[] = { 3, 3+22*1, 3+22*2 };
        DrawGrid(7, 3, gridx, gridy);
    }

    // draw stack info
    {
        char stackinfo[128] = {0};
        char stackbyte[  8] = {0};
        int  top    = 0x100 + m_pNES->cpu.sp;
        int  bottom = (top & 0xfff0) + 0xf;

        sprintf(stackinfo, "%02X                   stack                   %02X",
            (bottom & 0xff), (bottom & 0xf0));
        rect.left = 6; rect.top += 28;
        m_cdcDraw.DrawText(stackinfo, -1, &rect, 0);

        stackinfo[0] = '\0';
        for (int i=bottom; i>bottom-16; i--)
        {
            if (i > top) sprintf(stackbyte, "%02X ", m_pNES->cpu.cram[i]);
            else         sprintf(stackbyte, "-- ");
            strcat (stackinfo, stackbyte);
        }
        rect.left = 6; rect.top += 22;
        m_cdcDraw.DrawText(stackinfo, -1, &rect, 0);

        int gridx[] = { 3+24*0, 3+24*1, 3+24*2 , 3+24*3 , 3+24*4 , 3+24*5 , 3+24*6 , 3+24*7 ,
                        3+24*8, 3+24*9, 3+24*10, 3+24*11, 3+24*12, 3+24*13, 3+24*14, 3+24*15, 3+24*16-2};
        int gridy[] = { rect.top - 4, rect.top - 4 + 22 };
        DrawGrid(17, 2, gridx, gridy);
    }

    // draw edge
    rect.left = rect.top = 0;
    m_cdcDraw.DrawEdge(&rect, EDGE_ETCHED, BF_RECT);

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

