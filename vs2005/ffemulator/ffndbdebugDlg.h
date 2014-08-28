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

protected:
    NES *m_pNES;

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnDestroy();
    afx_msg void OnOK();
    afx_msg void OnCancel();
    afx_msg void OnBnClickedBtnNesReset();
    afx_msg void OnBnClickedBtnNesRunPause();
    afx_msg void OnBnClickedBtnNesDebugCpu();
    afx_msg void OnBnClickedBtnNesDebugPpu();
};
