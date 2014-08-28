#pragma once
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

    #define NDB_TIMER 1

protected:
    NES    *m_pNES;
    int     m_nDebugType;
    CBitmap m_bmpCpuInfo;
    CDC     m_cdcDraw;
    CFont   m_fntDraw;
    CPen    m_penDraw;

protected:
    void DrawCpuInfo();

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

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
    afx_msg void OnBnClickedBtnCpuGoto();
    afx_msg void OnBnClickedBtnCpuStep();

private:
    int     m_nCpuStopCond;
    CString m_strCpuStopNSteps;
    CString m_strCpuStopToPC;
};
