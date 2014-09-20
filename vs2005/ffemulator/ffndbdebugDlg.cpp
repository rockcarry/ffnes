// ffndbdebugDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ffemulator.h"
#include "ffndbdebugDlg.h"

// 内部常量定义
static RECT s_rtCpuInfo    = { 365, 68, 776, 386 };
static RECT s_rtPpuInfo    = { 0  , 63, 783, 543 };
static RECT s_rtPpuAreas[] = {
    { 0  , 63 + 0  , 512, 63 + 480 }, // name table
    { 520, 63 + 32 , 776, 63 + 96  }, // bkgrnd tile
    { 520, 63 + 128, 776, 63 + 192 }, // sprite tile
    { 520, 63 + 224, 776, 63 + 240 }, // bkgrnd palette
    { 520, 63 + 272, 776, 63 + 288 }, // sprite palette
    { 520, 63 + 320, 776, 63 + 388 }, // sprite ram
    { 0  , 0       , 999, 999      }, // ppu details
};
static RECT s_rtListCtrl   = { 9  , 68, 356, 537 };
static int  WM_FINDREPLACE = RegisterWindowMessage(FINDMSGSTRING);

// CffndbdebugDlg dialog

IMPLEMENT_DYNAMIC(CffndbdebugDlg, CDialog)

CffndbdebugDlg::CffndbdebugDlg(CWnd* pParent, NES *pnes)
    : CDialog(CffndbdebugDlg::IDD, pParent)
    , m_strWatchAddr("2000")
    , m_strCpuStopNSteps("100000")
    , m_bCheckAutoDasm(FALSE)
{
    // init varibles
    m_pNES            = pnes;
    m_pDASM           = NULL;
    m_bEnableTracking = FALSE;
    m_bDebugTracking  = FALSE;
    m_bDebugRunNStep  = FALSE;
    m_bIsSearchDown   = TRUE;
    m_nCurPpuDetails  = 6;
}

CffndbdebugDlg::~CffndbdebugDlg()
{
}

void CffndbdebugDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text   (pDX, IDC_EDT_WATCH           , m_strWatchAddr      );
    DDX_Text   (pDX, IDC_EDT_NSTEPS          , m_strCpuStopNSteps  );
    DDX_Check  (pDX, IDC_CHECK_AUTO_DASM     , m_bCheckAutoDasm    );
    DDX_Control(pDX, IDC_EDT_LIST_CTRL       , m_edtListCtrl       );
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

            if (pMsg->wParam == 'A' && GetKeyState(VK_CONTROL) < 0)
            {
                OnDasmlistSelectall();
                return TRUE;
            }

            if (pMsg->wParam == 'C' && GetKeyState(VK_CONTROL) < 0)
            {
                OnDasmlistCopy();
                return TRUE;
            }

            if (pMsg->wParam == 'E' && GetKeyState(VK_CONTROL) < 0)
            {
                OnDasmlistEdit();
                return TRUE;
            }
        }
        if (::GetFocus() == m_edtListCtrl.GetSafeHwnd())
        {
            if (pMsg->wParam == VK_RETURN)
            {
                OnEnKillfocusEdtListCtrl();
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
    ON_WM_MOUSEMOVE()
    ON_BN_CLICKED(IDC_BTN_NES_RESET       , &CffndbdebugDlg::OnBnClickedBtnNesReset)
    ON_BN_CLICKED(IDC_BTN_NES_RUN_PAUSE   , &CffndbdebugDlg::OnBnClickedBtnNesRunPause)
    ON_BN_CLICKED(IDC_BTN_NES_DEBUG_CPU   , &CffndbdebugDlg::OnBnClickedBtnNesDebugCpu)
    ON_BN_CLICKED(IDC_BTN_NES_DEBUG_PPU   , &CffndbdebugDlg::OnBnClickedBtnNesDebugPpu)
    ON_BN_CLICKED(IDC_BTN_ADD_WATCH       , &CffndbdebugDlg::OnBnClickedBtnAddWatch)
    ON_BN_CLICKED(IDC_BTN_DEL_WATCH       , &CffndbdebugDlg::OnBnClickedBtnDelWatch)
    ON_BN_CLICKED(IDC_BTN_DEL_ALL_WATCH   , &CffndbdebugDlg::OnBnClickedBtnDelAllWatch)
    ON_BN_CLICKED(IDC_BTN_DEL_ALL_BPOINT  , &CffndbdebugDlg::OnBnClickedBtnDelAllBpoint)
    ON_BN_CLICKED(IDC_RDO_CPU_RUN_DEBUG   , &CffndbdebugDlg::OnBnClickedRdoCpuRunDebug)
    ON_BN_CLICKED(IDC_RDO_CPU_RUN_NSTEPS  , &CffndbdebugDlg::OnBnClickedRdoCpuRunNsteps)
    ON_BN_CLICKED(IDC_BTN_CPU_STEP_IN     , &CffndbdebugDlg::OnBnClickedBtnCpuStepIn)
    ON_BN_CLICKED(IDC_BTN_CPU_STEP_OUT    , &CffndbdebugDlg::OnBnClickedBtnCpuStepOut)
    ON_BN_CLICKED(IDC_BTN_CPU_STEP_OVER   , &CffndbdebugDlg::OnBnClickedBtnCpuStepOver)
    ON_BN_CLICKED(IDC_BTN_CPU_TRACKING    , &CffndbdebugDlg::OnBnClickedBtnCpuTracking)
    ON_REGISTERED_MESSAGE(WM_FINDREPLACE  , &CffndbdebugDlg::OnFindReplace)
    ON_NOTIFY(NM_RCLICK, IDC_LST_OPCODE   , &CffndbdebugDlg::OnRclickListDasm)
    ON_NOTIFY(NM_DBLCLK, IDC_LST_OPCODE   , &CffndbdebugDlg::OnDclickListDasm)
    ON_EN_KILLFOCUS(IDC_EDT_LIST_CTRL     , &CffndbdebugDlg::OnEnKillfocusEdtListCtrl)
    ON_COMMAND(ID_ADDBREAKPOINT           , &CffndbdebugDlg::OnAddbreakpoint)
    ON_COMMAND(ID_DELBREAKPOINT           , &CffndbdebugDlg::OnDelbreakpoint)
    ON_COMMAND(ID_DASMLIST_SELECTALL      , &CffndbdebugDlg::OnDasmlistSelectall)
    ON_COMMAND(ID_DASMLIST_COPY           , &CffndbdebugDlg::OnDasmlistCopy)
    ON_COMMAND(ID_DASMLIST_EDIT           , &CffndbdebugDlg::OnDasmlistEdit)
END_MESSAGE_MAP()

// CffndbdebugDlg message handlers
void CffndbdebugDlg::OnOK() {}

void CffndbdebugDlg::OnCancel()
{
    CDialog::OnCancel();
    DestroyWindow();
}

BOOL CffndbdebugDlg::OnInitDialog()
{
    // enable ndb debugging
    ndb_set_debug(&(m_pNES->ndb), NDB_DEBUG_MODE_ENABLE);

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
    RECT rect = {0}; GetClientRect(&rect);
    BITMAPINFO bmpinfo = {0};
    bmpinfo.bmiHeader.biSize        =  sizeof(BITMAPINFOHEADER);
    bmpinfo.bmiHeader.biWidth       =  rect.right;
    bmpinfo.bmiHeader.biHeight      = -rect.bottom;
    bmpinfo.bmiHeader.biPlanes      =  1;
    bmpinfo.bmiHeader.biBitCount    =  32;
    bmpinfo.bmiHeader.biCompression =  BI_RGB;
    HBITMAP hbmp    = CreateDIBSection(m_cdcDraw.GetSafeHdc(), &bmpinfo, DIB_RGB_COLORS, (void**)&m_bmpDrawBuf, NULL, 0);
    BITMAP  bitmap  = {0}; GetObject(hbmp, sizeof(BITMAP), &bitmap);
    m_bmpDrawWidth  = rect.right;
    m_bmpDrawHeight = rect.bottom;
    m_bmpDrawStride = bitmap.bmWidthBytes / 4;
    m_bmpDrawBmp    = CBitmap::FromHandle(hbmp);

    // create font
    m_fntDraw.CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH , "Fixedsys");

    // create pen
    m_penGray .CreatePen(PS_SOLID, 2, RGB(128, 128, 128));
    m_penGreen.CreatePen(PS_SOLID, 1, RGB(0  , 255, 0  ));

    //++ create & init list control
    m_ctrInstructionList.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LVS_REPORT|LVS_SHOWSELALWAYS|WS_TABSTOP, s_rtListCtrl, this, IDC_LST_OPCODE);
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
    m_cdcDraw .DeleteDC();
    m_fntDraw .DeleteObject();
    m_penGray .DeleteObject();
    m_penGreen.DeleteObject();
    m_bmpDrawBmp->DeleteObject();

    // delete m_pDASM
    delete m_pDASM;

    // disable ndb debugging
    ndb_set_debug(&(m_pNES->ndb), NDB_DEBUG_MODE_DISABLE);

    // delete self
    delete this;
}

void CffndbdebugDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    RECT *prect = NULL;

    switch (m_nDebugType)
    {
    case DT_DEBUG_CPU: prect = &s_rtCpuInfo; break;
    case DT_DEBUG_PPU: prect = &s_rtPpuInfo; break;
    }

    if (prect)
    {
        int savedc = m_cdcDraw.SaveDC();
        m_cdcDraw.SelectObject(m_bmpDrawBmp);
        dc.BitBlt(prect->left, prect->top,
                  prect->right - prect->left,
                  prect->bottom - prect->top,
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
            //++ if bank switch occured, we need redo disassemble ++//
            if (m_pNES->ndb.banksw)
            {
                UpdateData(TRUE);
                if (m_bCheckAutoDasm)
                {
                    KillTimer(NDB_REFRESH_TIMER);
                    DoNesRomDisAsm(); // redo dasm
                    m_pNES->ndb.banksw = 0;
                    SetTimer(NDB_REFRESH_TIMER, 50, NULL);
                }
            }
            //-- if bank switch occured, we need redo disassemble --//

            // update cursor of list control for ffndb pc tracking
            if (m_bEnableTracking) UpdateCurInstHighLight();

            // for m_bDebugTracking
            if (m_bDebugTracking && m_pNES->ndb.stop)
            {
                if (m_pNES->ndb.banksw)
                {
                    KillTimer(NDB_REFRESH_TIMER);
                    MessageBox("nes mapper makes bank switch, need to redo disassemble!",
                        "ffndb dasm", MB_ICONASTERISK|MB_ICONINFORMATION);
                    DoNesRomDisAsm(); // redo dasm
                    m_pNES->ndb.banksw = 0;
                    SetTimer(NDB_REFRESH_TIMER, 50, NULL);
                }

                UpdateCurInstHighLight();
                m_bDebugTracking = FALSE;
            }

            if (m_bDebugRunNStep)
            {
                if (m_pNES->ndb.nsteps == 0) m_bDebugRunNStep = FALSE;
                m_strCpuStopNSteps.Format("%d", m_pNES->ndb.nsteps);
                UpdateData(FALSE);
            }

            // draw cpu debug info
            DrawCpuDebugging();
            break;

        case DT_DEBUG_PPU:
            // draw ppu debug info
            DrawPpuDebugging();
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

void CffndbdebugDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    int i = 0;
    switch (m_nDebugType)
    {
    case DT_DEBUG_PPU:
        for (i=0; i<7; i++)
        {
            if (PtInRect(&(s_rtPpuAreas[i]) , point)) break;
        }
        m_nCurPpuDetails  = i;
        m_ptCurPixelPoint = point - CPoint(0, 63);
        break;
    }
    CDialog::OnMouseMove(nFlags, point);
}

void CffndbdebugDlg::OnBnClickedBtnNesReset()
{
    nes_reset(m_pNES);
    m_bDebugTracking = TRUE;
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

    for (int i=IDC_GRP_WATCH_BPOINT; i<=IDC_LST_OPCODE; i++)
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

    for (int i=IDC_GRP_WATCH_BPOINT; i<=IDC_LST_OPCODE; i++)
    {
        CWnd *pwnd = GetDlgItem(i);
        pwnd->ShowWindow(SW_HIDE);
    }
    Invalidate(TRUE);
}

void CffndbdebugDlg::OnBnClickedBtnAddWatch()
{
    DWORD addr;
    UpdateData(TRUE);
    sscanf(m_strWatchAddr, "%x", &addr);
    ndb_add_watch(&(m_pNES->ndb), (WORD)addr);
    m_strWatchAddr.Format("%04X", ++addr);
    UpdateData(FALSE);
}

void CffndbdebugDlg::OnBnClickedBtnDelWatch()
{
    DWORD addr;
    UpdateData(TRUE);
    sscanf(m_strWatchAddr, "%x", &addr);
    ndb_del_watch(&(m_pNES->ndb), (WORD)addr);
}

void CffndbdebugDlg::OnBnClickedBtnDelAllWatch()
{
    for (int i=0; i<16; i++ )
    {
        m_pNES->ndb.watches[i] = 0xffff;
    }
}

void CffndbdebugDlg::OnBnClickedBtnDelAllBpoint()
{
    for (int i=0; i<16; i++)
    {
        int n = ndb_dasm_pc2instn(&(m_pNES->ndb), m_pDASM, m_pNES->ndb.bpoints[i]);
        m_ctrInstructionList.SetItemText(n, 0, "");
        m_pNES->ndb.bpoints[i] = 0xffff;
    }
}

void CffndbdebugDlg::OnBnClickedRdoCpuRunDebug()
{
    ndb_cpu_runto(&(m_pNES->ndb), NDB_CPU_RUN_DEBUG, NULL);
    m_bDebugTracking = TRUE;
}

void CffndbdebugDlg::OnBnClickedRdoCpuRunNsteps()
{
    UpdateData(TRUE);
    ndb_cpu_runto(&(m_pNES->ndb), NDB_CPU_RUN_NSTEPS, (DWORD)atoi(m_strCpuStopNSteps));
    m_bDebugRunNStep = TRUE;
    m_bDebugTracking = TRUE;
}

void CffndbdebugDlg::OnBnClickedBtnCpuStepIn()
{
    ndb_cpu_runto(&(m_pNES->ndb), NDB_CPU_RUN_STEP_IN, NULL); // step in
    m_bDebugTracking = TRUE;
}

void CffndbdebugDlg::OnBnClickedBtnCpuStepOut()
{
    ndb_cpu_runto(&(m_pNES->ndb), NDB_CPU_RUN_STEP_OUT, NULL); // step out
    m_bDebugTracking = TRUE;
}

void CffndbdebugDlg::OnBnClickedBtnCpuStepOver()
{
    ndb_cpu_runto(&(m_pNES->ndb), NDB_CPU_RUN_STEP_OVER, NULL); // step over
    m_bDebugTracking = TRUE;
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

void CffndbdebugDlg::OnDclickListDasm(NMHDR *pNMHDR, LRESULT *pResult)
{
    OnDasmlistEdit();
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
                MessageBox("only support 16 break points !", "ffndb find", MB_ICONASTERISK|MB_ICONINFORMATION);
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

void CffndbdebugDlg::OnDasmlistSelectall()
{
    int total = m_ctrInstructionList.GetItemCount();
    for (int i=0; i<total; i++) {
        m_ctrInstructionList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
    }
}

void CffndbdebugDlg::OnDasmlistCopy()
{
    BeginWaitCursor();
    if (::OpenClipboard(m_hWnd) && ::EmptyClipboard())
    {
        int    total = m_ctrInstructionList.GetSelectedCount();
        size_t cbStr = total * sizeof(char) * 50;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, cbStr);
        if (!hMem)
        {
            CloseClipboard();
            return;
        }

        LPTSTR lpDest = (LPTSTR)GlobalLock(hMem);
        POSITION pos = m_ctrInstructionList.GetFirstSelectedItemPosition();
        if (pos)
        {
            while (pos)
            {
                int n = m_ctrInstructionList.GetNextSelectedItem(pos);

                int len = sprintf_s(lpDest, cbStr, "%-6.4s %-10.8s %-12.12s\r\n",
                    m_ctrInstructionList.GetItemText(n, 1),
                    m_ctrInstructionList.GetItemText(n, 2),
                    m_ctrInstructionList.GetItemText(n, 3));
                lpDest += len;
                cbStr  -= len;
            }
        }
        GlobalUnlock(hMem);

        if (!SetClipboardData(CF_TEXT, hMem))
        {
            CloseClipboard();
            return;
        }
        CloseClipboard();
    }
    EndWaitCursor();
}

void CffndbdebugDlg::OnDasmlistEdit()
{
    m_nCurEditItemRow = m_ctrInstructionList.GetSelectionMark();
    m_nCurEditItemCol = 2;
    if (m_nCurEditItemRow != -1)
    {
        CRect rect;
        m_ctrInstructionList.GetSubItemRect(m_nCurEditItemRow, m_nCurEditItemCol, LVIR_LABEL, rect);
        rect.top  += s_rtListCtrl.top + 0; rect.bottom += s_rtListCtrl.top ;
        rect.left += s_rtListCtrl.left+ 6; rect.right  += s_rtListCtrl.left;
        m_edtListCtrl.SetWindowText(m_ctrInstructionList.GetItemText(m_nCurEditItemRow, m_nCurEditItemCol));
        m_edtListCtrl.MoveWindow(rect);
        m_edtListCtrl.ShowWindow(SW_SHOW);
        m_edtListCtrl.SetFocus();
        m_edtListCtrl.SetSel(0, -1);
    }
}

void CffndbdebugDlg::OnEnKillfocusEdtListCtrl()
{
    CString text;
    m_edtListCtrl.GetWindowText(text);
    m_edtListCtrl.ShowWindow(SW_HIDE);

    // read bytes from edit control
    BYTE bytes[8 ] = {0};
    int n = sscanf_s(text, "%x %x %x %x %x %x", &(bytes[0]), &(bytes[1]), &(bytes[2]), &(bytes[3]), &(bytes[4]), &(bytes[5]));

    // modify rom code
    WORD pc = m_pDASM->instlist[m_nCurEditItemRow].pc;
    m_pNES->cbus[6].membank->type = MEM_RAM; m_pNES->cbus[7].membank->type = MEM_RAM;
    for (int i=0; i<n; i++) bus_writeb(m_pNES->cbus, pc + i, bytes[i]);
    m_pNES->cbus[6].membank->type = MEM_ROM; m_pNES->cbus[7].membank->type = MEM_ROM;

    // disassemble one instruction
    int  btype = 0;
    WORD entry = 0;
    int  len   = 0;
    len = ndb_dasm_one_inst(&(m_pNES->ndb), pc,
        m_pDASM->instlist[m_nCurEditItemRow].bytes,
        m_pDASM->instlist[m_nCurEditItemRow].asmstr,
        m_pDASM->instlist[m_nCurEditItemRow].comment, &btype, &entry);

    // after rom modified, if new instruction length is same as old or not
    if (  len == m_pDASM->instlist[m_nCurEditItemRow].len
       && n   == m_pDASM->instlist[m_nCurEditItemRow].len)
    {
        char str[16];
        for (int i=0; i<len; i++) sprintf(&(str[i*3]), "%02X ", m_pDASM->instlist[m_nCurEditItemRow].bytes[i]);
        m_ctrInstructionList.SetItemText(m_nCurEditItemRow, 2, str);
        m_ctrInstructionList.SetItemText(m_nCurEditItemRow, 3, m_pDASM->instlist[m_nCurEditItemRow].asmstr );
        m_ctrInstructionList.SetItemText(m_nCurEditItemRow, 4, m_pDASM->instlist[m_nCurEditItemRow].comment);
    }
    else
    {
        MessageBox("after rom modified, the new instruction length is not same as old, need to redo disassemble!",
            "ffndb dasm", MB_ICONASTERISK|MB_ICONINFORMATION);
        DoNesRomDisAsm();
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

    m_cdcDraw.SelectObject(m_bmpDrawBmp);
    m_cdcDraw.SelectObject(&m_fntDraw);
    m_cdcDraw.SelectObject(&m_penGray);
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
        int gridy[] = { 3, 3 +22*1, 3 +22*2 };
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
        rect.left = 6; rect.top += 29;
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

    // draw breakpoints & watches
    {
        int  gridx[] = { 3+40*0, 3+40*1, 3+40*2, 3+40*3, 3+40*4, 3+40*5, 3+40*6, 3+40*7, 3+40*8, 3+40*9 };
        int  gridy[] = { 0, 22, 44, 66, 88 };
        char bpwv[128];
        int  i;

        rect.left = 6; rect.top += 29;
        m_cdcDraw.DrawText("break points:", -1, &rect, 0);
        for (i=0; i<2; i++) {
            rect.left = 6; rect.top += 22;
            ndb_dump_info(&(m_pNES->ndb), NDB_DUMP_BREAK_POINT0 + i, bpwv);
            m_cdcDraw.DrawText(bpwv, -1, &rect, 0);
        }
        for (i=0; i<5; i++) gridy[i] += 127;
        DrawGrid(9, 3, gridx, gridy);

        rect.left = 6; rect.top += 29;
        m_cdcDraw.DrawText("watches:", -1, &rect, 0);
        for (i=0; i<4; i++) {
            rect.left = 6; rect.top += 22;
            ndb_dump_info(&(m_pNES->ndb), NDB_DUMP_WATCH0 + i, bpwv);
            m_cdcDraw.DrawText(bpwv, -1, &rect, 0);
        }
        for (i=0; i<5; i++) gridy[i] += 72;
        DrawGrid(10, 5, gridx, gridy);
    }

    // draw bank switch info
    {
        char banksw[128];
        ndb_dump_info(&(m_pNES->ndb), NDB_DUMP_BANKSW, banksw);
        rect.left = 6; rect.top = 295;
        m_cdcDraw.DrawText(banksw, -1, &rect, DT_LEFT);
    }

    // draw edge
    rect.left = rect.top = 0;
    m_cdcDraw.DrawEdge(&rect, EDGE_ETCHED, BF_RECT);

    // restore dc
    m_cdcDraw.RestoreDC(savedc);

    // invalidate rect
    InvalidateRect(&s_rtCpuInfo, FALSE);
}

void CffndbdebugDlg::DrawPpuDebugging()
{
    // save dc
    int savedc = m_cdcDraw.SaveDC();

    // init cdc
    m_cdcDraw.SelectObject(m_bmpDrawBmp);
    m_cdcDraw.SelectObject(&m_fntDraw);
    m_cdcDraw.SelectObject(&m_penGreen);
    m_cdcDraw.SelectObject(GetStockObject(NULL_BRUSH));
    m_cdcDraw.SetBkMode(TRANSPARENT);
    m_cdcDraw.SetTextColor(RGB(255, 255, 255));

    // clear rect
    RECT rect = { 520, 320, 776, 543 };
    m_cdcDraw.FillSolidRect(&rect, RGB(0, 0, 0));

    // draw text
    m_cdcDraw.TextOut(512 + 8, 10 , "bkgrnd tiles:");
    m_cdcDraw.TextOut(512 + 8, 106, "sprite tiles:");
    m_cdcDraw.TextOut(512 + 8, 202, "bkgrnd palette:");
    m_cdcDraw.TextOut(512 + 8, 250, "sprite palette:");
    m_cdcDraw.TextOut(512 + 8, 298, "sprite ram:");

    // use ndb_dump_ppu to draw ppu info
    ndb_dump_ppu(&(m_pNES->ndb), m_bmpDrawBuf, m_bmpDrawWidth, m_bmpDrawHeight, m_bmpDrawStride);

    //+ draw scrolling line
    {
        #define FINEX (m_pNES->ppu.temp1)
        #define FINEY (m_pNES->ppu.temp0 >> 12)
        #define TILEX ((m_pNES->ppu.temp0 >> 0) & 0x1f)
        #define TILEY ((m_pNES->ppu.temp0 >> 5) & 0x1f)
        int x = TILEX * 8 + FINEX + ((m_pNES->ppu.temp0 & (1 << 10)) ? 256 : 0);
        int y = TILEY * 8 + FINEY + ((m_pNES->ppu.temp0 & (1 << 11)) ? 256 : 0);
        m_cdcDraw.MoveTo(x, 0); m_cdcDraw.LineTo(x  , 512);
        m_cdcDraw.MoveTo(0, y); m_cdcDraw.LineTo(512, y  );
        m_cdcDraw.MoveTo((x+256)%512, 0); m_cdcDraw.LineTo((x+256)%512, 512);
        m_cdcDraw.MoveTo(0, (y+240)%480); m_cdcDraw.LineTo(512, (y+240)%480);
    }
    //- draw scrolling line

    //++ for ppu details infomation ++//
    //+ for cursor
    RECT cursor = {0};
    switch (m_nCurPpuDetails)
    {
    case 0: case 1: case 2:
        cursor.left   = m_ptCurPixelPoint.x / 8 * 8;
        cursor.top    = m_ptCurPixelPoint.y / 8 * 8;
        cursor.right  = cursor.left + 8;
        cursor.bottom = cursor.top  + 8;
        break;
    case 3: case 4:
        cursor.left   = (m_ptCurPixelPoint.x - 8) / 16 * 16 + 8;
        cursor.top    = (m_ptCurPixelPoint.y - 0) / 16 * 16 + 0;
        cursor.right  = cursor.left + 16;
        cursor.bottom = cursor.top  + 16;
        break;
    case 5:
        cursor.left   = (m_ptCurPixelPoint.x - 8) / 16 * 16 + 8;
        cursor.top    = (m_ptCurPixelPoint.y - 0) / 16 * 16 + 0;
        cursor.right  = cursor.left + 8;
        cursor.bottom = cursor.top  + 8;
        if (m_pNES->ppu.regs[0x0000] & (1 << 5)) cursor.bottom += 8;
        break;
    }
    //- for cursor

    //+ draw details
    CString details = "";
    int     ntable, pixelx, pixely, tilex, tiley, addr, data;
    switch (m_nCurPpuDetails)
    {
    case 0: // name table
        ntable = m_ptCurPixelPoint.y / 240 * 2 + m_ptCurPixelPoint.x / 256;
        pixelx = m_ptCurPixelPoint.x % 256;
        pixely = m_ptCurPixelPoint.y % 240;
        tilex  = pixelx / 8;
        tiley  = pixely / 8;
        addr   = 0x2000 + 0x400 * ntable + tiley * 32 + tilex;
        data   = bus_readb(m_pNES->pbus, addr);
        details.Format(
            "details:\r\n"
            "nametable-%d\r\n"
            "0x%04X[%03d]: %d\r\n"
            "pixel xy: %3d,%3d\r\n"
            "tile  xy: %3d,%3d",
            ntable, 0x2000 + 0x400 * ntable, tiley * 32 + tilex, data,
            pixelx, pixely, tilex, tiley);
        break;

    case 1: // background tile
        tilex = (m_ptCurPixelPoint.x - 520) / 8;
        tiley = (m_ptCurPixelPoint.y - 32 ) / 8;
        addr  = (m_pNES->ppu.regs[0x0000] & (1 << 4)) ? 0x1000 : 0x0000;
        details.Format(
            "details:\r\n"
            "chrbank: %d\r\n"
            "tabaddr: 0x%04X\r\n"
            "tile id: %d",
            addr ? m_pNES->mmc.cbank1000 : m_pNES->mmc.cbank0000,
            addr,
            tiley * 32 + tilex);
        break;

    case 2: // sprite tile
        tilex = (m_ptCurPixelPoint.x - 520) / 8;
        tiley = (m_ptCurPixelPoint.y - 128) / 8;
        addr  = ((m_pNES->ppu.regs[0x0000] & (1 << 4)) ^ (1 << 4)) ? 0x1000 : 0x0000;
        details.Format(
            "details:\r\n"
            "chrbank: %d\r\n"
            "tabaddr: 0x%04X\r\n"
            "tile id: %d",
            addr ? m_pNES->mmc.cbank1000 : m_pNES->mmc.cbank0000,
            addr,
            tiley * 32 + tilex);
        break;

    case 3: // background palette
        tilex = (m_ptCurPixelPoint.x - 520) / 16;
        tiley = (m_ptCurPixelPoint.y - 224) / 16;
        details.Format(
            "details:\r\n"
            "palette id  : %d\r\n"
            "palette data: %d\r\n"
            "RGB(%3d,%3d,%3d)",
            tilex, m_pNES->ppu.palette[tilex],
            m_pNES->ppu.vdevpal[m_pNES->ppu.palette[tilex] * 4 + 2],
            m_pNES->ppu.vdevpal[m_pNES->ppu.palette[tilex] * 4 + 1],
            m_pNES->ppu.vdevpal[m_pNES->ppu.palette[tilex] * 4 + 0]);
        break;

    case 4: // sprite palette
        tilex = (m_ptCurPixelPoint.x - 520) / 16;
        tiley = (m_ptCurPixelPoint.y - 272) / 16;
        details.Format(
            "details:\r\n"
            "palette id  : %d\r\n"
            "palette data: %d\r\n"
            "RGB(%3d,%3d,%3d)",
            tiley * 16 + tilex, m_pNES->ppu.palette[16 + tilex],
            m_pNES->ppu.vdevpal[m_pNES->ppu.palette[16 + tilex] * 4 + 2],
            m_pNES->ppu.vdevpal[m_pNES->ppu.palette[16 + tilex] * 4 + 1],
            m_pNES->ppu.vdevpal[m_pNES->ppu.palette[16 + tilex] * 4 + 0]);
        break;

    case 5: // sprite ram
        tilex = (m_ptCurPixelPoint.x - 520) / 16;
        tiley = (m_ptCurPixelPoint.y - 320) / 16;
        data  = tiley * 16 + tilex;
        details.Format(
            "details:\r\n"
            "sprite id: %d\r\n"
            "sprite xy: %d, %d\r\n"
            "sprite tile: %d\r\n"
            "%c%c%c 0x%x",
            data,
            m_pNES->ppu.sprram[data * 4 + 3],
            m_pNES->ppu.sprram[data * 4 + 0],
            m_pNES->ppu.sprram[data * 4 + 1],
            (m_pNES->ppu.sprram[data * 4 + 2] & (1 << 7)) ? 'v' : '-',
            (m_pNES->ppu.sprram[data * 4 + 2] & (1 << 6)) ? 'h' : '-',
            (m_pNES->ppu.sprram[data * 4 + 2] & (1 << 5)) ? 'b' : '-',
            (m_pNES->ppu.sprram[data * 4 + 2] & 0x3) << 2);
        break;

    case 6:
        details.Format(
            "details:\r\n"
            "nmi vblk color rx  ry  clk\r\n"
            "%c   %d %c  %c %d  %3d %3d %5d\r\n"
            "vaddr iaddr temp0 temp1 spr  bkg\r\n"
            "%04X  %-4d  %04X  %04X  %c%c%c%c %c%c",
            (m_pNES->ppu.regs[0x0000] & (1 << 7)) ? 'e' : 'd',
            (m_pNES->ppu.regs[0x0002] & (1 << 7)) ? 1 : 0,
            m_pNES->ppu.pin_vbl ? 'h' : 'l',
            (m_pNES->ppu.regs[0x0001] & (1 << 0)) ? 'm' : 'c',
            (m_pNES->ppu.regs[0x0001] >> 5),
            m_pNES->ppu.pclk_line,
            m_pNES->ppu.scanline,
            m_pNES->ppu.pclk_frame,
            m_pNES->ppu.vaddr,
            (m_pNES->ppu.regs[0x0000] & (1 << 2)) ? 32 : 1,
            m_pNES->ppu.temp0,
            m_pNES->ppu.temp1,
            (m_pNES->ppu.regs[0x0001] & (1 << 4)) ? 'v' : '-',
            (m_pNES->ppu.regs[0x0001] & (1 << 2)) ? '-' : 'c',
            (m_pNES->ppu.regs[0x0002] & (1 << 6)) ? 'h' : '-',
            (m_pNES->ppu.regs[0x0002] & (1 << 5)) ? 'o' : '-',
            (m_pNES->ppu.regs[0x0001] & (1 << 3)) ? 'v' : '-',
            (m_pNES->ppu.regs[0x0001] & (1 << 1)) ? '-' : 'c');
        break;
    }
    rect.top = 392;
    m_cdcDraw.DrawText(details, -1, &rect, DT_LEFT);
    if (cursor.right - cursor.left)
    {
        rect.left += 176;
        rect.top  += 8;
        rect.right = rect.left + 64;
        rect.bottom= rect.top  + 64;
        if (  cursor.right - cursor.left == 8
           && cursor.bottom - cursor.top == 16)
        {
            rect.right -= 32;
        }
        if (  cursor.right - cursor.left == 16
           && cursor.bottom - cursor.top == 16)
        {
            m_cdcDraw.StretchBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                &m_cdcDraw, cursor.left + 1, cursor.top + 1, cursor.right - cursor.left - 2, cursor.bottom - cursor.top - 2, SRCCOPY);
        }
        else
        {
            m_cdcDraw.StretchBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                &m_cdcDraw, cursor.left, cursor.top, cursor.right - cursor.left, cursor.bottom - cursor.top, SRCCOPY);
        }
        m_cdcDraw.Rectangle(&rect  ); // draw border
        m_cdcDraw.Rectangle(&cursor); // draw cursor
    }
    //- draw details

    // restore dc
    m_cdcDraw.RestoreDC(savedc);

    // invalidate rect
    InvalidateRect(&s_rtPpuInfo, FALSE);
}

void CffndbdebugDlg::UpdateDasmListControl()
{
    char str[32];
    m_ctrInstructionList.SetRedraw(FALSE);
    m_ctrInstructionList.DeleteAllItems();
    for (int n=0; n<m_pDASM->curinstn; n++)
    {
        m_ctrInstructionList.InsertItem(n, "");

        sprintf(str, "%04X", m_pDASM->instlist[n].pc);
        m_ctrInstructionList.SetItemText(n, 1, str);

        for (int i=0; i<m_pDASM->instlist[n].len; i++) {
            sprintf(&(str[i*3]), "%02X ", m_pDASM->instlist[n].bytes[i]);
        }
        m_ctrInstructionList.SetItemText(n, 2, str);

        m_ctrInstructionList.SetItemText(n, 3, m_pDASM->instlist[n].asmstr );
        m_ctrInstructionList.SetItemText(n, 4, m_pDASM->instlist[n].comment);
    }
    for (int i=0; i<16; i++)
    {
        if (m_pNES->ndb.bpoints[i] != 0xffff)
        {
            m_ctrInstructionList.SetItemText(
                ndb_dasm_pc2instn(&(m_pNES->ndb), m_pDASM, m_pNES->ndb.bpoints[i]),
                0, "B");
        }
    }
    m_ctrInstructionList.SetRedraw(TRUE);
}

void CffndbdebugDlg::DoNesRomDisAsm()
{
    BeginWaitCursor();
    ndb_dasm_nes_rom_begin(&(m_pNES->ndb), m_pDASM);
    ndb_dasm_nes_rom_entry(&(m_pNES->ndb), m_pDASM, bus_readw(m_pNES->cbus, RST_VECTOR));
    ndb_dasm_nes_rom_entry(&(m_pNES->ndb), m_pDASM, bus_readw(m_pNES->cbus, NMI_VECTOR));
    ndb_dasm_nes_rom_entry(&(m_pNES->ndb), m_pDASM, bus_readw(m_pNES->cbus, IRQ_VECTOR));
    ndb_dasm_nes_rom_done (&(m_pNES->ndb), m_pDASM);
    UpdateDasmListControl();
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


