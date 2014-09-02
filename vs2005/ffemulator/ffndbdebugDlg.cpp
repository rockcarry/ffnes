// ffndbdebugDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ffemulator.h"
#include "ffndbdebugDlg.h"

// 内部常量定义
static RECT s_rtCpuInfo    = { 362, 87, 750, 425 };
static int  WM_FINDREPLACE = RegisterWindowMessage(FINDMSGSTRING);

// CffndbdebugDlg dialog

IMPLEMENT_DYNAMIC(CffndbdebugDlg, CDialog)

CffndbdebugDlg::CffndbdebugDlg(CWnd* pParent, NES *pnes)
    : CDialog(CffndbdebugDlg::IDD, pParent)
    , m_nCpuStopCond(0)
    , m_strCpuStopNSteps("1")
    , m_strCpuStopToPC("FFFF")
{
    // init varibles
    m_pNES            = pnes;
    m_nDebugType      = 0;
    m_pPCInstMapTab   = NULL;
    m_bEnableTracking = FALSE;
    m_bIsSearchDown   = TRUE;
}

CffndbdebugDlg::~CffndbdebugDlg()
{
}

void CffndbdebugDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Radio  (pDX, IDC_RDO_CPU_KEEP_RUNNING, m_nCpuStopCond      );
    DDX_Text   (pDX, IDC_EDT_NSTEPS          , m_strCpuStopNSteps  );
    DDX_Text   (pDX, IDC_EDT_PC              , m_strCpuStopToPC    );
    DDX_Control(pDX, IDC_LST_OPCODE          , m_ctrInstructionList);
}

