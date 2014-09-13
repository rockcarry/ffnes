#pragma once

// 包含头文件
#include "afxcmn.h"

extern "C" {
#include "../../src/nes.h"
}

// CffndbdebugDlg dialog

#include "afxwin.h"
class CffndbdebugDlg : public CDialog
{
    DECLARE_DYNAMIC(CffndbdebugDlg)

public:
    CffndbdebugDlg(CWnd* pParent, NES *pnes);  // standard constructor
    virtual ~CffndbdebugDlg();

// Dialog Data
    enum { IDD = IDD_FFNDBDEBUG_DIALOG };

    enum {
        DT_DEBUG_CPU,
        DT_DEBUG_PPU,
        DT_DEBUG_MEM,
    };

    #define NDB_REFRESH_TIMER 1
    #define NDB_DIASM_TIMER   2

private:
    NES    *m_pNES;
    DASM   *m_pDASM;
    int     m_nDebugType;
    CBitmap*m_bmpDrawBmp;
    void   *m_bmpDrawBuf;
    LONG    m_bmpDrawStride;
    CDC     m_cdcDraw;
    CFont   m_fntDraw;
    CPen    m_penDraw;
    BOOL    m_bEnableTracking;
    BOOL    m_bDebugTracking;
    BOOL    m_bDebugRunNStep;
    BOOL    m_bIsSearchDown;
    CString m_strCurFindStr;
    int     m_nCurEditItemRow;
    int     m_nCurEditItemCol;

private:
    void DrawGrid(int m, int n, int *x, int *y);
    void DrawCpuDebugging();
    void DoNesRomDisAsm();
    void UpdateDasmListControl();
    void UpdateCurInstHighLight();
    void FindStrInListCtrl(CString str, BOOL down);

private:
    //++ mfc control variables
    CString   m_strWatchAddr;
    CString   m_strCpuStopNSteps;
    BOOL      m_bCheckAutoDasm;
    CListCtrl m_ctrInstructionList;
    CEdit     m_edtListCtrl;
    //-- mfc control variables

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange *pDX);
    virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnOK();
    afx_msg void OnCancel();
    afx_msg void OnBnClickedBtnNesReset();
    afx_msg void OnBnClickedBtnNesRunPause();
    afx_msg void OnBnClickedBtnNesDebugCpu();
    afx_msg void OnBnClickedBtnNesDebugPpu();
    afx_msg void OnBnClickedBtnAddWatch();
    afx_msg void OnBnClickedBtnDelWatch();
    afx_msg void OnBnClickedBtnDelAllWatch();
    afx_msg void OnBnClickedBtnDelAllBpoint();
    afx_msg void OnBnClickedRdoCpuRunDebug();
    afx_msg void OnBnClickedRdoCpuRunNsteps();
    afx_msg void OnBnClickedBtnCpuStepIn();
    afx_msg void OnBnClickedBtnCpuStepOut();
    afx_msg void OnBnClickedBtnCpuStepOver();
    afx_msg void OnBnClickedBtnCpuTracking();
    afx_msg LONG OnFindReplace(WPARAM wParam, LPARAM lParam);
    afx_msg void OnRclickListDasm(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDclickListDasm(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnEnKillfocusEdtListCtrl();
    afx_msg void OnAddbreakpoint();
    afx_msg void OnDelbreakpoint();
    afx_msg void OnDasmlistSelectall();
    afx_msg void OnDasmlistCopy();
    afx_msg void OnDasmlistEdit();
};
