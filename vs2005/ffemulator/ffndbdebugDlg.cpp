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
{
    // init varibles
    m_pNES            = pnes;
    m_pDASM           = NULL;
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
    DDX_Control(pDX, IDC_LST_OPCODE          , m_ctrInstructionList);
}

BOOL CffndbdebugDlg::PreTranslateMessage(MSG* pMsg)
{
    switch (pMsg->message)
    {
    case WM_KEYDOWN:
        if (::GetFocus() == m_ctrInstructionList.GetSafeHwnd())
        {
            if (  (pMsg->wParam == VK_HOME || pMsg->wParam == VK_END)
                && GetKeyState(VK_CONTROL) < 0 )
            {
                int n = (pMsg->wParam == VK_HOME) ? 0 : m_ctrInstructionList.GetItemCount() - 1;
                m_ctrInstructionList.EnsureVisible(n, FALSE);
                m_ctrInstructionList.SetItemState (m_ctrInstructionList.SetSelectionMark(n), 0, LVIS_SELECTED);
                m_ctrInstructionList.SetItemState (n, LVIS_SELECTED, LVIS_SELECTED);
                return TRUE;
            }

            if (  pMsg->wParam == 'F' && GetKeyState(VK_CONTROL) < 0
               || pMsg->wParam == VK_F3 && m_strCurFindStr.Compare("") == 0 )
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
                return TRUE;
            }

            if (pMsg->wParam == VK_F3 && m_strCurFindStr.Compare("") != 0)
            {
                FindStrInListCtrl(m_strCurFindStr, m_bIsSearchDown);
                return TRUE;
            }

            if (pMsg->wParam == 'B' && GetKeyState(VK_CONTROL) < 0)
            {
                OnAddbreakpoint();
                return TRUE;
            }

            if (pMsg->wParam == 'D' && GetKeyState(VK_CONTROL) < 0)
            {
                OnDelbreakpoint();
                return TRUE;
            }
        }
        break;
    }
    return CDialog::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CffndbdebugDlg, CDialog)
    ON_WM_DESTROY()
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BTN_NES_RESET     , &CffndbdebugDlg::OnBnClickedBtnNesReset)
    ON_BN_CLICKED(IDC_BTN_NES_RUN_PAUSE , &CffndbdebugDlg::OnBnClickedBtnNesRunPause)
    ON_BN_CLICKED(IDC_BTN_NES_DEBUG_CPU , &CffndbdebugDlg::OnBnClickedBtnNesDebugCpu)
    ON_BN_CLICKED(IDC_BTN_NES_DEBUG_PPU , &CffndbdebugDlg::OnBnClickedBtnNesDebugPpu)
    ON_BN_CLICKED(IDC_BTN_CPU_GOTO      , &CffndbdebugDlg::OnBnClickedBtnCpuGoto)
    ON_BN_CLICKED(IDC_BTN_CPU_STEP      , &CffndbdebugDlg::OnBnClickedBtnCpuStep)
    ON_BN_CLICKED(IDC_BTN_CPU_TRACKING  , &CffndbdebugDlg::OnBnClickedBtnCpuTracking)
    ON_REGISTERED_MESSAGE(WM_FINDREPLACE, &CffndbdebugDlg::OnFindReplace)
    ON_NOTIFY(NM_RCLICK, IDC_LST_OPCODE , &CffndbdebugDlg::OnRclickListDasm)
    ON_COMMAND(ID_ADDBREAKPOINT         , &CffndbdebugDlg::OnAddbreakpoint)
    ON_COMMAND(ID_DELBREAKPOINT         , &CffndbdebugDlg::OnDelbreakpoint)
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
        pwnd->SetWindowText("disable pc tracking");
    }
    else pwnd->SetWindowText("enable pc tracking");

    // allcate dasm
    m_pDASM = new DASM();

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

    //++ create & init list control
    m_ctrInstructionList.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LVS_REPORT|LVS_SHOWSELALWAYS|WS_TABSTOP, CRect(9, 87, 356, 527), this, IDC_LST_OPCODE);
    m_ctrInstructionList.SetExtendedStyle(m_ctrInstructionList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    m_ctrInstructionList.InsertColumn(0, "b"       , LVCFMT_LEFT, 18 );
    m_ctrInstructionList.InsertColumn(1, "pc"      , LVCFMT_LEFT, 40 );
    m_ctrInstructionList.InsertColumn(2, "opcode"  , LVCFMT_LEFT, 70 );
    m_ctrInstructionList.InsertColumn(3, "asm"     , LVCFMT_LEFT, 90 );
    m_ctrInstructionList.InsertColumn(4, "comments", LVCFMT_LEFT, 110);
    //-- create & init list control

    // setup timer
    SetTimer(NDB_REFRESH_TIMER, 50, NULL);
    SetTimer(NDB_DIASM_TIMER  , 50, NULL);

    // update data false
    UpdateData(FALSE);

    return TRUE;
}