BOOL CffndbdebugDlg::PreTranslateMessage(MSG* pMsg)
{
    BOOL bShowFindDialog = FALSE;

    switch (pMsg->message)
    {
    case WM_KEYDOWN:
        if (pMsg->wParam == 'F' && GetKeyState(VK_CONTROL) < 0) bShowFindDialog = TRUE;
        if (pMsg->wParam == VK_F3)
        {
            if (m_strCurFindStr.Compare("") == 0) bShowFindDialog = TRUE;
            else FindStrInListCtrl(m_strCurFindStr, m_bIsSearchDown);
        }
        if (bShowFindDialog)
        {
            CFindReplaceDialog *dlg = (CFindReplaceDialog*)FindWindow(NULL, "ffndb find");
            if (!dlg) {
                DWORD flags = FR_HIDEMATCHCASE|FR_HIDEWHOLEWORD;
                if (m_bIsSearchDown) flags |= FR_DOWN;
                dlg = new CFindReplaceDialog();
                dlg->Create(TRUE, m_strCurFindStr, NULL, flags, this);
                dlg->SetWindowText("ffndb find");
                dlg->ShowWindow(SW_SHOW);
            }
            else SwitchToThisWindow(dlg->GetSafeHwnd(), TRUE);
        }
        return TRUE;

    default: return CDialog::PreTranslateMessage(pMsg);
    }
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
    ON_BN_CLICKED(IDC_BTN_CPU_TRACKING , &CffndbdebugDlg::OnBnClickedBtnCpuTracking)
    ON_REGISTERED_MESSAGE(WM_FINDREPLACE, OnFindReplace)
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
    // update button
    CWnd *pwnd = GetDlgItem(IDC_BTN_NES_RUN_PAUSE);
    if (m_pNES->isrunning) {
        pwnd->SetWindowText("pause");
    }
    else pwnd->SetWindowText("run");

    pwnd = GetDlgItem(IDC_BTN_CPU_TRACKING);
    if (m_bEnableTracking) {
        pwnd->SetWindowText("disable cpu tracking");
    }
    else pwnd->SetWindowText("enable cpu tracking");

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

    // allocate pc-instruction map tab
    m_pPCInstMapTab = (WORD*)malloc(32 * 1024 * sizeof(WORD));

    //++ create & init list control
    m_ctrInstructionList.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LVS_REPORT|LVS_SHOWSELALWAYS, CRect(9, 87, 356, 527), this, IDC_LST_OPCODE);
    m_ctrInstructionList.SetExtendedStyle(m_ctrInstructionList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    m_ctrInstructionList.InsertColumn(0, "b"       , LVCFMT_LEFT, 18 );
    m_ctrInstructionList.InsertColumn(1, "pc"      , LVCFMT_LEFT, 40 );
    m_ctrInstructionList.InsertColumn(2, "opcode"  , LVCFMT_LEFT, 70 );
    m_ctrInstructionList.InsertColumn(3, "asm"     , LVCFMT_LEFT, 90 );
    m_ctrInstructionList.InsertColumn(4, "comments", LVCFMT_LEFT, 110);
    //-- create & init list control

    // do nes rom disasm
    DoNesRomDisAsm();

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

    // free m_pPCInstMapTab
    if (m_pPCInstMapTab) {
        free(m_pPCInstMapTab);
        m_pPCInstMapTab = NULL;
    }

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

void CffndbdebugDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (m_nDebugType)
    {
    case DT_DEBUG_CPU:
        UpdateCurInstHighLight();
        DrawCpuDebugging();
        break;
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
        pwnd->SetWindowText("run");
    }
    else {
        nes_run(m_pNES);
        pwnd->SetWindowText("pause");
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
    WORD   wparam = 0;
    DWORD dwparam = 0;
    LONG   lparam = 0;

    UpdateData(TRUE);
    switch (m_nCpuStopCond)
    {
    case NDB_CPU_KEEP_RUNNING:
        ndb_cpu_runto(&(m_pNES->ndb), m_nCpuStopCond, NULL);
        break;

    case NDB_CPU_RUN_NSTEPS:
        lparam = atoi(m_strCpuStopNSteps);
        ndb_cpu_runto(&(m_pNES->ndb), m_nCpuStopCond, &lparam);
        break;

    case NDB_CPU_STOP_PCEQU:
        wparam = (WORD)strtoul(m_strCpuStopToPC, NULL, 16);
        ndb_cpu_runto(&(m_pNES->ndb), m_nCpuStopCond, &wparam);
        break;
    }
}

void CffndbdebugDlg::OnBnClickedBtnCpuStep()
{
    DWORD nsteps = 1;
    ndb_cpu_runto(&(m_pNES->ndb), NDB_CPU_RUN_NSTEPS, &nsteps);
    if (!m_bEnableTracking)
    {
        m_bEnableTracking = TRUE;
        CWnd *pwnd = GetDlgItem(IDC_BTN_CPU_TRACKING);
        pwnd->SetWindowText("disable cpu tracking");
    }
}

void CffndbdebugDlg::OnBnClickedBtnCpuTracking()
{
    CWnd *pwnd = GetDlgItem(IDC_BTN_CPU_TRACKING);
    m_bEnableTracking = !m_bEnableTracking;
    if (m_bEnableTracking) {
        pwnd->SetWindowText("disable cpu tracking");
    }
    else pwnd->SetWindowText("enable cpu tracking");
}

LONG CffndbdebugDlg::OnFindReplace(WPARAM wparam, LPARAM lparam)
{
    CFindReplaceDialog *dlg = CFindReplaceDialog::GetNotifier(lparam);

    if (dlg->FindNext())
    {
        // save current find string
        m_strCurFindStr = dlg->GetFindString();
        m_bIsSearchDown = dlg->SearchDown();
        FindStrInListCtrl(m_strCurFindStr, m_bIsSearchDown);
    }

    return 0;
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

void CffndbdebugDlg::DrawCpuDebugging()
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
        char cpuregs[128] = {0};
        rect.left += 6; rect.top += 6;
        ndb_dump_info(&(m_pNES->ndb), NDB_DUMP_CPU_REGS0, cpuregs);
        m_cdcDraw.DrawText(cpuregs, -1, &rect, 0);
        rect.left += 0; rect.top += 22;
        ndb_dump_info(&(m_pNES->ndb), NDB_DUMP_CPU_REGS1, cpuregs);
        m_cdcDraw.DrawText(cpuregs, -1, &rect, 0);

        int gridx[] = { 3, 45+32*0, 45+32*1, 45+32*2, 45+32*3, 45+32*4, 256 };
        int gridy[] = { 3, 3+22*1, 3+22*2 };
        DrawGrid(7, 3, gridx, gridy);

        char vector[128] = {0};
        ndb_dump_info(&(m_pNES->ndb), NDB_DUMP_CPU_VECTOR, vector);
        rect.left += 266; rect.top -= 27;
        m_cdcDraw.DrawText(vector, -1, &rect, DT_LEFT);
        rect.left -= 266; rect.top += 27;
    }

    // draw stack info
    {
        char stackinfo[128] = {0};
        rect.left = 6; rect.top += 28;
        ndb_dump_info(&(m_pNES->ndb), NDB_DUMP_CPU_STACK0, stackinfo);
        m_cdcDraw.DrawText(stackinfo, -1, &rect, 0);
        rect.left = 6; rect.top += 22;
        ndb_dump_info(&(m_pNES->ndb), NDB_DUMP_CPU_STACK1, stackinfo);
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

void CffndbdebugDlg::DoNesRomDisAsm()
{
    WORD rst = bus_readw(m_pNES->cbus, 0xfffc);
    WORD nmi = bus_readw(m_pNES->cbus, 0xfffa);
    WORD irq = bus_readw(m_pNES->cbus, 0xfffe);
    WORD pc  = 0;
    BYTE bytes [ 3];
    char strasm[64];
    char strtmp[16];
    int  len = 0;
    int  i   = 0;
    int  n   = 0;

    for (pc=rst; pc<0xffff; )
    {
        len = ndb_cpu_dasm(&(m_pNES->ndb), pc, bytes, strasm);
        sprintf(strtmp, "%04X", pc);
        n = m_ctrInstructionList.InsertItem(n, "");

        m_ctrInstructionList.SetItemText(n, 1, strtmp);
        for (i=0; i<len; i++) {
            sprintf(&(strtmp[i*3]), "%02X ", bytes[i]);
        }
        m_ctrInstructionList.SetItemText(n, 2, strtmp);
        m_ctrInstructionList.SetItemText(n, 3, strasm);

        if (m_pPCInstMapTab) m_pPCInstMapTab[pc - 0x8000] = n;
        pc += len; n++;
    }
}

void CffndbdebugDlg::UpdateCurInstHighLight()
{
    if (!m_bEnableTracking) return;
    int nCurInst = m_pPCInstMapTab[m_pNES->cpu.pc - 0x8000];
    m_ctrInstructionList.EnsureVisible(nCurInst, FALSE);
    m_ctrInstructionList.SetItemState (m_ctrInstructionList.SetSelectionMark(nCurInst), 0, LVIS_SELECTED);
    m_ctrInstructionList.SetItemState (nCurInst, LVIS_SELECTED, LVIS_SELECTED);
}

void CffndbdebugDlg::FindStrInListCtrl(CString str, BOOL down)
{
    int     d     = down ? 1 : -1;
    int     n     = m_ctrInstructionList.GetSelectionMark() + d;
    int     total = m_ctrInstructionList.GetItemCount();
    CString find  = str.MakeUpper();

    while (n >= 0 && n < total) {
        CString strItem = "";
        strItem += m_ctrInstructionList.GetItemText(n, 1);
        strItem += m_ctrInstructionList.GetItemText(n, 2);
        strItem += m_ctrInstructionList.GetItemText(n, 3);
        strItem += m_ctrInstructionList.GetItemText(n, 4);
        strItem.MakeUpper();
        if (strItem.Find(find) != -1) break;
        n += d;
    }

    if (n >= 0 && n < total)
    {
        m_ctrInstructionList.EnsureVisible(n, FALSE);
        m_ctrInstructionList.SetItemState (m_ctrInstructionList.SetSelectionMark(n), 0, LVIS_SELECTED);
        m_ctrInstructionList.SetItemState (n, LVIS_SELECTED, LVIS_SELECTED);
    }
    else
    {
        MessageBox(CString("can't find \"") + str + "\"", "ffndb find", MB_ICONASTERISK|MB_ICONINFORMATION);
    }
}


