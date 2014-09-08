#pragma once

// 包含头文件
#include "afxcmn.h"

extern "C" {
#include "../../src/nes.h"
}

// CffndbdebugDlg dialog

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
    CBitmap m_bmpCpuInfo;
    CDC     m_cdcDraw;
    CFont   m_fntDraw;
    CPen    m_penDraw;
    BOOL    m_bEnableTracking;
    BOOL    m_bIsSearchDown;
    CString m_strCurFindStr;

private:
    void DrawGrid(int m, int n, int *x, int *y);
    void DrawCpuDebugging();
    void DoNesRomDisAsm();
    void UpdateCurInstHighLight();
    void FindStrInListCtrl(CString str, BOOL down);

private:
    //++ mfc control variables
    int       m_nCpuStopCond;
    CString   m_strCpuStopNSteps;
    CListCtrl m_ctrInstructionList;
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
    afx_msg void OnBnClickedRdoCpuKeepRunning();
    afx_msg void OnBnClickedRdoCpuRunNsteps();
    afx_msg void OnBnClickedRdoCpuRunBpoints();
    afx_msg void OnBnClickedBtnCpuStep();
    afx_msg void OnBnClickedBtnCpuTracking();
    afx_msg LONG OnFindReplace(WPARAM wParam, LPARAM lParam);
    afx_msg void OnRclickListDasm(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnAddbreakpoint();
    afx_msg void OnDelbreakpoint();
};