void CffndbdebugDlg::OnDestroy()
{
    CDialog::OnDestroy();

    // kill timer
    KillTimer(NDB_REFRESH_TIMER);
    KillTimer(NDB_DIASM_TIMER  );

    // delete dc & object
    m_cdcDraw.DeleteDC();
    m_bmpCpuInfo.DeleteObject();
    m_fntDraw.DeleteObject();
    m_penDraw.DeleteObject();

    // delete m_pDASM
    delete m_pDASM;

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
    switch (nIDEvent)
    {
    case NDB_REFRESH_TIMER:
        switch (m_nDebugType)
        {
        case DT_DEBUG_CPU:
            if (m_bEnableTracking) UpdateCurInstHighLight();
            DrawCpuDebugging();
            break;
        }
        break;

    case NDB_DIASM_TIMER:
        KillTimer(NDB_DIASM_TIMER);
        DoNesRomDisAsm(); // do nes rom disasm
        m_nDebugType = DT_DEBUG_CPU;
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

    case NDB_CPU_RUN_BPOINTS:
        ndb_cpu_runto(&(m_pNES->ndb), m_nCpuStopCond, NULL);
        break;
    }
}

void CffndbdebugDlg::OnBnClickedBtnCpuStep()
{
    DWORD nsteps = 1;
    ndb_cpu_runto(&(m_pNES->ndb), NDB_CPU_RUN_NSTEPS, &nsteps); // run 1 step
    while (!m_pNES->ndb.stop) Sleep(20); // wait for cpu stop
    UpdateCurInstHighLight(); // update cursor high light for instruction list
}

void CffndbdebugDlg::OnBnClickedBtnCpuTracking()
{
    CWnd *pwnd = GetDlgItem(IDC_BTN_CPU_TRACKING);
    m_bEnableTracking = !m_bEnableTracking;
    if (m_bEnableTracking) {
        pwnd->SetWindowText("disable pc tracking");
    }
    else pwnd->SetWindowText("enable pc tracking");
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

void CffndbdebugDlg::OnRclickListDasm(NMHDR *pNMHDR, LRESULT *pResult)
{
    DWORD  dwPos = GetMessagePos();
    CPoint point(LOWORD(dwPos), HIWORD(dwPos));

    CMenu  menu;
    menu.LoadMenu(IDR_MENU1);
    menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, this);
    *pResult = 0;
}

void CffndbdebugDlg::OnAddbreakpoint()
{
    POSITION pos = m_ctrInstructionList.GetFirstSelectedItemPosition();
    if (pos)
    {
        while (pos)
        {
            int   n  = 0;
            DWORD pc = 0;
            n = m_ctrInstructionList.GetNextSelectedItem(pos);
            sscanf(m_ctrInstructionList.GetItemText(n, 1), "%x", &pc);

            m_ctrInstructionList.SetItemText(n, 0, "B");
            if (!ndb_add_bpoint(&(m_pNES->ndb), (WORD)pc))
            {
                MessageBox(CString("only support 16 break points !"), "ffndb find", MB_ICONASTERISK|MB_ICONINFORMATION);
                break;
            }
        }
    }
}

void CffndbdebugDlg::OnDelbreakpoint()
{
    POSITION pos = m_ctrInstructionList.GetFirstSelectedItemPosition();
    if (pos)
    {
        while (pos)
        {
            int   n  = 0;
            DWORD pc = 0;
            n = m_ctrInstructionList.GetNextSelectedItem(pos);
            sscanf(m_ctrInstructionList.GetItemText(n, 1), "%x", &pc);

            m_ctrInstructionList.SetItemText(n, 0, "");
            ndb_del_bpoint(&(m_pNES->ndb), (WORD)pc);
        }
    }
}

///////////////////////////////////////////////
// private methods
///////////////////////////////////////////////
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
    char str[64];
    int  n, i;

    BeginWaitCursor();
    ndb_dasm_nes_rom(&(m_pNES->ndb), m_pDASM);
    m_ctrInstructionList.InsertItem(0, "{");
    for (n=1; n<m_pDASM->curinstn; n++)
    {
        m_ctrInstructionList.InsertItem(n, "");

        sprintf(str, "%04X", m_pDASM->instlist[n].pc);
        m_ctrInstructionList.SetItemText(n, 1, str);

        for (i=0; i<m_pDASM->instlist[n].len; i++) {
            sprintf(&(str[i*3]), "%02X ", m_pDASM->instlist[n].bytes[i]);
        }
        m_ctrInstructionList.SetItemText(n, 2, str);

        m_ctrInstructionList.SetItemText(n, 3, m_pDASM->instlist[n].asmstr );
        m_ctrInstructionList.SetItemText(n, 4, m_pDASM->instlist[n].comment);
    }
    m_ctrInstructionList.InsertItem(n, "}");
    EndWaitCursor();
}

void CffndbdebugDlg::UpdateCurInstHighLight()
{
    int n = ndb_dasm_pc2instn(&(m_pNES->ndb), m_pDASM, m_pNES->ndb.curpc);
    m_ctrInstructionList.EnsureVisible(n, FALSE);
    m_ctrInstructionList.SetItemState (m_ctrInstructionList.SetSelectionMark(n), 0, LVIS_SELECTED);
    m_ctrInstructionList.SetItemState (n, LVIS_SELECTED, LVIS_SELECTED);
}

void CffndbdebugDlg::FindStrInListCtrl(CString str, BOOL down)
{
    int     d     = down ? 1 : -1;
    int     n     = m_ctrInstructionList.GetSelectionMark() + d;
    int     total = m_ctrInstructionList.GetItemCount();
    CString find  = str.MakeUpper();

    BeginWaitCursor();

    while (n >= 0 && n < total) {
        CString strItem = "";
        strItem += m_ctrInstructionList.GetItemText(n, 1) + "|";
        strItem += m_ctrInstructionList.GetItemText(n, 2) + "|";
        strItem += m_ctrInstructionList.GetItemText(n, 3) + "|";
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

    EndWaitCursor();
}



